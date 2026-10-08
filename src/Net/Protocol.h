#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// The wire format for "Open to LAN".
//
// The host owns the world; everyone else is a guest whose edits are
// requests. The one trick that makes this cheap: guests generate the
// terrain themselves from the host's seed, so the only block data that
// ever crosses the wire is the difference between the generated world and
// the host's actual one. An untouched world costs four bytes to share.
namespace Net
{
    // Bumped whenever the layout below changes, so an old client meets a
    // new host with a clear error instead of garbled blocks.
    constexpr uint32_t PROTOCOL_VERSION = 1;
    constexpr uint16_t DEFAULT_PORT = 25585;
    constexpr uint16_t DISCOVERY_PORT = 25586;
    constexpr int MAX_PLAYERS = 8;

    enum class MessageId : uint8_t
    {
        // guest -> host
        Hello = 1,       // name, protocol version
        RequestBlock,    // "I would like to mine/place here"
        Move,            // position and look, several times a second

        // host -> guest
        Welcome = 64,    // seed, game mode, time, your id, spawn point
        Reject,          // reason string; the connection closes after this
        ChunkDelta,      // every block the host has changed from generation
        DeltaComplete,   // all deltas sent; the guest can start playing
        BlockChange,     // one block changed, by anyone
        PlayerJoin,      // id + name of someone who arrived
        PlayerLeave,
        PlayerMove,      // id + position and look
        TimeSync,        // the host's clock wins
        Chat
    };

    // --- little-endian byte buffers -------------------------------------
    //
    // Hand-rolled rather than memcpy of structs: struct padding differs
    // between the MSVC desktop build and the Emscripten web build, and the
    // two have to talk to each other.

    class Writer
    {
    public:
        explicit Writer(MessageId id) { u8(static_cast<uint8_t>(id)); }

        void u8(uint8_t v) { m_bytes.push_back(v); }
        void u16(uint16_t v) { u8(v & 0xFF); u8((v >> 8) & 0xFF); }
        void u32(uint32_t v) { u16(v & 0xFFFF); u16((v >> 16) & 0xFFFF); }
        void i32(int32_t v) { u32(static_cast<uint32_t>(v)); }
        void f32(float v) { uint32_t bits; std::memcpy(&bits, &v, 4); u32(bits); }
        void vec3(const glm::vec3& v) { f32(v.x); f32(v.y); f32(v.z); }

        void str(const std::string& v)
        {
            const uint16_t length = static_cast<uint16_t>(v.size() > 0xFFFF ? 0xFFFF : v.size());
            u16(length);
            m_bytes.insert(m_bytes.end(), v.begin(), v.begin() + length);
        }

        const std::vector<uint8_t>& bytes() const { return m_bytes; }

    private:
        std::vector<uint8_t> m_bytes;
    };

    // Reads never run off the end: past it, every read returns zero and
    // ok() goes false, so a truncated or hostile packet is just ignored
    // instead of crashing the host.
    class Reader
    {
    public:
        Reader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

        MessageId id() { return static_cast<MessageId>(u8()); }

        uint8_t u8()
        {
            if (m_pos >= m_size) { m_ok = false; return 0; }
            return m_data[m_pos++];
        }
        uint16_t u16() { uint16_t a = u8(); return static_cast<uint16_t>(a | (u8() << 8)); }
        uint32_t u32() { uint32_t a = u16(); return a | (static_cast<uint32_t>(u16()) << 16); }
        int32_t i32() { return static_cast<int32_t>(u32()); }
        float f32() { const uint32_t bits = u32(); float v; std::memcpy(&v, &bits, 4); return v; }
        glm::vec3 vec3() { const float x = f32(), y = f32(), z = f32(); return glm::vec3(x, y, z); }

        std::string str()
        {
            const uint16_t length = u16();
            if (!m_ok || m_pos + length > m_size) { m_ok = false; return {}; }
            std::string out(reinterpret_cast<const char*>(m_data + m_pos), length);
            m_pos += length;
            return out;
        }

        bool ok() const { return m_ok; }
        size_t remaining() const { return m_ok ? m_size - m_pos : 0; }

    private:
        const uint8_t* m_data;
        size_t m_size;
        size_t m_pos = 0;
        bool m_ok = true;
    };
}
