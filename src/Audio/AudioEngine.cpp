#define STB_VORBIS_NO_PUSHDATA_API
#include <stb_vorbis.c>

#include "AudioEngine.h"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace
{
    constexpr float TAU = 6.2831853071795864f;

    // Deterministic white noise, so the sounds are identical every run.
    struct Rng
    {
        uint32_t state;
        explicit Rng(uint32_t seed) : state(seed ? seed : 1u) {}
        float next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (static_cast<float>(state & 0xFFFFFF) / static_cast<float>(0x1000000)) * 2.0f - 1.0f;
        }
    };

    // One-pole filter: the cheapest way to turn flat white noise into
    // something that sounds like a material rather than static.
    struct OnePole
    {
        float value = 0.0f;
        float coefficient = 1.0f;

        void setCutoff(float hz, float sampleRate)
        {
            coefficient = 1.0f - std::exp(-TAU * hz / sampleRate);
        }
        float lowPass(float x)
        {
            value += (x - value) * coefficient;
            return value;
        }
    };

    struct Synth
    {
        std::vector<float>& out;
        int sampleRate;

        float t(size_t i) const { return static_cast<float>(i) / sampleRate; }

        void addNoise(float seconds, float cutoffHz, float decay, float amplitude,
                      bool highPass, uint32_t seed)
        {
            Rng rng(seed);
            OnePole filter;
            filter.setCutoff(cutoffHz, static_cast<float>(sampleRate));

            const size_t count = static_cast<size_t>(seconds * sampleRate);
            if (out.size() < count) out.resize(count, 0.0f);

            for (size_t i = 0; i < count; ++i)
            {
                const float raw = rng.next();
                const float low = filter.lowPass(raw);
                const float shaped = highPass ? (raw - low) : low;
                out[i] += shaped * amplitude * std::exp(-t(i) * decay);
            }
        }

        void addTone(float seconds, float startHz, float endHz, float decay,
                     float amplitude, bool square = false, float delaySeconds = 0.0f)
        {
            const size_t count = static_cast<size_t>(seconds * sampleRate);
            const size_t offset = static_cast<size_t>(delaySeconds * sampleRate);
            if (out.size() < offset + count) out.resize(offset + count, 0.0f);

            float phase = 0.0f;
            for (size_t i = 0; i < count; ++i)
            {
                const float progress = static_cast<float>(i) / std::max<size_t>(1, count - 1);
                const float hz = startHz + (endHz - startHz) * progress;
                phase += TAU * hz / sampleRate;
                float wave = std::sin(phase);
                if (square) wave = wave >= 0.0f ? 0.6f : -0.6f;
                out[offset + i] += wave * amplitude * std::exp(-t(i) * decay);
            }
        }

        // Keeps the mix from clipping and stops buffers starting or ending
        // on a non-zero sample, which would click.
        void finish(float peak = 0.85f)
        {
            if (out.empty()) return;

            float maximum = 0.0f;
            for (float sample : out) maximum = std::max(maximum, std::fabs(sample));
            if (maximum > 0.0001f)
            {
                const float gain = peak / maximum;
                for (float& sample : out) sample *= gain;
            }

            const size_t fade = std::min<size_t>(out.size() / 2, sampleRate / 500); // ~2 ms
            for (size_t i = 0; i < fade; ++i)
            {
                const float k = static_cast<float>(i) / fade;
                out[i] *= k;
                out[out.size() - 1 - i] *= k;
            }
        }
    };
}

AudioEngine::AudioEngine()
{
    buildSounds();

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        std::printf("Audio: unavailable (%s) - running silent\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec want{};
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = &AudioEngine::callback;
    want.userdata = this;

    SDL_AudioSpec have{};
    m_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (m_device == 0)
    {
        std::printf("Audio: could not open device (%s) - running silent\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(m_device, 0);

    size_t samples = 0, clipCount = 0;
    for (const auto& variants : m_buffers)
        for (const auto& clip : variants) { samples += clip.size(); ++clipCount; }

    std::printf("Audio: %d clips (%d loaded from assets/sounds), %.0f KB at %d Hz\n",
                static_cast<int>(clipCount), m_loadedFromFiles,
                samples * sizeof(float) / 1024.0, have.freq);
}

AudioEngine::~AudioEngine()
{
    if (m_device)
    {
        SDL_CloseAudioDevice(m_device);
        m_device = 0;
    }
}

bool AudioEngine::loadOgg(const std::string& path, Clip& out)
{
    int channels = 0, rate = 0;
    short* samples = nullptr;
    const int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &samples);
    if (frames <= 0 || !samples) return false;

    // Downmix to mono and resample to the mixer rate. Vanilla clips are a
    // mix of 44.1 and 48 kHz, and playing a 48 kHz clip as though it were
    // 44.1 drops it almost a semitone.
    const double ratio = static_cast<double>(rate) / SAMPLE_RATE;
    const int outFrames = static_cast<int>(frames / ratio);
    out.resize(static_cast<size_t>(outFrames > 0 ? outFrames : 0));

    for (int i = 0; i < outFrames; ++i)
    {
        const double source = i * ratio;
        const int index = static_cast<int>(source);
        const double fraction = source - index;

        auto frameAt = [&](int f) {
            if (f >= frames) f = frames - 1;
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += samples[static_cast<size_t>(f) * channels + c] / 32768.0f;
            return sum / channels;
        };

        out[i] = static_cast<float>(frameAt(index) * (1.0 - fraction) +
                                    frameAt(index + 1) * fraction);
    }

    free(samples);
    return !out.empty();
}

int AudioEngine::loadVariants(Sound id, const char* stem, int maxVariants)
{
    auto& clips = m_buffers[static_cast<size_t>(id)];
    for (int n = 1; n <= maxVariants; ++n)
    {
        Clip clip;
        char path[256];
        std::snprintf(path, sizeof(path), "assets/sounds/%s%d.ogg", stem, n);
        if (!loadOgg(path, clip)) break;
        clips.push_back(std::move(clip));
    }
    m_loadedFromFiles += static_cast<int>(clips.size());
    return static_cast<int>(clips.size());
}

bool AudioEngine::loadSingle(Sound id, const char* file)
{
    Clip clip;
    if (!loadOgg(std::string("assets/sounds/") + file, clip)) return false;
    m_buffers[static_cast<size_t>(id)].push_back(std::move(clip));
    ++m_loadedFromFiles;
    return true;
}

// Minecraft's own layout: mob/<name>/say1.ogg and so on. Death is often
// a single file rather than a numbered set, so both spellings are tried.
// Anything with no file keeps the synthesised voice below.
void AudioEngine::loadMobVoice(Sound say, Sound hurt, Sound death, const char* folder)
{
    char stem[128];

    std::snprintf(stem, sizeof(stem), "mob/%s/say", folder);
    loadVariants(say, stem, 5);

    std::snprintf(stem, sizeof(stem), "mob/%s/hurt", folder);
    if (loadVariants(hurt, stem, 5) == 0)
    {
        std::snprintf(stem, sizeof(stem), "mob/%s/hurt.ogg", folder);
        loadSingle(hurt, stem);
    }

    std::snprintf(stem, sizeof(stem), "mob/%s/death", folder);
    if (loadVariants(death, stem, 5) == 0)
    {
        std::snprintf(stem, sizeof(stem), "mob/%s/death.ogg", folder);
        loadSingle(death, stem);
    }
}

void AudioEngine::buildSounds()
{
    // Real audio first, wherever it has been installed.
    loadVariants(Sound::DigStone, "dig/stone", 4);
    loadVariants(Sound::DigDirt, "dig/gravel", 4);
    loadVariants(Sound::DigGrass, "dig/grass", 4);
    loadVariants(Sound::DigWood, "dig/wood", 4);
    loadVariants(Sound::DigSand, "dig/sand", 4);
    loadVariants(Sound::DigGlass, "random/glass", 3);
    loadVariants(Sound::DigWool, "dig/cloth", 4);
    loadVariants(Sound::DigPlant, "dig/grass", 4);

    loadVariants(Sound::StepStone, "step/stone", 6);
    loadVariants(Sound::StepGravel, "step/gravel", 6);
    loadVariants(Sound::StepGrass, "step/grass", 6);
    loadVariants(Sound::StepWood, "step/wood", 6);
    loadVariants(Sound::StepSand, "step/sand", 6);
    loadVariants(Sound::StepCloth, "step/cloth", 6);
    loadVariants(Sound::StepSnow, "step/snow", 6);

    loadSingle(Sound::Pickup, "random/pop.ogg");
    loadSingle(Sound::Click, "random/click.ogg");
    loadSingle(Sound::Land, "damage/fallsmall.ogg");
    loadVariants(Sound::Hurt, "damage/hit", 3);

    loadMobVoice(Sound::MobSheepSay, Sound::MobSheepHurt, Sound::MobSheepDeath, "sheep");
    loadMobVoice(Sound::MobPigSay, Sound::MobPigHurt, Sound::MobPigDeath, "pig");
    loadMobVoice(Sound::MobCowSay, Sound::MobCowHurt, Sound::MobCowDeath, "cow");
    loadMobVoice(Sound::MobChickenSay, Sound::MobChickenHurt, Sound::MobChickenDeath, "chicken");
    loadMobVoice(Sound::MobZombieSay, Sound::MobZombieHurt, Sound::MobZombieDeath, "zombie");
    loadMobVoice(Sound::MobSkeletonSay, Sound::MobSkeletonHurt, Sound::MobSkeletonDeath, "skeleton");
    loadMobVoice(Sound::MobCreeperSay, Sound::MobCreeperHurt, Sound::MobCreeperDeath, "creeper");
    loadMobVoice(Sound::MobSpiderSay, Sound::MobSpiderHurt, Sound::MobSpiderDeath, "spider");
    loadMobVoice(Sound::MobEndermanSay, Sound::MobEndermanHurt, Sound::MobEndermanDeath, "enderman");
    loadMobVoice(Sound::MobPigmanSay, Sound::MobPigmanHurt, Sound::MobPigmanDeath, "zombie_pigman");
    loadMobVoice(Sound::MobVillagerSay, Sound::MobVillagerHurt, Sound::MobVillagerDeath, "villager");
    loadMobVoice(Sound::MobWolfSay, Sound::MobWolfHurt, Sound::MobWolfDeath, "wolf");
    loadMobVoice(Sound::MobGolemSay, Sound::MobGolemHurt, Sound::MobGolemDeath,
                 "mob/irongolem");
    loadMobVoice(Sound::MobHorseSay, Sound::MobHorseHurt, Sound::MobHorseDeath, "horse");
    loadMobVoice(Sound::MobBlazeSay, Sound::MobBlazeHurt, Sound::MobBlazeDeath, "blaze");
    loadMobVoice(Sound::MobWitherSkeletonSay, Sound::MobWitherSkeletonHurt,
                 Sound::MobWitherSkeletonDeath, "mob/wither_skeleton");
    loadMobVoice(Sound::MobMagmaCubeSay, Sound::MobMagmaCubeHurt,
                 Sound::MobMagmaCubeDeath, "slime");
    loadMobVoice(Sound::MobBreezeSay, Sound::MobBreezeHurt,
                 Sound::MobBreezeDeath, "breeze");
    loadMobVoice(Sound::MobGuardianSay, Sound::MobGuardianHurt,
                 Sound::MobGuardianDeath, "mob/guardian");
    loadMobVoice(Sound::MobWitchSay, Sound::MobWitchHurt,
                 Sound::MobWitchDeath, "mob/witch");
    loadMobVoice(Sound::MobDragonSay, Sound::MobDragonHurt,
                 Sound::MobDragonDeath, "mob/enderdragon");
    loadMobVoice(Sound::MobWitherSay, Sound::MobWitherHurt,
                 Sound::MobWitherDeath, "mob/wither");
    loadMobVoice(Sound::MobWardenSay, Sound::MobWardenHurt,
                 Sound::MobWardenDeath, "mob/warden");
    loadMobVoice(Sound::MobShulkerSay, Sound::MobShulkerHurt,
                 Sound::MobShulkerDeath, "mob/shulker");
    loadMobVoice(Sound::MobDrownedSay, Sound::MobDrownedHurt,
                 Sound::MobDrownedDeath, "mob/drowned");
    loadMobVoice(Sound::MobFrogSay, Sound::MobFrogHurt,
                 Sound::MobFrogDeath, "mob/frog");
    loadVariants(Sound::Rain, "weather/rain", 1);
    loadVariants(Sound::Thunder, "weather/thunder", 1);

    auto make = [&](Sound id) -> Synth {
        auto& clips = m_buffers[static_cast<size_t>(id)];
        // Anything that came from a file keeps it; only the gaps get
        // filled, so a partial install still sounds right.
        if (!clips.empty()) { m_scratch.clear(); return Synth{ m_scratch, SAMPLE_RATE }; }
        clips.emplace_back();
        return Synth{ clips.back(), SAMPLE_RATE };
    };

    // Each material gets a filtered noise burst; the cutoff and decay are
    // what make stone read as stone and wool as wool.
    {
        Synth s = make(Sound::DigStone);
        s.addNoise(0.11f, 2200.0f, 42.0f, 1.0f, false, 11);
        s.addTone(0.09f, 115.0f, 95.0f, 55.0f, 0.30f);
        s.finish();
    }
    // Footsteps. These are loaded from assets/sounds when a real set is
    // installed, and had no synthesised fallback at all -- so the build
    // that browsercraft.net serves, which has no audio assets, walked
    // in complete silence. Each is a short noise burst shaped like the
    // thing underfoot: brighter and sharper on stone, broader on
    // gravel, muffled on grass and cloth, hollow on wood.
    {
        Synth s = make(Sound::StepStone);
        s.addNoise(0.055f, 2600.0f, 70.0f, 1.0f, false, 101);
        s.addTone(0.045f, 150.0f, 120.0f, 80.0f, 0.22f);
        s.finish(0.55f);
    }
    {
        Synth s = make(Sound::StepGravel);
        s.addNoise(0.075f, 1500.0f, 48.0f, 1.0f, false, 103);
        s.finish(0.52f);
    }
    {
        Synth s = make(Sound::StepGrass);
        s.addNoise(0.070f, 1700.0f, 52.0f, 1.0f, true, 107);
        s.finish(0.42f);
    }
    {
        Synth s = make(Sound::StepWood);
        s.addNoise(0.060f, 1900.0f, 60.0f, 1.0f, false, 109);
        // The hollow note under it is what makes a plank floor sound
        // like boards rather than like stone.
        s.addTone(0.080f, 320.0f, 210.0f, 36.0f, 0.34f, true);
        s.finish(0.50f);
    }
    {
        Synth s = make(Sound::StepSand);
        s.addNoise(0.080f, 3200.0f, 40.0f, 1.0f, true, 113);
        s.finish(0.40f);
    }
    {
        Synth s = make(Sound::StepCloth);
        s.addNoise(0.085f, 800.0f, 38.0f, 1.0f, true, 127);
        s.finish(0.34f);
    }
    {
        Synth s = make(Sound::StepSnow);
        s.addNoise(0.075f, 2000.0f, 44.0f, 1.0f, true, 131);
        s.addTone(0.050f, 200.0f, 160.0f, 60.0f, 0.18f);
        s.finish(0.40f);
    }

    {
        Synth s = make(Sound::DigDirt);
        s.addNoise(0.09f, 900.0f, 50.0f, 1.0f, false, 22);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::DigGrass);
        s.addNoise(0.09f, 1300.0f, 55.0f, 1.0f, true, 33);
        s.finish(0.55f);
    }
    {
        Synth s = make(Sound::DigWood);
        s.addNoise(0.13f, 1800.0f, 45.0f, 0.8f, false, 44);
        s.addTone(0.13f, 300.0f, 270.0f, 32.0f, 0.5f);
        s.finish();
    }
    {
        Synth s = make(Sound::DigSand);
        s.addNoise(0.12f, 1500.0f, 28.0f, 1.0f, true, 55);
        s.finish(0.6f);
    }
    {
        Synth s = make(Sound::DigGlass);
        s.addNoise(0.20f, 3000.0f, 30.0f, 0.7f, true, 66);
        s.addTone(0.18f, 2400.0f, 2300.0f, 18.0f, 0.25f, false, 0.01f);
        s.addTone(0.16f, 3100.0f, 3000.0f, 20.0f, 0.20f, false, 0.03f);
        s.addTone(0.14f, 4200.0f, 4100.0f, 22.0f, 0.15f, false, 0.06f);
        s.finish();
    }
    {
        Synth s = make(Sound::DigWool);
        s.addNoise(0.08f, 600.0f, 60.0f, 1.0f, false, 77);
        s.finish(0.45f);
    }
    {
        Synth s = make(Sound::DigPlant);
        s.addNoise(0.07f, 2500.0f, 70.0f, 1.0f, true, 88);
        s.finish(0.45f);
    }
    {
        Synth s = make(Sound::Place);
        s.addTone(0.10f, 95.0f, 80.0f, 45.0f, 0.6f);
        s.addNoise(0.06f, 1200.0f, 80.0f, 0.5f, false, 99);
        s.finish(0.8f);
    }
    {
        Synth s = make(Sound::Jump);
        s.addNoise(0.09f, 700.0f, 45.0f, 1.0f, false, 123);
        s.finish(0.30f);
    }
    {
        Synth s = make(Sound::Land);
        s.addTone(0.14f, 70.0f, 55.0f, 30.0f, 0.7f);
        s.addNoise(0.10f, 900.0f, 45.0f, 0.5f, false, 124);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::Hurt);
        s.addTone(0.25f, 420.0f, 170.0f, 12.0f, 0.5f, true);
        s.finish(0.6f);
    }
    {
        Synth s = make(Sound::Pickup);
        s.addTone(0.10f, 700.0f, 1150.0f, 22.0f, 0.5f);
        s.finish(0.45f);
    }
    {
        Synth s = make(Sound::Click);
        s.addNoise(0.03f, 3000.0f, 200.0f, 1.0f, false, 150);
        s.finish(0.35f);
    }
    {
        // Fire: a crackle, which is noise with no tone under it at all.
        Synth s = make(Sound::Fire);
        s.addNoise(0.28f, 1900.0f, 9.0f, 0.60f, true, 151);
        s.addNoise(0.28f, 420.0f, 7.0f, 0.34f, false, 152);
        s.finish(0.52f);
    }

    // --- animals and monsters ---
    //
    // A voice is a falling tone with a little noise over it: the pitch it
    // starts at and how fast it slides is most of what separates a sheep
    // Each species has its own voice, so a sheep and a zombie never share
    // a cry. Anything a pack supplies wins; the rest is synthesised, and
    // every species carries a pitch multiplier on top.
    {
        Synth s = make(Sound::MobSheepSay);
        s.addTone(0.38f, 420.0f, 330.0f, 5.0f, 0.55f, true);
        s.addTone(0.34f, 630.0f, 500.0f, 7.0f, 0.22f);
        s.addNoise(0.30f, 1800.0f, 9.0f, 0.18f, false, 211);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobSheepHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 251);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobSheepDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 291);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobPigSay);
        s.addTone(0.30f, 240.0f, 150.0f, 8.0f, 0.70f, true);
        s.addNoise(0.26f, 900.0f, 12.0f, 0.22f, false, 212);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobPigHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 252);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobPigDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 292);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobCowSay);
        s.addTone(0.46f, 190.0f, 120.0f, 5.0f, 0.72f, true);
        s.addNoise(0.40f, 760.0f, 9.0f, 0.22f, false, 222);
        s.finish(0.78f);
    }
    {
        Synth s = make(Sound::MobCowHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 262);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobCowDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 302);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobChickenSay);
        s.addTone(0.10f, 900.0f, 1250.0f, 26.0f, 0.45f);
        s.addTone(0.13f, 760.0f, 540.0f, 20.0f, 0.38f, false, 0.09f);
        s.addNoise(0.09f, 2600.0f, 34.0f, 0.20f, true, 213);
        s.finish(0.65f);
    }
    {
        Synth s = make(Sound::MobChickenHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 253);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobChickenDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 293);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobZombieSay);
        s.addTone(0.60f, 150.0f, 96.0f, 4.0f, 0.70f, true);
        s.addTone(0.55f, 228.0f, 150.0f, 5.0f, 0.26f);
        s.addNoise(0.50f, 700.0f, 6.0f, 0.26f, false, 214);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobZombieHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 254);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobZombieDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 294);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobSkeletonSay);
        for (int i = 0; i < 5; ++i)
            s.addTone(0.05f, 1500.0f - i * 90.0f, 900.0f, 70.0f, 0.42f, false, i * 0.055f);
        s.addNoise(0.28f, 4200.0f, 18.0f, 0.20f, true, 215);
        s.finish(0.60f);
    }
    {
        Synth s = make(Sound::MobSkeletonHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 255);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobSkeletonDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 295);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobCreeperSay);
        s.addNoise(0.52f, 5200.0f, 5.5f, 1.0f, true, 216);
        s.addNoise(0.52f, 1400.0f, 6.5f, 0.30f, false, 217);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobCreeperHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 256);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobCreeperDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 296);
        s.finish(0.80f);
    }
    // The Enderman: a low, hollow note with a rasp under it.
    {
        Synth s = make(Sound::MobEndermanSay);
        s.addTone(0.55f, 120.0f, 86.0f, 3.0f, 0.60f, true);
        s.addNoise(0.40f, 700.0f, 5.0f, 0.22f, false, 601);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobEndermanHurt);
        s.addTone(0.22f, 200.0f, 90.0f, 14.0f, 0.70f, true);
        s.addNoise(0.18f, 1400.0f, 18.0f, 0.30f, false, 611);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobEndermanDeath);
        s.addTone(0.70f, 150.0f, 50.0f, 4.0f, 0.75f, true);
        s.addNoise(0.55f, 900.0f, 5.0f, 0.28f, false, 621);
        s.finish(0.85f);
    }

    // Rain: a long hiss, played over and over while it falls. There is
    // no looping voice in this engine, so the clip is long enough that
    // the joins between repeats are not obvious.
    {
        Synth s = make(Sound::Rain);
        s.addNoise(2.40f, 4200.0f, 0.0f, 0.55f, true, 701);
        s.addNoise(2.40f, 1500.0f, 0.0f, 0.22f, true, 703);
        s.finish(0.40f);
    }

    // Thunder: a crack with a long roll after it.
    {
        Synth s = make(Sound::Thunder);
        s.addNoise(0.22f, 2600.0f, 14.0f, 1.00f, false, 711);
        s.addNoise(2.60f, 260.0f, 1.6f, 0.80f, true, 713);
        s.addTone(1.80f, 56.0f, 30.0f, 1.4f, 0.60f, true);
        s.finish(0.95f);
    }

    // The villager: the famous two-note "hrmm", which is a hum that
    // steps down rather than sliding, so the two notes are separate
    // tones instead of one tone bent in the middle.
    {
        Synth s = make(Sound::MobVillagerSay);
        s.addTone(0.18f, 300.0f, 300.0f, 9.0f, 0.55f, true);
        s.addTone(0.22f, 215.0f, 205.0f, 7.0f, 0.60f, true);
        s.addNoise(0.10f, 700.0f, 14.0f, 0.10f, false, 661);
        s.finish(0.72f);
    }
    {
        Synth s = make(Sound::MobVillagerHurt);
        s.addTone(0.16f, 380.0f, 190.0f, 16.0f, 0.62f, true);
        s.addNoise(0.12f, 1100.0f, 22.0f, 0.22f, false, 663);
        s.finish(0.78f);
    }
    {
        Synth s = make(Sound::MobVillagerDeath);
        s.addTone(0.52f, 330.0f, 90.0f, 5.0f, 0.68f, true);
        s.addNoise(0.26f, 900.0f, 9.0f, 0.24f, false, 665);
        s.finish(0.82f);
    }

    // The wolf: a short bark, which is a burst of noise with a tone
    // under it rather than a tone with noise on top -- a dog's voice is
    // mostly air.
    {
        Synth s = make(Sound::MobWolfSay);
        s.addNoise(0.09f, 1500.0f, 30.0f, 0.55f, false, 701);
        s.addTone(0.11f, 420.0f, 260.0f, 20.0f, 0.45f, true);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobWolfHurt);
        s.addTone(0.20f, 700.0f, 300.0f, 13.0f, 0.60f, true);
        s.addNoise(0.12f, 1800.0f, 24.0f, 0.26f, false, 703);
        s.finish(0.76f);
    }
    {
        Synth s = make(Sound::MobWolfDeath);
        s.addTone(0.46f, 600.0f, 130.0f, 6.0f, 0.64f, true);
        s.addNoise(0.22f, 1200.0f, 10.0f, 0.22f, false, 705);
        s.finish(0.80f);
    }

    // The iron golem: not a voice at all. A ton of iron moving is a
    // low grinding scrape, so it is built the other way up from an
    // animal -- noise low down with barely any tone in it.
    {
        Synth s = make(Sound::MobGolemSay);
        s.addNoise(0.34f, 260.0f, 9.0f, 0.40f, false, 811);
        s.addTone(0.26f, 90.0f, 70.0f, 7.0f, 0.30f, true);
        s.finish(0.72f);
    }
    {
        Synth s = make(Sound::MobGolemHurt);
        s.addNoise(0.22f, 420.0f, 16.0f, 0.46f, false, 813);
        s.addTone(0.18f, 150.0f, 110.0f, 12.0f, 0.34f, true);
        s.finish(0.78f);
    }
    {
        Synth s = make(Sound::MobGolemDeath);
        s.addNoise(0.60f, 300.0f, 5.0f, 0.44f, false, 815);
        s.addTone(0.52f, 120.0f, 60.0f, 4.0f, 0.38f, true);
        s.finish(0.84f);
    }

    // The horse: a whinny, which is a tone that wavers rather than
    // slides -- the waver is the whole character of the sound.
    {
        Synth s = make(Sound::MobHorseSay);
        s.addTone(0.42f, 390.0f, 300.0f, 4.0f, 0.55f, true);
        s.addTone(0.30f, 520.0f, 430.0f, 6.0f, 0.34f, true);
        s.addNoise(0.16f, 900.0f, 11.0f, 0.16f, false, 711);
        s.finish(0.72f);
    }
    {
        Synth s = make(Sound::MobHorseHurt);
        s.addTone(0.22f, 520.0f, 240.0f, 12.0f, 0.62f, true);
        s.addNoise(0.14f, 1300.0f, 20.0f, 0.24f, false, 713);
        s.finish(0.78f);
    }
    {
        Synth s = make(Sound::MobHorseDeath);
        s.addTone(0.58f, 440.0f, 105.0f, 5.0f, 0.66f, true);
        s.addNoise(0.28f, 1000.0f, 8.0f, 0.22f, false, 715);
        s.finish(0.82f);
    }

    // The blaze: not a voice but a furnace. Breathing noise with a
    // crackle over the top of it and no tone to speak of, which is what
    // tells you one is in the room before you can see it.
    {
        Synth s = make(Sound::MobBlazeSay);
        s.addNoise(0.52f, 1100.0f, 3.0f, 0.34f, true, 821);
        s.addNoise(0.52f, 240.0f, 2.0f, 0.30f, false, 822);
        s.finish(0.62f);
    }
    {
        Synth s = make(Sound::MobBlazeHurt);
        s.addNoise(0.22f, 2600.0f, 14.0f, 0.46f, true, 823);
        s.addTone(0.16f, 680.0f, 300.0f, 14.0f, 0.20f, false);
        s.finish(0.74f);
    }
    {
        Synth s = make(Sound::MobBlazeDeath);
        s.addNoise(0.64f, 1800.0f, 5.0f, 0.44f, true, 825);
        s.addNoise(0.64f, 300.0f, 4.0f, 0.34f, false, 826);
        s.finish(0.80f);
    }

    // The wither skeleton: a skeleton's rattle, deeper and slower. The
    // pitch shift on the species does most of the work; what is left is
    // making it drier than the one up top.
    {
        Synth s = make(Sound::MobWitherSkeletonSay);
        s.addNoise(0.26f, 1500.0f, 11.0f, 0.38f, true, 831);
        s.addTone(0.20f, 180.0f, 120.0f, 9.0f, 0.22f, true);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobWitherSkeletonHurt);
        s.addNoise(0.18f, 2200.0f, 18.0f, 0.44f, true, 833);
        s.addTone(0.14f, 260.0f, 140.0f, 16.0f, 0.24f, true);
        s.finish(0.76f);
    }
    {
        Synth s = make(Sound::MobWitherSkeletonDeath);
        s.addNoise(0.52f, 1700.0f, 6.0f, 0.42f, true, 835);
        s.addTone(0.40f, 200.0f, 90.0f, 5.0f, 0.26f, true);
        s.finish(0.80f);
    }

    // The magma cube: a wet slap. Low noise that dies almost at once,
    // which is the whole of a slime landing.
    {
        Synth s = make(Sound::MobMagmaCubeSay);
        s.addNoise(0.18f, 520.0f, 22.0f, 0.42f, false, 841);
        s.finish(0.68f);
    }
    {
        Synth s = make(Sound::MobMagmaCubeHurt);
        s.addNoise(0.14f, 760.0f, 30.0f, 0.46f, false, 843);
        s.finish(0.74f);
    }
    {
        Synth s = make(Sound::MobMagmaCubeDeath);
        s.addNoise(0.34f, 420.0f, 12.0f, 0.44f, false, 845);
        s.addNoise(0.34f, 1400.0f, 9.0f, 0.18f, true, 846);
        s.finish(0.78f);
    }

    // The wither: a rattle rather than a roar. Three heads, so three
    // tones close enough together to beat against each other, which
    // is what makes it sound wrong rather than just loud.
    {
        Synth s = make(Sound::MobWitherSay);
        s.addTone(1.10f, 196.0f, 150.0f, 1.8f, 0.64f, true);
        s.addTone(1.10f, 203.0f, 156.0f, 1.8f, 0.52f, true);
        s.addTone(1.10f, 131.0f, 100.0f, 1.6f, 0.46f, true);
        s.addNoise(0.80f, 540.0f, 2.6f, 0.20f, true, 901);
        s.finish(0.86f);
    }
    {
        Synth s = make(Sound::MobWitherHurt);
        s.addTone(0.34f, 300.0f, 150.0f, 10.0f, 0.74f, true);
        s.addNoise(0.28f, 1000.0f, 12.0f, 0.26f, false, 902);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobWitherDeath);
        s.addTone(1.80f, 210.0f, 58.0f, 1.3f, 0.74f, true);
        s.addTone(1.80f, 139.0f, 40.0f, 1.2f, 0.50f, true);
        s.addNoise(1.30f, 460.0f, 1.8f, 0.22f, true, 903);
        s.finish(0.90f);
    }

    // The dragon: the biggest noise in the game. A long roar made of
    // two tones a fifth apart sliding down together, with a growl of
    // noise under them -- loud and low rather than sharp, so it
    // carries across the island.
    {
        Synth s = make(Sound::MobDragonSay);
        s.addTone(1.60f, 150.0f, 64.0f, 1.2f, 0.78f, true);
        s.addTone(1.60f, 100.0f, 42.0f, 1.1f, 0.54f, true);
        s.addNoise(1.20f, 320.0f, 2.0f, 0.22f, true, 891);
        s.finish(0.90f);
    }
    {
        Synth s = make(Sound::MobDragonHurt);
        s.addTone(0.50f, 260.0f, 110.0f, 7.0f, 0.80f, true);
        s.addNoise(0.42f, 800.0f, 9.0f, 0.28f, false, 892);
        s.finish(0.86f);
    }
    {
        Synth s = make(Sound::MobDragonDeath);
        s.addTone(2.40f, 170.0f, 34.0f, 1.0f, 0.80f, true);
        s.addTone(2.40f, 112.0f, 22.0f, 0.9f, 0.56f, true);
        s.addNoise(1.80f, 400.0f, 1.6f, 0.24f, true, 893);
        s.finish(0.94f);
    }

    // The witch: a cackle, which is a tone that jumps rather than
    // slides -- the one voice in the game that goes up at the end.
    {
        Synth s = make(Sound::MobWitchSay);
        s.addTone(0.46f, 290.0f, 420.0f, 9.0f, 0.60f, true);
        s.addTone(0.46f, 360.0f, 560.0f, 7.0f, 0.34f, true);
        s.addNoise(0.30f, 1400.0f, 12.0f, 0.14f, false, 881);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobWitchHurt);
        s.addTone(0.22f, 420.0f, 200.0f, 16.0f, 0.70f, true);
        s.addNoise(0.18f, 1700.0f, 20.0f, 0.26f, false, 882);
        s.finish(0.76f);
    }
    {
        Synth s = make(Sound::MobWitchDeath);
        s.addTone(0.80f, 380.0f, 120.0f, 3.4f, 0.66f, true);
        s.addNoise(0.60f, 1100.0f, 4.0f, 0.20f, true, 883);
        s.finish(0.78f);
    }

    // The guardian: the groan of something that lives underwater,
    // which is a low tone bent downwards with water noise over it.
    {
        Synth s = make(Sound::MobGuardianSay);
        s.addTone(0.70f, 160.0f, 78.0f, 2.2f, 0.62f, true);
        s.addNoise(0.70f, 520.0f, 2.4f, 0.16f, true, 861);
        s.finish(0.62f);
    }
    {
        Synth s = make(Sound::MobGuardianHurt);
        s.addTone(0.26f, 240.0f, 110.0f, 12.0f, 0.70f, true);
        s.addNoise(0.22f, 900.0f, 16.0f, 0.22f, false, 862);
        s.finish(0.74f);
    }
    {
        Synth s = make(Sound::MobGuardianDeath);
        s.addTone(0.90f, 150.0f, 52.0f, 3.0f, 0.60f, true);
        s.addNoise(0.90f, 420.0f, 3.2f, 0.20f, true, 863);
        s.finish(0.70f);
    }

    // The warden: the lowest thing in the game. Almost no top end at
    // all -- a heartbeat rather than a voice, which is what it is.
    {
        Synth s = make(Sound::MobWardenSay);
        s.addTone(1.20f, 62.0f, 44.0f, 1.4f, 0.80f, true);
        s.addTone(1.20f, 31.0f, 22.0f, 1.2f, 0.50f, true);
        s.addNoise(0.40f, 180.0f, 5.0f, 0.12f, true, 871);
        s.finish(0.86f);
    }
    {
        Synth s = make(Sound::MobWardenHurt);
        s.addTone(0.40f, 96.0f, 50.0f, 9.0f, 0.82f, true);
        s.addNoise(0.34f, 400.0f, 11.0f, 0.26f, false, 872);
        s.finish(0.84f);
    }
    {
        Synth s = make(Sound::MobWardenDeath);
        s.addTone(1.50f, 70.0f, 26.0f, 1.8f, 0.80f, true);
        s.addTone(1.50f, 35.0f, 14.0f, 1.6f, 0.52f, true);
        s.addNoise(0.80f, 260.0f, 3.0f, 0.18f, true, 873);
        s.finish(0.88f);
    }

    // The shulker: a shell opening and shutting. Mostly a hard knock
    // with a short ring after it, which is a box rather than a voice.
    {
        Synth s = make(Sound::MobShulkerSay);
        s.addNoise(0.14f, 1800.0f, 22.0f, 0.34f, false, 881);
        s.addTone(0.30f, 420.0f, 300.0f, 7.0f, 0.42f, true);
        s.finish(0.58f);
    }
    {
        Synth s = make(Sound::MobShulkerHurt);
        s.addNoise(0.18f, 2600.0f, 18.0f, 0.40f, false, 882);
        s.addTone(0.22f, 620.0f, 280.0f, 12.0f, 0.44f, true);
        s.finish(0.62f);
    }
    {
        Synth s = make(Sound::MobShulkerDeath);
        s.addNoise(0.40f, 1400.0f, 6.0f, 0.36f, true, 883);
        s.addTone(0.50f, 340.0f, 120.0f, 4.0f, 0.46f, true);
        s.finish(0.66f);
    }

    // The drowned: the zombie's groan with the water over it, which
    // is the same shape bent lower and run through a dull filter.
    {
        Synth s = make(Sound::MobDrownedSay);
        s.addTone(0.60f, 128.0f, 86.0f, 2.6f, 0.56f, true);
        s.addNoise(0.60f, 380.0f, 2.8f, 0.22f, true, 891);
        s.finish(0.58f);
    }
    {
        Synth s = make(Sound::MobDrownedHurt);
        s.addTone(0.28f, 190.0f, 96.0f, 11.0f, 0.64f, true);
        s.addNoise(0.26f, 640.0f, 13.0f, 0.26f, false, 892);
        s.finish(0.66f);
    }
    {
        Synth s = make(Sound::MobDrownedDeath);
        s.addTone(0.85f, 140.0f, 50.0f, 3.2f, 0.58f, true);
        s.addNoise(0.85f, 320.0f, 3.4f, 0.24f, true, 893);
        s.finish(0.64f);
    }

    // The frog: a croak, which is a short low tone with a rasp over
    // it and nothing else at all.
    {
        Synth s = make(Sound::MobFrogSay);
        s.addTone(0.22f, 210.0f, 150.0f, 9.0f, 0.60f, true);
        s.addNoise(0.22f, 900.0f, 11.0f, 0.22f, true, 901);
        s.finish(0.54f);
    }
    {
        Synth s = make(Sound::MobFrogHurt);
        s.addTone(0.16f, 380.0f, 190.0f, 16.0f, 0.62f, true);
        s.addNoise(0.14f, 1400.0f, 18.0f, 0.24f, false, 902);
        s.finish(0.58f);
    }
    {
        Synth s = make(Sound::MobFrogDeath);
        s.addTone(0.34f, 260.0f, 110.0f, 6.0f, 0.58f, true);
        s.addNoise(0.34f, 700.0f, 7.0f, 0.22f, true, 903);
        s.finish(0.58f);
    }

    // The breeze: wind, which is noise with no tone under it and a
    // long slow swell rather than a hit.
    {
        Synth s = make(Sound::MobBreezeSay);
        s.addNoise(0.60f, 2400.0f, 2.5f, 0.30f, true, 851);
        s.addNoise(0.60f, 700.0f, 2.0f, 0.22f, true, 852);
        s.finish(0.56f);
    }
    {
        Synth s = make(Sound::MobBreezeHurt);
        s.addNoise(0.20f, 3200.0f, 15.0f, 0.42f, true, 853);
        s.finish(0.70f);
    }
    {
        Synth s = make(Sound::MobBreezeDeath);
        s.addNoise(0.70f, 2000.0f, 4.0f, 0.40f, true, 855);
        s.addNoise(0.70f, 520.0f, 3.0f, 0.24f, true, 856);
        s.finish(0.76f);
    }

    // The pigman: a grunt with a groan in it, halfway between the two
    // things it is made of.
    {
        Synth s = make(Sound::MobPigmanSay);
        s.addTone(0.34f, 210.0f, 140.0f, 7.0f, 0.66f, true);
        s.addNoise(0.28f, 820.0f, 10.0f, 0.24f, false, 631);
        s.finish(0.76f);
    }
    {
        Synth s = make(Sound::MobPigmanHurt);
        s.addTone(0.20f, 260.0f, 120.0f, 15.0f, 0.70f, true);
        s.addNoise(0.16f, 1200.0f, 20.0f, 0.32f, false, 641);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobPigmanDeath);
        s.addTone(0.60f, 220.0f, 70.0f, 5.0f, 0.74f, true);
        s.addNoise(0.48f, 1000.0f, 6.0f, 0.30f, false, 651);
        s.finish(0.84f);
    }

    {
        Synth s = make(Sound::MobSpiderSay);
        for (int i = 0; i < 6; ++i)
            s.addTone(0.04f, 2100.0f - i * 120.0f, 1400.0f, 90.0f, 0.34f, false, i * 0.042f);
        s.addNoise(0.22f, 5200.0f, 22.0f, 0.18f, true, 225);
        s.finish(0.55f);
    }
    {
        Synth s = make(Sound::MobSpiderHurt);
        s.addTone(0.18f, 430.0f, 210.0f, 16.0f, 0.65f, true);
        s.addNoise(0.14f, 2000.0f, 22.0f, 0.34f, false, 265);
        s.finish(0.75f);
    }
    {
        Synth s = make(Sound::MobSpiderDeath);
        s.addTone(0.55f, 360.0f, 90.0f, 6.0f, 0.70f, true);
        s.addNoise(0.45f, 1500.0f, 8.0f, 0.30f, false, 305);
        s.finish(0.80f);
    }
    {
        Synth s = make(Sound::MobStep);
        s.addNoise(0.07f, 1100.0f, 50.0f, 1.0f, false, 220);
        s.finish(0.30f);
    }
}

int AudioEngine::clipCount(Sound sound) const
{
    const size_t index = static_cast<size_t>(sound);
    if (index >= m_buffers.size()) return 0;
    return static_cast<int>(m_buffers[index].size());
}

void AudioEngine::play(Sound sound, float volume, float pitch)
{
    if (m_muted || m_device == 0) return;

    const size_t index = static_cast<size_t>(sound);
    if (index >= m_buffers.size() || m_buffers[index].empty()) return;

    // Whatever asked for this, it does not get to ask again for another
    // few hundredths of a second. One clip retriggered every frame is a
    // buzz, not a sound.
    const uint32_t now = SDL_GetTicks();
    const uint32_t gap = static_cast<uint32_t>(RETRIGGER_SECONDS * 1000.0f);
    if (m_lastPlayed[index] != 0 && now - m_lastPlayed[index] < gap) return;
    m_lastPlayed[index] = now;

    // Pick one of the takes at random, the way Minecraft does.
    static uint32_t rolling = 12345u;
    rolling = rolling * 1664525u + 1013904223u;
    const auto& variants = m_buffers[index];
    const Clip& clip = variants[(rolling >> 16) % variants.size()];
    if (clip.empty()) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Take the first idle voice; if they're all busy the sound is dropped,
    // which is better than cutting off something already playing.
    for (Voice& voice : m_voices)
    {
        if (voice.buffer != nullptr) continue;
        voice.buffer = &clip;
        voice.position = 0.0;
        voice.step = std::clamp(static_cast<double>(pitch), 0.25, 4.0);
        voice.volume = std::clamp(volume, 0.0f, 1.0f);
        return;
    }
}

void AudioEngine::playDig(BlockId block, float volume)
{
    switch (blockMaterial(block))
    {
        case Material::Stone: play(Sound::DigStone, volume); break;
        case Material::Dirt:  play(Sound::DigDirt, volume); break;
        case Material::Grass: play(Sound::DigGrass, volume); break;
        case Material::Wood:  play(Sound::DigWood, volume); break;
        case Material::Sand:  play(Sound::DigSand, volume); break;
        case Material::Glass: play(Sound::DigGlass, volume); break;
        case Material::Wool:  play(Sound::DigWool, volume); break;
        case Material::Snow:  play(Sound::DigWool, volume); break;
        case Material::Plant: play(Sound::DigPlant, volume); break;
        default: break;
    }
}

void AudioEngine::playStep(BlockId block)
{
    // Vanilla has dedicated footstep clips, several takes of each, rather
    // than a dig sound played higher; play() picks between the takes.
    switch (blockMaterial(block))
    {
        case Material::Stone: play(Sound::StepStone, 0.30f); break;
        case Material::Dirt:  play(Sound::StepGravel, 0.32f); break;
        case Material::Grass: play(Sound::StepGrass, 0.34f); break;
        case Material::Wood:  play(Sound::StepWood, 0.30f); break;
        case Material::Sand:  play(Sound::StepSand, 0.32f); break;
        case Material::Glass: play(Sound::StepStone, 0.20f); break;
        case Material::Wool:  play(Sound::StepCloth, 0.34f); break;
        case Material::Snow:  play(Sound::StepSnow, 0.34f); break;
        default:              play(Sound::StepGravel, 0.26f); break;
    }
}

void AudioEngine::playPlace(BlockId block)
{
    // Minecraft has no separate "place" sound: putting a block down plays
    // that block's own material sound. Layering a synthesised blip on top
    // of a real sample made every placement sound doubled.
    playDig(block, 0.8f);
}

void AudioEngine::mix(float* output, int frames)
{
    std::fill(output, output + frames * 2, 0.0f);

    std::lock_guard<std::mutex> lock(m_mutex);

    for (Voice& voice : m_voices)
    {
        if (voice.buffer == nullptr) continue;
        const std::vector<float>& data = *voice.buffer;

        for (int i = 0; i < frames; ++i)
        {
            const size_t sampleIndex = static_cast<size_t>(voice.position);
            if (sampleIndex + 1 >= data.size())
            {
                voice.buffer = nullptr;
                break;
            }

            // Linear interpolation so pitch-shifted playback isn't gritty.
            const float fraction = static_cast<float>(voice.position - sampleIndex);
            const float sample = data[sampleIndex] * (1.0f - fraction) + data[sampleIndex + 1] * fraction;
            const float value = sample * voice.volume;

            output[i * 2] += value;
            output[i * 2 + 1] += value;
            voice.position += voice.step;
        }
    }

    for (int i = 0; i < frames * 2; ++i)
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
}

void AudioEngine::callback(void* userData, uint8_t* stream, int lengthBytes)
{
    auto* engine = static_cast<AudioEngine*>(userData);
    engine->mix(reinterpret_cast<float*>(stream), lengthBytes / static_cast<int>(sizeof(float) * 2));
}
