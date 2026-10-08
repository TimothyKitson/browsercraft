#pragma once
#include "Transport.h"
#include <cstdint>
#include <unordered_map>

#ifndef __EMSCRIPTEN__

namespace Net
{
    // Deliberately not SOCKET / int: including <winsock2.h> from a header
    // drags half of windows.h into every translation unit that touches
    // networking. The real types live in the .cpp.
#ifdef _WIN32
    using SocketHandle = uintptr_t;
    constexpr SocketHandle INVALID_SOCK = ~static_cast<SocketHandle>(0);
#else
    using SocketHandle = int;
    constexpr SocketHandle INVALID_SOCK = static_cast<SocketHandle>(-1);
#endif

    // Plain non-blocking TCP, one connection per peer, messages framed by
    // a 4-byte little-endian length. This is the desktop half of
    // "Open to LAN", where the LAN is an actual LAN.
    class SocketTransport final : public Transport
    {
    public:
        SocketTransport();
        ~SocketTransport() override;

        bool listen(uint16_t port) override;
        bool connect(const std::string& address, uint16_t port) override;

        void send(PeerId peer, const std::vector<uint8_t>& bytes) override;
        void broadcast(const std::vector<uint8_t>& bytes, PeerId except = HOST_PEER) override;
        void disconnect(PeerId peer) override;

        void poll(std::vector<Packet>& incoming,
                  std::vector<PeerId>& joined,
                  std::vector<PeerId>& left) override;

        void close() override;
        const std::string& error() const override { return m_error; }

    private:
        struct Peer
        {
            SocketHandle socket = INVALID_SOCK;
            // Partial frames in both directions: TCP is a byte stream, so a
            // message can arrive in pieces and leave in pieces.
            std::vector<uint8_t> inbox;
            std::vector<uint8_t> outbox;
            bool connecting = false;   // non-blocking connect still in flight
            bool announced = false;    // already reported through joined()
        };

        SocketHandle m_listener = INVALID_SOCK;
        std::unordered_map<PeerId, Peer> m_peers;
        PeerId m_nextPeerId = 1;
        std::string m_error;

        void closeSocket(SocketHandle& socket);
        void flush(Peer& peer);
        void extractMessages(PeerId id, Peer& peer, std::vector<Packet>& incoming);
    };

    // Minecraft announces an open LAN world by shouting into the void a
    // few times a second; clients just listen. Same idea here, over UDP
    // broadcast so no multicast group membership is needed.
    class LanDiscovery
    {
    public:
        struct Game
        {
            std::string address;
            uint16_t port = 0;
            std::string name;   // "<player>'s world"
            float lastSeen = 0.0f;
        };

        ~LanDiscovery();

        // Host side: repeat this every second or so.
        void startAnnouncing(const std::string& name, uint16_t gamePort);
        void announce(float deltaTime);
        void stopAnnouncing();

        // Guest side.
        void startListening();
        void update(float deltaTime);
        void stopListening();

        const std::vector<Game>& games() const { return m_games; }

    private:
        SocketHandle m_announceSocket = INVALID_SOCK;
        SocketHandle m_listenSocket = INVALID_SOCK;
        std::string m_name;
        uint16_t m_gamePort = 0;
        float m_announceTimer = 0.0f;
        std::vector<Game> m_games;
    };
}

#endif // !__EMSCRIPTEN__
