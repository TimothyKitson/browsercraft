#pragma once
#include <array>
#include <cstdint>

// Classic Perlin gradient noise, seeded by a permutation table. Pure and
// const, so world-generation worker threads can share one instance.
class Noise
{
public:
    explicit Noise(uint32_t seed);

    float perlin2D(float x, float y) const;
    float perlin3D(float x, float y, float z) const;

    // Sums octaves at doubling frequency / halving amplitude.
    float fbm2D(float x, float y, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const;
    float fbm3D(float x, float y, float z, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const;

    // |fbm|-style ridged noise: good for mountain spines and cave tunnels.
    float ridged2D(float x, float y, int octaves) const;

private:
    std::array<int, 512> m_perm{};

    float gradient2D(int hash, float x, float y) const;
    float gradient3D(int hash, float x, float y, float z) const;
};

// Deterministic hash helpers for scattering features (trees, ores) without
// needing a noise lookup.
uint32_t hashCoords(int x, int y, int z, uint32_t seed);
float hashToFloat(uint32_t h);
