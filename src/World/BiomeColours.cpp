#include "BiomeColours.h"
#include "Noise.h"
#include "Core/Packs.h"
#include <stb_image.h>
#include <algorithm>
#include <mutex>
#include <vector>

namespace BiomeColours
{
    namespace
    {
        struct Map
        {
            int width = 0;
            int height = 0;
            std::vector<uint32_t> rgb;
        };

        struct Maps
        {
            bool loaded = false;
            Map grass;
            Map foliage;
        };

        std::mutex g_lock;

        Map load(const char* path)
        {
            Map map;
            int w = 0, h = 0, channels = 0;
            unsigned char* data = Packs::load(path, &w, &h, &channels, 4);
            if (!data) return map;
            map.width = w;
            map.height = h;
            map.rgb.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
            for (size_t i = 0; i < map.rgb.size(); ++i)
                map.rgb[i] = (static_cast<uint32_t>(data[i * 4]) << 16) | (static_cast<uint32_t>(data[i * 4 + 1]) << 8) | data[i * 4 + 2];
            stbi_image_free(data);
            return map;
        }

        Maps& maps()
        {
            static Maps instance;
            std::lock_guard<std::mutex> guard(g_lock);
            if (!instance.loaded)
            {
                instance.grass = load("assets/textures/colormap/grass.png");
                instance.foliage = load("assets/textures/colormap/foliage.png");
                instance.loaded = true;
            }
            return instance;
        }

        uint32_t mix(uint32_t a, uint32_t b, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            uint32_t out = 0;
            for (int shift = 0; shift <= 16; shift += 8)
            {
                const float ca = static_cast<float>((a >> shift) & 0xFFu);
                const float cb = static_cast<float>((b >> shift) & 0xFFu);
                out |= static_cast<uint32_t>(ca + (cb - ca) * t + 0.5f) << shift;
            }
            return out;
        }

        uint32_t painted(bool foliage, float temperature, float wetness)
        {
            const uint32_t cold = foliage ? 0x60A17Bu : 0x80B497u;
            const uint32_t dry = foliage ? 0xAEA42Au : 0xBFB755u;
            const uint32_t wet = foliage ? 0x1ABF00u : 0x47CD33u;
            const float share = temperature > 0.0f ? wetness / temperature : 0.0f;
            return mix(cold, mix(dry, wet, share), temperature);
        }

        const Noise& swampNoise()
        {
            static const Noise noise(2345u);
            return noise;
        }
    }

    uint32_t fromMap(bool foliage, float temperature, float downfall)
    {
        const float t = std::clamp(temperature, 0.0f, 1.0f);
        const float d = std::clamp(downfall, 0.0f, 1.0f) * t;
        const Map& map = foliage ? maps().foliage : maps().grass;
        if (map.width <= 0 || map.height <= 0) return painted(foliage, t, d);
        const int x = std::clamp(static_cast<int>((1.0f - t) * static_cast<float>(map.width - 1) * 255.0f / 255.0f), 0, map.width - 1);
        const int y = std::clamp(static_cast<int>((1.0f - d) * static_cast<float>(map.height - 1)), 0, map.height - 1);
        return map.rgb[static_cast<size_t>(y) * static_cast<size_t>(map.width) + static_cast<size_t>(x)];
    }

    uint32_t grass(Biomes::Kind kind, int x, int z)
    {
        const Biomes::Info& info = Biomes::info(kind);
        if (info.grassModifier == 2)
            return swampNoise().perlin2D(static_cast<float>(x) * 0.0225f, static_cast<float>(z) * 0.0225f) < -0.1f ? 0x4C763Cu : 0x6A7039u;
        uint32_t colour = info.grass >= 0 ? static_cast<uint32_t>(info.grass) : fromMap(false, info.temperature, info.downfall);
        if (info.grassModifier == 1) colour = ((colour & 0xFEFEFEu) + 0x28340Au) >> 1;
        return colour;
    }

    uint32_t foliage(Biomes::Kind kind)
    {
        const Biomes::Info& info = Biomes::info(kind);
        return info.foliage >= 0 ? static_cast<uint32_t>(info.foliage) : fromMap(true, info.temperature, info.downfall);
    }

    uint32_t water(Biomes::Kind kind)
    {
        return Biomes::info(kind).water;
    }

    uint32_t average(const uint32_t* colours, int count)
    {
        if (count <= 0) return 0;
        uint32_t r = 0, g = 0, b = 0;
        for (int i = 0; i < count; ++i)
        {
            r += (colours[i] >> 16) & 0xFFu;
            g += (colours[i] >> 8) & 0xFFu;
            b += colours[i] & 0xFFu;
        }
        const uint32_t n = static_cast<uint32_t>(count);
        return ((r / n) << 16) | ((g / n) << 8) | (b / n);
    }

    void reload()
    {
        Maps& m = maps();
        std::lock_guard<std::mutex> guard(g_lock);
        m.loaded = false;
    }

    bool fromPack()
    {
        return maps().grass.width > 0;
    }
}
