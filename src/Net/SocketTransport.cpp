#include "SocketTransport.h"

#ifndef __EMSCRIPTEN__

#include "Protocol.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   using socklen_type = int;
#  define SOCK_WOULD_BLOCK (WSAGetLastError() == WSAEWOULDBLOCK)
#  define SOCK_IN_PROGRESS (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
   using socklen_type = socklen_t;
#  define SOCK_WOULD_BLOCK (errno == EAGAIN || errno == EWOULDBLOCK)
#  define SOCK_IN_PROGRESS (errno == EINPROGRESS)
#endif

namespace Net
{
namespace
{
    constexpr uint32_t MAX_MESSAGE_BYTES = 4u * 1024u * 1024u; // sanity cap
    constexpr float ANNOUNCE_INTERVAL = 1.5f;
    constexpr float GAME_TIMEOUT = 5.0f;
    const char* DISCOVERY_MAGIC = "BRWC";

    // Winsock needs starting exactly once per process and never really
    // needs stopping, so a function-local static does the job.
    bool socketsReady()
    {
#ifdef _WIN32
        static const bool ok = []() {
            WSADATA data;
            return WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }();
        return ok;
#else
        return true;
#endif
    }

    void setNonBlocking(SocketHandle socket)
    {
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(static_cast<SOCKET>(socket), FIONBIO, &mode);
#else
        const int flags = fcntl(socket, F_GETFL, 0);
        fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
    }

    void setNoDelay(SocketHandle socket)
    {
        // Block edits are small and latency-sensitive; without this they
        // sit in Nagle's buffer waiting for company.
        const int on = 1;
        setsockopt(static_cast<int>(socket), IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&on), sizeof(on));
    }

    int sendRaw(SocketHandle socket, const uint8_t* data, size_t size)
    {
        return static_cast<int>(::send(static_cast<int>(socket),
                                       reinterpret_cast<const char*>(data),
                                       static_cast<int>(size), 0));
    }

    int recvRaw(SocketHandle socket, uint8_t* data, size_t size)
    {
        return static_cast<int>(::recv(static_cast<int>(socket),
                                       reinterpret_cast<char*>(data),
                                       static_cast<int>(size), 0));
    }
}

// ----------------------------------------------------------- transport ---

SocketTransport::SocketTransport() { socketsReady(); }
SocketTransport::~SocketTransport() { close(); }

void SocketTransport::closeSocket(SocketHandle& socket)
{
    if (socket == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(socket));
#else
    ::close(socket);
#endif
    socket = INVALID_SOCK;
}

bool SocketTransport::listen(uint16_t port)
{
    if (!socketsReady()) { m_error = "Networking unavailable"; return false; }

    m_listener = static_cast<SocketHandle>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (m_listener == INVALID_SOCK) { m_error = "Could not create socket"; return false; }

    const int reuse = 1;
    setsockopt(static_cast<int>(m_listener), SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(static_cast<int>(m_listener), reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) != 0)
    {
        closeSocket(m_listener);
        m_error = "Port " + std::to_string(port) + " is already in use";
        return false;
    }

    if (::listen(static_cast<int>(m_listener), MAX_PLAYERS) != 0)
    {
        closeSocket(m_listener);
        m_error = "Could not open the world to the network";
        return false;
    }

    setNonBlocking(m_listener);
    m_error.clear();
    return true;
}

bool SocketTransport::connect(const std::string& address, uint16_t port)
{
    if (!socketsReady()) { m_error = "Networking unavailable"; return false; }

    SocketHandle socket = static_cast<SocketHandle>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (socket == INVALID_SOCK) { m_error = "Could not create socket"; return false; }

    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &target.sin_addr) != 1)
    {
        closeSocket(socket);
        m_error = address + " is not an address";
        return false;
    }

    // Connecting before going non-blocking would stall the whole frame on
    // an unreachable host, so the handshake finishes inside poll() instead.
    setNonBlocking(socket);
    const int result = ::connect(static_cast<int>(socket),
                                 reinterpret_cast<sockaddr*>(&target), sizeof(target));
    if (result != 0 && !SOCK_IN_PROGRESS)
    {
        closeSocket(socket);
        m_error = "Could not reach " + address;
        return false;
    }

    setNoDelay(socket);

    Peer peer;
    peer.socket = socket;
    peer.connecting = true;
    m_peers[HOST_PEER] = std::move(peer);
    m_error.clear();
    return true;
}

void SocketTransport::send(PeerId id, const std::vector<uint8_t>& bytes)
{
    auto it = m_peers.find(id);
    if (it == m_peers.end()) return;

    const uint32_t length = static_cast<uint32_t>(bytes.size());
    Peer& peer = it->second;
    peer.outbox.push_back(static_cast<uint8_t>(length & 0xFF));
    peer.outbox.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    peer.outbox.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    peer.outbox.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
    peer.outbox.insert(peer.outbox.end(), bytes.begin(), bytes.end());

    if (!peer.connecting) flush(peer);
}

void SocketTransport::broadcast(const std::vector<uint8_t>& bytes, PeerId except)
{
    for (auto& entry : m_peers)
        if (entry.first != except) send(entry.first, bytes);
}

void SocketTransport::disconnect(PeerId id)
{
    auto it = m_peers.find(id);
    if (it == m_peers.end()) return;
    closeSocket(it->second.socket);
    m_peers.erase(it);
}

void SocketTransport::flush(Peer& peer)
{
    while (!peer.outbox.empty())
    {
        const int sent = sendRaw(peer.socket, peer.outbox.data(), peer.outbox.size());
        if (sent > 0)
        {
            peer.outbox.erase(peer.outbox.begin(), peer.outbox.begin() + sent);
            continue;
        }
        // A full send buffer is normal: whatever is left waits for the
        // next poll rather than blocking the frame.
        break;
    }
}

void SocketTransport::extractMessages(PeerId id, Peer& peer, std::vector<Packet>& incoming)
{
    for (;;)
    {
        if (peer.inbox.size() < 4) return;

        const uint32_t length = static_cast<uint32_t>(peer.inbox[0]) |
                                (static_cast<uint32_t>(peer.inbox[1]) << 8) |
                                (static_cast<uint32_t>(peer.inbox[2]) << 16) |
                                (static_cast<uint32_t>(peer.inbox[3]) << 24);

        if (length > MAX_MESSAGE_BYTES)
        {
            // Either a bug or someone poking at the port. Drop them.
            peer.inbox.clear();
            closeSocket(peer.socket);
            return;
        }

        if (peer.inbox.size() < 4 + static_cast<size_t>(length)) return;

        Packet packet;
        packet.peer = id;
        packet.bytes.assign(peer.inbox.begin() + 4, peer.inbox.begin() + 4 + length);
        incoming.push_back(std::move(packet));

        peer.inbox.erase(peer.inbox.begin(), peer.inbox.begin() + 4 + length);
    }
}

void SocketTransport::poll(std::vector<Packet>& incoming,
                           std::vector<PeerId>& joined,
                           std::vector<PeerId>& left)
{
    incoming.clear();
    joined.clear();
    left.clear();

    // --- accept new peers ---
    if (m_listener != INVALID_SOCK)
    {
        for (;;)
        {
            sockaddr_in from{};
            socklen_type fromSize = sizeof(from);
            SocketHandle accepted = static_cast<SocketHandle>(
                ::accept(static_cast<int>(m_listener),
                         reinterpret_cast<sockaddr*>(&from), &fromSize));
            if (accepted == INVALID_SOCK) break;

            if (m_peers.size() >= static_cast<size_t>(MAX_PLAYERS))
            {
                closeSocket(accepted);
                continue;
            }

            setNonBlocking(accepted);
            setNoDelay(accepted);

            Peer peer;
            peer.socket = accepted;
            const PeerId id = m_nextPeerId++;
            m_peers[id] = std::move(peer);
            joined.push_back(id);
        }
    }

    // --- read and write ---
    std::vector<PeerId> dead;
    uint8_t buffer[8192];

    for (auto& entry : m_peers)
    {
        Peer& peer = entry.second;
        if (peer.socket == INVALID_SOCK) { dead.push_back(entry.first); continue; }

        if (peer.connecting)
        {
            // Writable means the non-blocking connect finished -- which
            // includes finishing with a refusal, so SO_ERROR decides.
            fd_set writable, failed;
            FD_ZERO(&writable); FD_ZERO(&failed);
            FD_SET(static_cast<SOCKET>(peer.socket), &writable);
            FD_SET(static_cast<SOCKET>(peer.socket), &failed);
            timeval immediately{ 0, 0 };
            ::select(static_cast<int>(peer.socket) + 1, nullptr, &writable, &failed, &immediately);

            const bool ready = FD_ISSET(static_cast<SOCKET>(peer.socket), &writable) != 0;
            const bool broke = FD_ISSET(static_cast<SOCKET>(peer.socket), &failed) != 0;
            if (!ready && !broke) continue;

            int soError = 0;
            socklen_type size = sizeof(soError);
            getsockopt(static_cast<int>(peer.socket), SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&soError), &size);
            if (broke || soError != 0)
            {
                m_error = "Could not reach the host";
                dead.push_back(entry.first);
                continue;
            }

            peer.connecting = false;
            joined.push_back(entry.first);
            flush(peer);
        }

        for (;;)
        {
            const int received = recvRaw(peer.socket, buffer, sizeof(buffer));
            if (received > 0)
            {
                peer.inbox.insert(peer.inbox.end(), buffer, buffer + received);
                continue;
            }
            if (received == 0) dead.push_back(entry.first);          // clean close
            else if (!SOCK_WOULD_BLOCK) dead.push_back(entry.first); // broken pipe
            break;
        }

        extractMessages(entry.first, peer, incoming);
        if (peer.socket == INVALID_SOCK) dead.push_back(entry.first);
        else flush(peer);
    }

    for (PeerId id : dead)
    {
        auto it = m_peers.find(id);
        if (it == m_peers.end()) continue;
        closeSocket(it->second.socket);
        m_peers.erase(it);
        left.push_back(id);
    }
}

void SocketTransport::close()
{
    for (auto& entry : m_peers) closeSocket(entry.second.socket);
    m_peers.clear();
    closeSocket(m_listener);
}

// ----------------------------------------------------------- discovery ---

LanDiscovery::~LanDiscovery()
{
    stopAnnouncing();
    stopListening();
}

void LanDiscovery::startAnnouncing(const std::string& name, uint16_t gamePort)
{
    if (!socketsReady()) return;
    stopAnnouncing();

    m_announceSocket = static_cast<SocketHandle>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (m_announceSocket == INVALID_SOCK) return;

    const int broadcast = 1;
    setsockopt(static_cast<int>(m_announceSocket), SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

    m_name = name;
    m_gamePort = gamePort;
    m_announceTimer = ANNOUNCE_INTERVAL; // shout immediately
}

void LanDiscovery::announce(float deltaTime)
{
    if (m_announceSocket == INVALID_SOCK) return;

    m_announceTimer += deltaTime;
    if (m_announceTimer < ANNOUNCE_INTERVAL) return;
    m_announceTimer = 0.0f;

    // "BRWC" <port, 2 bytes little-endian> <world name>
    std::string message = DISCOVERY_MAGIC;
    message.push_back(static_cast<char>(m_gamePort & 0xFF));
    message.push_back(static_cast<char>((m_gamePort >> 8) & 0xFF));
    message += m_name;

    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(DISCOVERY_PORT);
    target.sin_addr.s_addr = INADDR_BROADCAST;

    ::sendto(static_cast<int>(m_announceSocket), message.data(),
             static_cast<int>(message.size()), 0,
             reinterpret_cast<sockaddr*>(&target), sizeof(target));
}

void LanDiscovery::stopAnnouncing()
{
    if (m_announceSocket == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(m_announceSocket));
#else
    ::close(m_announceSocket);
#endif
    m_announceSocket = INVALID_SOCK;
}

void LanDiscovery::startListening()
{
    if (!socketsReady()) return;
    stopListening();

    m_listenSocket = static_cast<SocketHandle>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (m_listenSocket == INVALID_SOCK) return;

    const int reuse = 1;
    setsockopt(static_cast<int>(m_listenSocket), SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(DISCOVERY_PORT);

    if (::bind(static_cast<int>(m_listenSocket), reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) != 0)
    {
        stopListening();
        return;
    }

    setNonBlocking(m_listenSocket);
    m_games.clear();
}

void LanDiscovery::update(float deltaTime)
{
    if (m_listenSocket == INVALID_SOCK) return;

    char buffer[512];
    for (;;)
    {
        sockaddr_in from{};
        socklen_type fromSize = sizeof(from);
        const int received = static_cast<int>(
            ::recvfrom(static_cast<int>(m_listenSocket), buffer, sizeof(buffer) - 1, 0,
                       reinterpret_cast<sockaddr*>(&from), &fromSize));
        if (received < 7) break;
        if (std::memcmp(buffer, DISCOVERY_MAGIC, 4) != 0) continue;

        Game game;
        game.port = static_cast<uint16_t>(static_cast<uint8_t>(buffer[4]) |
                                          (static_cast<uint8_t>(buffer[5]) << 8));
        game.name.assign(buffer + 6, received - 6);

        char text[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &from.sin_addr, text, sizeof(text));
        game.address = text;

        auto existing = std::find_if(m_games.begin(), m_games.end(), [&](const Game& g) {
            return g.address == game.address && g.port == game.port;
        });
        if (existing != m_games.end()) { existing->name = game.name; existing->lastSeen = 0.0f; }
        else m_games.push_back(game);
    }

    // Forget worlds that stopped shouting.
    for (Game& game : m_games) game.lastSeen += deltaTime;
    m_games.erase(std::remove_if(m_games.begin(), m_games.end(),
                                 [](const Game& g) { return g.lastSeen > GAME_TIMEOUT; }),
                  m_games.end());
}

void LanDiscovery::stopListening()
{
    if (m_listenSocket == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(m_listenSocket));
#else
    ::close(m_listenSocket);
#endif
    m_listenSocket = INVALID_SOCK;
    m_games.clear();
}

} // namespace Net

#endif // !__EMSCRIPTEN__
