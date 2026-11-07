#pragma once
#include "Renderer/Mesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class World;
class Shader;

class SpriteParticles
{
public:
    enum class Kind
    {
        Explosion, Poof, Smoke, BigSmoke, Flame, Heart, Angry, Happy, Crit, Damage,
        EnchantedHit, Note, Portal, Enchant, Bubble, BubblePop, Splash, DripWater, DripLava,
        Lava, Effect, Spell, Sweep, Glitter, Gust, SmallGust, SonicBoom, TrialFlame,
        TrialFlameOminous, Vibration, Shriek, SculkSoul, SculkCharge, SculkChargePop,
        VaultConnection, OminousSpawning, TrialOmen, RaidOmen, Flash, Spark, Glow,
        Soul, SoulFlame, Cherry, Nautilus, Infested, GoldHeart,
        DripHangWater, DripHangLava, DripLandLava, Ash, WhiteAsh, CrimsonSpore, WarpedSpore, Count
    };

    SpriteParticles();
    ~SpriteParticles();

    bool loaded() const { return m_texture != 0; }

    void spawn(Kind kind, const glm::vec3& position, const glm::vec3& velocity,
               float size, float lifetime, float gravity = 0.0f);
    void burst(Kind kind, const glm::vec3& position, int count, float spread, float speed,
               float size, float lifetime, float gravity = 0.0f);

    void drip(bool lava, const glm::vec3& position, float floorY);

    void update(float deltaTime);
    void render(Shader& shader, const World& world, const glm::vec3& cameraRight,
                const glm::vec3& cameraUp, unsigned int restoreTexture);

    void clear() { m_particles.clear(); }
    int count() const { return static_cast<int>(m_particles.size()); }
    float random();

private:
    struct Particle
    {
        Kind kind = Kind::Poof;
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        float size = 0.1f;
        float age = 0.0f;
        float lifetime = 1.0f;
        float gravity = 0.0f;
        float floorY = -1.0e9f;
        int frame = 0;
    };

    struct Frames
    {
        int first = 0;
        int count = 0;
        bool animate = false;
        bool reverse = false;
        bool glow = false;
    };

    unsigned int m_texture = 0;
    Frames m_frames[static_cast<int>(Kind::Count)];
    std::vector<Particle> m_particles;
    std::vector<float> m_vertices;
    Mesh m_mesh;
    uint32_t m_seed = 0x9E3779B9u;

    void buildAtlas();
};
