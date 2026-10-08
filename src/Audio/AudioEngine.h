#pragma once
#include "World/Block.h"
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <cstdint>

enum class Sound : uint8_t
{
    DigStone,
    DigDirt,
    DigGrass,
    DigWood,
    DigSand,
    DigGlass,
    DigWool,
    DigPlant,
    StepStone,
    StepGravel,
    StepGrass,
    StepWood,
    StepSand,
    StepCloth,
    StepSnow,
    Place,
    Jump,
    Land,
    Hurt,
    Pickup,
    Click,
    Count
};

// Sounds come from assets/sounds when that folder exists, and are
// synthesised otherwise.
//
// Same arrangement as the textures: real Minecraft audio is Mojang's and
// cannot ship in a public build, but nothing stops you dropping it in on
// your own machine. The synthesised set is what browsercraft.net serves,
// and it means the game is never silent.
//
// Each sound holds several variants, because vanilla has four or five
// takes of every footstep and picking between them is most of why
// walking does not sound like a machine.
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void play(Sound sound, float volume = 1.0f, float pitch = 1.0f);

    // Convenience wrappers that pick the right sound for a block.
    void playDig(BlockId block, float volume = 1.0f);
    void playStep(BlockId block);
    void playPlace(BlockId block);

    void setMuted(bool muted) { m_muted = muted; }
    bool muted() const { return m_muted; }
    bool available() const { return m_device != 0; }

private:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int MAX_VOICES = 24;

    struct Voice
    {
        const std::vector<float>* buffer = nullptr;
        double position = 0.0;
        double step = 1.0;
        float volume = 1.0f;
    };

    unsigned int m_device = 0;
    bool m_muted = false;
    using Clip = std::vector<float>;
    std::array<std::vector<Clip>, static_cast<size_t>(Sound::Count)> m_buffers;
    int m_loadedFromFiles = 0;
    // Somewhere harmless for the synth to write when a sound already came
    // from a file and the synthesised version is not wanted.
    Clip m_scratch;
    std::array<Voice, MAX_VOICES> m_voices{};
    std::mutex m_mutex;

    void buildSounds();
    // Decodes one Ogg Vorbis file to mono at the mixer's sample rate.
    static bool loadOgg(const std::string& path, Clip& out);
    // Loads "<stem>1.ogg", "<stem>2.ogg", ... until one is missing.
    int loadVariants(Sound id, const char* stem, int maxVariants);
    bool loadSingle(Sound id, const char* file);
    void mix(float* output, int frames);
    static void callback(void* userData, uint8_t* stream, int lengthBytes);
};
