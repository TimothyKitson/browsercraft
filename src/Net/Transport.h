#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Net
{
    // Peers are numbered from 1; 0 means "the host" / "nobody".
    using PeerId = uint32_t;
    constexpr PeerId HOST_PEER = 0;

    struct Packet
    {
        PeerId peer = HOST_PEER;
        std::vector<uint8_t> bytes;
    };

    // A reliable, ordered, message-oriented link to one or more peers.
    //
    // Two implementations sit behind this: TCP sockets on the desktop,
    // where a LAN is a real LAN, and WebRTC data channels in the browser,
    // where it isn't. Nothing above this interface knows which it is --
    // that is the whole point of it existing.
    class Transport
    {
    public:
        virtual ~Transport() = default;

        // Start accepting peers. Returns false if the port is taken.
        virtual bool listen(uint16_t port) = 0;
        // Start connecting to a host. Non-blocking: watch joined() for the
        // host peer appearing, or failed() for giving up.
        virtual bool connect(const std::string& address, uint16_t port) = 0;

        virtual void send(PeerId peer, const std::vector<uint8_t>& bytes) = 0;
        virtual void broadcast(const std::vector<uint8_t>& bytes, PeerId except = HOST_PEER) = 0;
        virtual void disconnect(PeerId peer) = 0;

        // Pumps the sockets. Everything that arrived since the last call
        // lands in the three out-parameters; they are cleared first.
        virtual void poll(std::vector<Packet>& incoming,
                          std::vector<PeerId>& joined,
                          std::vector<PeerId>& left) = 0;

        virtual void close() = 0;

        // Set once something has gone permanently wrong, for the UI.
        virtual const std::string& error() const = 0;
    };
}
