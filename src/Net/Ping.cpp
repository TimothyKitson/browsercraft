#include "Ping.h"
#include "Protocol.h"
#include <cctype>
#include <cstdlib>
#include <chrono>
#include <thread>
#ifndef __EMSCRIPTEN__
#include "SocketTransport.h"
#endif

bool Net::splitAddress(const std::string& text, std::string& host, uint16_t& port)
{
    size_t start = 0, end = text.size();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    const std::string trimmed = text.substr(start, end - start);
    if (trimmed.empty()) return false;
    const size_t colon = trimmed.rfind(':');
    host = trimmed;
    port = DEFAULT_PORT;
    if (colon != std::string::npos && trimmed.find(':') == colon)
    {
        host = trimmed.substr(0, colon);
        const std::string digits = trimmed.substr(colon + 1);
        if (digits.empty() || digits.size() > 5) return false;
        for (char c : digits)
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        const long value = std::strtol(digits.c_str(), nullptr, 10);
        if (value <= 0 || value > 65535) return false;
        port = static_cast<uint16_t>(value);
    }
    return !host.empty();
}

int Net::pingBars(int ms)
{
    if (ms < 150) return 5;
    if (ms < 300) return 4;
    if (ms < 600) return 3;
    if (ms < 1000) return 2;
    return 1;
}

void Net::Pinger::start(const std::string& address)
{
    m_shared = std::make_shared<Shared>();
#ifdef __EMSCRIPTEN__
    m_shared->result.state = PingResult::State::Unreachable;
    (void)address;
#else
    std::shared_ptr<Shared> shared = m_shared;
    std::thread([shared, address]() {
        PingResult out;
        std::string host;
        uint16_t port = DEFAULT_PORT;
        auto finish = [&]() {
            std::lock_guard<std::mutex> guard(shared->lock);
            shared->result = out;
        };
        if (!splitAddress(address, host, port))
        {
            out.state = PingResult::State::Unresolved;
            finish();
            return;
        }
        SocketTransport transport;
        const auto began = std::chrono::steady_clock::now();
        if (!transport.connect(host, port))
        {
            out.state = transport.error() == "Can't resolve hostname" ? PingResult::State::Unresolved : PingResult::State::Unreachable;
            finish();
            return;
        }
        std::vector<Packet> incoming;
        std::vector<PeerId> joined, left;
        bool asked = false, answered = false;
        while (!answered)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - began).count();
            if (elapsed > 3000) break;
            transport.poll(incoming, joined, left);
            if (!left.empty()) break;
            if (!joined.empty() && !asked)
            {
                Writer query(MessageId::StatusQuery);
                query.u32(PROTOCOL_VERSION);
                transport.send(HOST_PEER, query.bytes());
                asked = true;
            }
            for (const Packet& packet : incoming)
            {
                Reader reader(packet.bytes.data(), packet.bytes.size());
                if (reader.id() != MessageId::StatusReply) continue;
                const uint32_t version = reader.u32();
                out.motd = reader.str();
                out.online = static_cast<int>(reader.u32());
                out.max = static_cast<int>(reader.u32());
                out.version = reader.str();
                out.ms = static_cast<int>(elapsed);
                out.state = version == PROTOCOL_VERSION ? PingResult::State::Online : PingResult::State::Incompatible;
                answered = true;
            }
            if (!answered) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        transport.close();
        if (!answered) out.state = PingResult::State::Unreachable;
        finish();
    }).detach();
#endif
}

Net::PingResult Net::Pinger::result() const
{
    if (!m_shared) return PingResult{};
    std::lock_guard<std::mutex> guard(m_shared->lock);
    return m_shared->result;
}
