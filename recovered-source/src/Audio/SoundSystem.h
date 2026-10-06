#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Positional sound playback over SDL2's audio device.
//
// Clips are Ogg Vorbis under assets/sounds, decoded to 16-bit stereo once on
// first use and kept in memory; the whole set is a couple of megabytes. Mixing
// happens in SDL's audio callback, so everything the callback touches is
// behind m_mutex and the public API only ever queues work.
//
// Sounds are addressed the way the rest of the engine names them: a path plus
// a variant number, so ("dig/stone", 4) picks one of dig/stone1.ogg through
// dig/stone4.ogg at random.
class SoundSystem
{
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int MAX_VOICES = 24;
    static constexpr float HEARING_RANGE = 36.0f; // silent beyond this

    SoundSystem();
    ~SoundSystem();

    SoundSystem(const SoundSystem&) = delete;
    SoundSystem& operator=(const SoundSystem&) = delete;

    bool available() const { return m_device != 0; }

    // Where the listener is, so sounds can be attenuated and panned. `forward`
    // and `right` come from the camera.
    void setListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& right);

    // Positional. `name` is a path without the variant number or extension.
    void play(const std::string& name, int variants, const glm::vec3& position,
              float volume = 1.0f, float pitch = 1.0f);

    // Non-positional, for UI clicks and anything that belongs to the player.
    void playGlobal(const std::string& name, int variants, float volume = 1.0f, float pitch = 1.0f);

    void setMuted(bool muted) { m_muted = muted; }
    bool muted() const { return m_muted; }

private:
    struct Clip
    {
        std::vector<int16_t> samples; // interleaved stereo
        bool loaded = false;          // false means the file was missing or bad
    };

    struct Voice
    {
        const Clip* clip = nullptr;
        double cursor = 0.0;  // fractional sample index, so pitch can shift
        double step = 1.0;
        float leftGain = 1.0f;
        float rightGain = 1.0f;
        bool active = false;
    };

    // Full path for one take of a sound, picking a variant at random.
    std::string resolve(const std::string& name, int variants);
    const Clip* clipFor(const std::string& path);
    void queue(const Clip* clip, float leftGain, float rightGain, float pitch);

    static void audioCallback(void* userData, uint8_t* stream, int length);
    void mix(int16_t* output, int frames);

    uint32_t m_device = 0;
    bool m_muted = false;

    std::mutex m_mutex;
    std::unordered_map<std::string, Clip> m_clips;
    Voice m_voices[MAX_VOICES];

    glm::vec3 m_listenerPosition{ 0.0f };
    glm::vec3 m_listenerForward{ 0.0f, 0.0f, 1.0f };
    glm::vec3 m_listenerRight{ 1.0f, 0.0f, 0.0f };

    uint32_t m_rng = 0x1234567u;
};
