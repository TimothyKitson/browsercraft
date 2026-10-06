#include "SoundSystem.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr float MIN_GAIN = 0.0015f; // below this a voice is not worth starting
}

SoundSystem::SoundSystem()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        std::fprintf(stderr, "[Audio] SDL audio unavailable: %s\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec want{};
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = &SoundSystem::audioCallback;
    want.userdata = this;

    SDL_AudioSpec got{};
    m_device = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
    if (m_device == 0)
    {
        std::fprintf(stderr, "[Audio] could not open a device: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(m_device, 0);
    std::printf("[Audio] %d Hz, %d channels\n", got.freq, got.channels);
}

SoundSystem::~SoundSystem()
{
    if (m_device) SDL_CloseAudioDevice(m_device);
}

void SoundSystem::setListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& right)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listenerPosition = position;
    m_listenerForward = forward;
    m_listenerRight = right;
}

const SoundSystem::Clip* SoundSystem::clipFor(const std::string& path)
{
    auto existing = m_clips.find(path);
    if (existing != m_clips.end())
        return existing->second.loaded ? &existing->second : nullptr;

    Clip clip;

    int channels = 0;
    int rate = 0;
    short* decoded = nullptr;
    const int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &decoded);

    if (frames > 0 && decoded && channels > 0)
    {
        // Normalise to interleaved stereo at the mixer's rate. Resampling is a
        // plain linear walk: these are short effects, not music.
        const double ratio = static_cast<double>(rate) / SAMPLE_RATE;
        const int outFrames = static_cast<int>(frames / ratio);
        clip.samples.resize(static_cast<size_t>(outFrames) * 2);

        for (int i = 0; i < outFrames; ++i)
        {
            const double source = i * ratio;
            const int index = static_cast<int>(source);
            const int next = std::min(index + 1, frames - 1);
            const float blend = static_cast<float>(source - index);

            for (int c = 0; c < 2; ++c)
            {
                const int sourceChannel = (channels == 1) ? 0 : c;
                const float a = decoded[static_cast<size_t>(index) * channels + sourceChannel];
                const float b = decoded[static_cast<size_t>(next) * channels + sourceChannel];
                clip.samples[static_cast<size_t>(i) * 2 + c] =
                    static_cast<int16_t>(a + (b - a) * blend);
            }
        }
        clip.loaded = true;
    }

    if (decoded) free(decoded);

    auto inserted = m_clips.emplace(path, std::move(clip)).first;
    return inserted->second.loaded ? &inserted->second : nullptr;
}

void SoundSystem::queue(const Clip* clip, float leftGain, float rightGain, float pitch)
{
    if (!clip || clip->samples.empty()) return;

    // Steal the voice that is closest to finishing if every slot is busy, so a
    // burst of noise never silences itself.
    int chosen = -1;
    double mostProgressed = -1.0;
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (!m_voices[i].active) { chosen = i; break; }
        const double progress = m_voices[i].cursor / static_cast<double>(m_voices[i].clip->samples.size() / 2);
        if (progress > mostProgressed) { mostProgressed = progress; chosen = i; }
    }
    if (chosen < 0) return;

    Voice& voice = m_voices[chosen];
    voice.clip = clip;
    voice.cursor = 0.0;
    voice.step = std::max(0.25, std::min(4.0, static_cast<double>(pitch)));
    voice.leftGain = leftGain;
    voice.rightGain = rightGain;
    voice.active = true;
}

void SoundSystem::play(const std::string& name, int variants, const glm::vec3& position,
                       float volume, float pitch)
{
    if (!m_device || m_muted || name.empty() || variants <= 0) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    const glm::vec3 toSource = position - m_listenerPosition;
    const float distance = glm::length(toSource);
    if (distance > HEARING_RANGE) return;

    // Linear roll-off: simple, predictable, and matches how Minecraft feels.
    float gain = volume * (1.0f - distance / HEARING_RANGE);
    if (gain < MIN_GAIN) return;

    // Pan by which side the source sits on, easing off as it gets close so a
    // sound at your feet is not hard-panned.
    float pan = 0.0f;
    if (distance > 0.1f)
        pan = glm::dot(toSource / distance, m_listenerRight) * std::min(1.0f, distance / 3.0f);

    const float left = gain * std::sqrt(std::max(0.0f, 0.5f * (1.0f - pan)));
    const float right = gain * std::sqrt(std::max(0.0f, 0.5f * (1.0f + pan)));

    m_rng ^= m_rng << 13; m_rng ^= m_rng >> 17; m_rng ^= m_rng << 5;
    const int variant = 1 + static_cast<int>(m_rng % static_cast<uint32_t>(variants));

    queue(clipFor("assets/sounds/" + name + std::to_string(variant) + ".ogg"), left, right, pitch);
}

void SoundSystem::playGlobal(const std::string& name, int variants, float volume, float pitch)
{
    if (!m_device || m_muted || name.empty() || variants <= 0) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_rng ^= m_rng << 13; m_rng ^= m_rng >> 17; m_rng ^= m_rng << 5;
    const int variant = 1 + static_cast<int>(m_rng % static_cast<uint32_t>(variants));

    const float gain = volume * 0.7071f;
    queue(clipFor("assets/sounds/" + name + std::to_string(variant) + ".ogg"), gain, gain, pitch);
}

void SoundSystem::audioCallback(void* userData, uint8_t* stream, int length)
{
    auto* self = static_cast<SoundSystem*>(userData);
    std::memset(stream, 0, static_cast<size_t>(length));
    self->mix(reinterpret_cast<int16_t*>(stream), length / (2 * static_cast<int>(sizeof(int16_t))));
}

void SoundSystem::mix(int16_t* output, int frames)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (Voice& voice : m_voices)
    {
        if (!voice.active || !voice.clip) continue;

        const std::vector<int16_t>& samples = voice.clip->samples;
        const size_t available = samples.size() / 2;

        for (int frame = 0; frame < frames; ++frame)
        {
            const size_t index = static_cast<size_t>(voice.cursor);
            if (index >= available) { voice.active = false; break; }

            const float left = samples[index * 2] * voice.leftGain;
            const float right = samples[index * 2 + 1] * voice.rightGain;

            int mixedLeft = output[frame * 2] + static_cast<int>(left);
            int mixedRight = output[frame * 2 + 1] + static_cast<int>(right);

            output[frame * 2] = static_cast<int16_t>(std::clamp(mixedLeft, -32768, 32767));
            output[frame * 2 + 1] = static_cast<int16_t>(std::clamp(mixedRight, -32768, 32767));

            voice.cursor += voice.step;
        }
    }
}
