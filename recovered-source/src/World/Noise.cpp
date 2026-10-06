#include "Noise.h"
#include <numeric>
#include <cmath>
#include <random>

namespace
{
    float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    float lerp(float a, float b, float t) { return a + (b - a) * t; }
}

Noise::Noise(uint32_t seed)
{
    std::array<int, 256> p{};
    std::iota(p.begin(), p.end(), 0);

    std::mt19937 rng(seed);
    for (int i = 255; i > 0; --i)
    {
        std::uniform_int_distribution<int> dist(0, i);
        std::swap(p[i], p[dist(rng)]);
    }

    for (int i = 0; i < 512; ++i)
        m_perm[i] = p[i & 255];
}

float Noise::gradient2D(int hash, float x, float y) const
{
    switch (hash & 7)
    {
        case 0: return x + y;
        case 1: return -x + y;
        case 2: return x - y;
        case 3: return -x - y;
        case 4: return x;
        case 5: return -x;
        case 6: return y;
        default: return -y;
    }
}

float Noise::gradient3D(int hash, float x, float y, float z) const
{
    switch (hash & 15)
    {
        case 0: case 12: return x + y;
        case 1: case 13: return -x + y;
        case 2: return x - y;
        case 3: return -x - y;
        case 4: return x + z;
        case 5: return -x + z;
        case 6: return x - z;
        case 7: return -x - z;
        case 8: return y + z;
        case 9: case 14: return -y + z;
        case 10: return y - z;
        default: return -y - z;
    }
}

float Noise::perlin2D(float x, float y) const
{
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = m_perm[m_perm[xi] + yi];
    int ab = m_perm[m_perm[xi] + yi + 1];
    int ba = m_perm[m_perm[xi + 1] + yi];
    int bb = m_perm[m_perm[xi + 1] + yi + 1];

    float x1 = lerp(gradient2D(aa, xf, yf), gradient2D(ba, xf - 1.0f, yf), u);
    float x2 = lerp(gradient2D(ab, xf, yf - 1.0f), gradient2D(bb, xf - 1.0f, yf - 1.0f), u);
    return lerp(x1, x2, v); // roughly [-1, 1]
}

float Noise::perlin3D(float x, float y, float z) const
{
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int a = m_perm[xi] + yi;
    int aa = m_perm[a] + zi;
    int ab = m_perm[a + 1] + zi;
    int b = m_perm[xi + 1] + yi;
    int ba = m_perm[b] + zi;
    int bb = m_perm[b + 1] + zi;

    float x1 = lerp(gradient3D(m_perm[aa], xf, yf, zf),
                    gradient3D(m_perm[ba], xf - 1.0f, yf, zf), u);
    float x2 = lerp(gradient3D(m_perm[ab], xf, yf - 1.0f, zf),
                    gradient3D(m_perm[bb], xf - 1.0f, yf - 1.0f, zf), u);
    float y1 = lerp(x1, x2, v);

    float x3 = lerp(gradient3D(m_perm[aa + 1], xf, yf, zf - 1.0f),
                    gradient3D(m_perm[ba + 1], xf - 1.0f, yf, zf - 1.0f), u);
    float x4 = lerp(gradient3D(m_perm[ab + 1], xf, yf - 1.0f, zf - 1.0f),
                    gradient3D(m_perm[bb + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
    float y2 = lerp(x3, x4, v);

    return lerp(y1, y2, w);
}

float Noise::fbm2D(float x, float y, int octaves, float lacunarity, float gain) const
{
    float sum = 0.0f, amplitude = 1.0f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        sum += perlin2D(x * frequency, y * frequency) * amplitude;
        norm += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

float Noise::fbm3D(float x, float y, float z, int octaves, float lacunarity, float gain) const
{
    float sum = 0.0f, amplitude = 1.0f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        sum += perlin3D(x * frequency, y * frequency, z * frequency) * amplitude;
        norm += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

float Noise::ridged2D(float x, float y, int octaves) const
{
    float sum = 0.0f, amplitude = 1.0f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        float n = 1.0f - std::fabs(perlin2D(x * frequency, y * frequency));
        sum += n * n * amplitude;
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return norm > 0.0f ? sum / norm : 0.0f; // [0, 1]
}

uint32_t hashCoords(int x, int y, int z, uint32_t seed)
{
    uint32_t h = seed * 2654435761u;
    h ^= static_cast<uint32_t>(x) * 374761393u;
    h ^= static_cast<uint32_t>(y) * 668265263u;
    h ^= static_cast<uint32_t>(z) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float hashToFloat(uint32_t h)
{
    return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0x1000000);
}
