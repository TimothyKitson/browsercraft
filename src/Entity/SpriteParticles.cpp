#include "SpriteParticles.h"
#include "World/World.h"
#include "Renderer/Shader.h"
#include "Core/GLFunctions.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

namespace
{
    const std::vector<int> LAYOUT = { 3, 2, 1, 1, 1, 1 };
    constexpr int CELL = 16;
    constexpr int ACROSS = 32;
    constexpr int SIZE = CELL * ACROSS;

    struct KindDef
    {
        const char* name;
        int frames;
        glm::vec3 tint;
        bool animate;
        bool reverse;
        bool glow;
    };

    const KindDef KINDS[] = {
        { "explosion", 16, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "generic", 8, { 0.95f, 0.95f, 0.95f }, true, true, false },
        { "generic", 8, { 0.35f, 0.35f, 0.35f }, true, true, false },
        { "big_smoke", 12, { 0.75f, 0.75f, 0.75f }, true, false, false },
        { "flame", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "heart", 0, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "angry", 0, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "glint", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "critical_hit", 0, { 0.9f, 0.9f, 0.9f }, false, false, false },
        { "damage", 0, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "enchanted_hit", 0, { 0.6f, 0.4f, 1.0f }, false, false, true },
        { "note", 0, { 0.35f, 0.95f, 0.35f }, false, false, true },
        { "generic", 8, { 0.75f, 0.35f, 1.0f }, false, false, true },
        { "sga_", 26, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "bubble", 0, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "bubble_pop", 5, { 1.0f, 1.0f, 1.0f }, true, false, false },
        { "splash", 4, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "drip_fall", 0, { 0.25f, 0.35f, 1.0f }, false, false, false },
        { "drip_fall", 0, { 1.0f, 0.4f, 0.05f }, false, false, true },
        { "lava", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "effect", 8, { 1.0f, 1.0f, 1.0f }, true, true, false },
        { "spell", 8, { 1.0f, 1.0f, 1.0f }, true, true, false },
        { "sweep", 8, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "glitter", 8, { 0.95f, 0.9f, 0.35f }, true, true, true },
        { "gust", 12, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "small_gust", 7, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "sonic_boom", 16, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "trial_spawner_detection", 5, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "trial_spawner_detection_ominous", 5, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "vibration", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "shriek", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "sculk_soul", 11, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "sculk_charge", 7, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "sculk_charge_pop", 4, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "vault_connection", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "ominous_spawning", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "trial_omen", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "raid_omen", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "flash", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "spark", 8, { 1.0f, 1.0f, 1.0f }, true, true, true },
        { "glow", 0, { 0.5f, 1.0f, 0.9f }, false, false, true },
        { "soul", 11, { 1.0f, 1.0f, 1.0f }, true, false, true },
        { "soul_fire_flame", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "cherry", 12, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "nautilus", 0, { 1.0f, 1.0f, 1.0f }, false, false, true },
        { "infested", 0, { 1.0f, 1.0f, 1.0f }, false, false, false },
        { "goldheart", 3, { 1.0f, 1.0f, 1.0f }, true, false, false },
        { "drip_hang", 0, { 0.25f, 0.35f, 1.0f }, false, false, false },
        { "drip_hang", 0, { 1.0f, 0.4f, 0.05f }, false, false, true },
        { "drip_land", 0, { 1.0f, 0.4f, 0.05f }, false, false, true },
    };
    static_assert(sizeof(KINDS) / sizeof(KINDS[0]) == static_cast<size_t>(SpriteParticles::Kind::Count),
                  "one definition per kind");
}

SpriteParticles::SpriteParticles()
{
    buildAtlas();
}

SpriteParticles::~SpriteParticles()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

void SpriteParticles::buildAtlas()
{
    std::vector<unsigned char> atlas(static_cast<size_t>(SIZE) * SIZE * 4, 0);
    int next = 0;
    int found = 0;

    for (int kind = 0; kind < static_cast<int>(Kind::Count); ++kind)
    {
        const KindDef& def = KINDS[kind];
        Frames& frames = m_frames[kind];
        frames.first = next;
        frames.animate = def.animate;
        frames.reverse = def.reverse;
        frames.glow = def.glow;

        const int wanted = std::max(def.frames, 1);
        for (int f = 0; f < wanted && next < ACROSS * ACROSS; ++f)
        {
            std::string file = std::string("assets/textures/particle/") + def.name;
            if (def.frames > 0)
            {
                if (std::string(def.name) == "sga_") file += static_cast<char>('a' + f);
                else file += "_" + std::to_string(f);
            }
            file += ".png";

            int width = 0, height = 0, channels = 0;
            unsigned char* data = stbi_load(file.c_str(), &width, &height, &channels, 4);
            if (!data) continue;
            const int side = std::min(width, height);

            const int cellX = (next % ACROSS) * CELL;
            const int cellY = (next / ACROSS) * CELL;
            for (int y = 0; y < CELL; ++y)
                for (int x = 0; x < CELL; ++x)
                {
                    const int sx = x * side / CELL;
                    const int sy = y * side / CELL;
                    const unsigned char* p = data + (static_cast<size_t>(sy) * width + sx) * 4;
                    unsigned char* q = &atlas[(static_cast<size_t>(cellY + y) * SIZE + cellX + x) * 4];
                    q[0] = static_cast<unsigned char>(std::min(255.0f, p[0] * def.tint.r));
                    q[1] = static_cast<unsigned char>(std::min(255.0f, p[1] * def.tint.g));
                    q[2] = static_cast<unsigned char>(std::min(255.0f, p[2] * def.tint.b));
                    q[3] = p[3];
                }
            stbi_image_free(data);
            ++next;
            ++frames.count;
            ++found;
        }
    }

    if (found == 0) return;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    std::printf("Particles: %d sprite frames from assets/textures/particle\n", found);
}

float SpriteParticles::random()
{
    m_seed ^= m_seed << 13;
    m_seed ^= m_seed >> 17;
    m_seed ^= m_seed << 5;
    return static_cast<float>(m_seed & 0xFFFFFFu) / static_cast<float>(0x1000000);
}

void SpriteParticles::spawn(Kind kind, const glm::vec3& position, const glm::vec3& velocity,
                            float size, float lifetime, float gravity)
{
    if (!m_texture) return;
    const Frames& frames = m_frames[static_cast<int>(kind)];
    if (frames.count == 0) return;
    if (m_particles.size() >= 2400) m_particles.erase(m_particles.begin());

    Particle p;
    p.kind = kind;
    p.position = position;
    p.velocity = velocity;
    p.size = size;
    p.lifetime = std::max(0.05f, lifetime);
    p.gravity = gravity;
    p.frame = frames.animate ? 0 : static_cast<int>(random() * frames.count) % frames.count;
    m_particles.push_back(p);
}

void SpriteParticles::burst(Kind kind, const glm::vec3& position, int count, float spread, float speed,
                            float size, float lifetime, float gravity)
{
    for (int i = 0; i < count; ++i)
    {
        const glm::vec3 offset((random() - 0.5f) * 2.0f * spread, (random() - 0.5f) * 2.0f * spread,
                               (random() - 0.5f) * 2.0f * spread);
        const glm::vec3 direction = glm::length(offset) > 0.001f ? glm::normalize(offset) : glm::vec3(0, 1, 0);
        spawn(kind, position + offset, direction * speed * (0.5f + random()), size * (0.75f + random() * 0.5f),
              lifetime * (0.7f + random() * 0.6f), gravity);
    }
}

void SpriteParticles::drip(bool lava, const glm::vec3& position, float floorY)
{
    Particle p;
    p.kind = lava ? Kind::DripHangLava : Kind::DripHangWater;
    p.position = position;
    p.size = 0.14f;
    p.lifetime = lava ? 2.0f : 1.0f;
    p.floorY = floorY;
    m_particles.push_back(p);
}

void SpriteParticles::update(float deltaTime)
{
    for (Particle& p : m_particles)
    {
        p.age += deltaTime;
        if (p.kind == Kind::DripHangWater || p.kind == Kind::DripHangLava)
        {
            if (p.age >= p.lifetime)
            {
                p.kind = p.kind == Kind::DripHangLava ? Kind::DripLava : Kind::DripWater;
                p.age = 0.0f;
                p.lifetime = 6.0f;
                p.gravity = 9.0f;
            }
            continue;
        }
        p.velocity.y -= p.gravity * deltaTime;
        p.velocity *= std::pow(0.4f, deltaTime);
        p.position += p.velocity * deltaTime;
        if ((p.kind == Kind::DripWater || p.kind == Kind::DripLava) && p.position.y <= p.floorY)
        {
            p.kind = p.kind == Kind::DripLava ? Kind::DripLandLava : Kind::Splash;
            p.position.y = p.floorY + 0.03f;
            p.velocity = glm::vec3(0.0f);
            p.gravity = 0.0f;
            p.age = 0.0f;
            p.lifetime = p.kind == Kind::Splash ? 0.3f : 0.5f;
            p.frame = static_cast<int>(random() * 4.0f);
        }
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                     [](const Particle& p) { return p.age >= p.lifetime; }),
                      m_particles.end());
}

void SpriteParticles::render(Shader& shader, const World& world, const glm::vec3& cameraRight,
                             const glm::vec3& cameraUp, unsigned int restoreTexture)
{
    if (m_particles.empty() || !m_texture) return;

    m_vertices.clear();
    m_vertices.reserve(m_particles.size() * 6 * 9);
    const float cell = 1.0f / ACROSS;

    for (const Particle& p : m_particles)
    {
        const Frames& frames = m_frames[static_cast<int>(p.kind)];
        int frame = p.frame;
        if (frames.animate)
        {
            const float along = std::clamp(p.age / p.lifetime, 0.0f, 0.999f);
            frame = static_cast<int>(along * frames.count);
            if (frames.reverse) frame = frames.count - 1 - frame;
        }
        const int index = frames.first + std::clamp(frame, 0, frames.count - 1);
        const float u0 = static_cast<float>(index % ACROSS) * cell;
        const float v0 = static_cast<float>(index / ACROSS) * cell;
        const float u1 = u0 + cell;
        const float v1 = v0 + cell;

        const glm::vec3 right = cameraRight * (p.size * 0.5f);
        const glm::vec3 up = cameraUp * (p.size * 0.5f);
        const int lx = static_cast<int>(std::floor(p.position.x));
        const int ly = static_cast<int>(std::floor(p.position.y));
        const int lz = static_cast<int>(std::floor(p.position.z));
        const float sky = frames.glow ? 1.0f : world.skyLight(lx, ly, lz) / 15.0f;
        const float blockLight = frames.glow ? 1.0f : world.blockLightAt(lx, ly, lz) / 15.0f;

        const glm::vec3 corners[4] = { p.position - right - up, p.position + right - up,
                                       p.position + right + up, p.position - right + up };
        const float u[4] = { u0, u1, u1, u0 };
        const float v[4] = { v1, v1, v0, v0 };
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int k : order)
        {
            m_vertices.push_back(corners[k].x);
            m_vertices.push_back(corners[k].y);
            m_vertices.push_back(corners[k].z);
            m_vertices.push_back(u[k]);
            m_vertices.push_back(v[k]);
            m_vertices.push_back(1.0f);
            m_vertices.push_back(sky);
            m_vertices.push_back(blockLight);
            m_vertices.push_back(6.0f);
        }
    }

    m_mesh.upload(m_vertices, LAYOUT, true);
    shader.bind();
    shader.setVec3("uChunkOffset", glm::vec3(0.0f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glDisable(GL_CULL_FACE);
    m_mesh.draw(GL_TRIANGLES);
    glEnable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, restoreTexture);
}
