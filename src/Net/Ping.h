#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace Net
{
    struct PingResult
    {
        enum class State : uint8_t { Pinging, Online, Unreachable, Unresolved, Incompatible };
        State state = State::Pinging;
        int ms = 0;
        std::string motd;
        int online = 0;
        int max = 0;
        std::string version;
    };

    bool splitAddress(const std::string& text, std::string& host, uint16_t& port);
    int pingBars(int ms);

    class Pinger
    {
    public:
        void start(const std::string& address);
        PingResult result() const;
        bool started() const { return static_cast<bool>(m_shared); }

    private:
        struct Shared
        {
            std::mutex lock;
            PingResult result;
        };
        std::shared_ptr<Shared> m_shared;
    };
}
