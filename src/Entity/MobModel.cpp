#include "MobModel.h"
#include "World/WorldGen.h"
#include <stb_image.h>
#include "BoxMesh.h"
#include "EntityManager.h"
#include "Mob.h"
#include "PlayerAnimation.h"
#include "Core/GLFunctions.h"
#include "Renderer/Shader.h"
#include "World/World.h"
#include "Game/Trading.h"
#include "Game/Items.h"
#include "Game/Banners.h"
#include <cstdio>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "Core/Packs.h"

namespace
{
    const std::vector<int> CHUNK_LAYOUT = { 3, 2, 1, 1, 1, 1 };

    // How far a sitting animal drops, in blocks. A wolf is 0.85 tall,
    // so a fifth of that reads as sitting without burying its paws.
    constexpr float SIT_DROP = 0.17f;

    // How far a limb swings at a full run, in radians. Less than the
    // player's: four short legs covering the same ground do not need to
    // reach as far as two long ones.
    constexpr float LEG_SWING = 0.9f;

    // A quadruped's diagonal pairs swing together, the way a real animal
    // walks. For the bipeds the arms are filed as the front pair, so the
    // same rule gives them an arm swinging opposite each leg.
    float swingFor(Part part, float phase, float amount)
    {
        switch (part)
        {
            case Part::LegFrontLeft:
            case Part::LegBackRight:
                return std::cos(phase) * LEG_SWING * amount;
            case Part::LegFrontRight:
            case Part::LegBackLeft:
                return std::cos(phase + PlayerAnimation::PI) * LEG_SWING * amount;
            default:
                return 0.0f;
        }
    }

    // Wings beat out from the body rather than back and forth, so a
    // chicken's pair turn about the way it is facing instead.
    float flapFor(Part part, float phase, float amount)
    {
        const float beat = (std::cos(phase) * 0.5f + 0.5f) * amount;
        if (part == Part::WingRight) return -beat;
        if (part == Part::WingLeft) return beat;
        return 0.0f;
    }
}

std::vector<std::pair<unsigned int, std::vector<float>>> MobModel::figure(const Mob& mob)
{
    std::vector<std::pair<unsigned int, std::vector<float>>> layers;
    const MobSkin& skin = skinFor(mob.typeId());
    if (!skin.textureId()) return layers;
    build(skin, mob, 1.0f, 1.0f, 0);
    unsigned int base = skin.textureId();
    const std::string path = variantOf(mob);
    if (!path.empty())
        if (const unsigned int own = villagerTexture(path.c_str())) base = own;
    layers.emplace_back(base, m_vertices);
    if (skin.overlayTextureId())
    {
        build(skin, mob, 1.0f, 1.0f, 1);
        layers.emplace_back(skin.overlayTextureId(), m_vertices);
    }
    const std::vector<std::string> coats = coatOverlaysOf(mob);
    if (!coats.empty())
    {
        build(skin, mob, 1.0f, 1.0f, 0, 0.0f);
        for (const std::string& coat : coats)
            if (const unsigned int texture = villagerTexture(coat.c_str())) layers.emplace_back(texture, m_vertices);
    }
    return layers;
}

const MobSkin& MobModel::skinFor(MobId id)
{
    if (m_skins.empty()) m_skins.resize(static_cast<size_t>(MobId::Count));

    const size_t index = static_cast<size_t>(id);
    if (!m_skins[index])
    {
        m_skins[index] = std::make_unique<MobSkin>();
        m_skins[index]->build(id);
    }

    return *m_skins[index];
}

void MobModel::build(const MobSkin& skin, const Mob& mob, float sky, float blockLight, int layer,
                     float extraInflate)
{
    const MobType& type = mob.type();

    BoxMesh::Frame frame;
    frame.feet = mob.position() + m_shift - glm::vec3(0.0f, mob.burrowDepth(), 0.0f);
    frame.bodyYaw = mob.yaw();
    frame.sky = sky;
    frame.blockLight = blockLight;
    frame.textureWidth = static_cast<float>(skin.width());
    frame.textureHeight = static_cast<float>(skin.height());
    // A lamb is half a sheep. Shrinking the whole figure here rather than
    // each box keeps every texture patch the size the sheet says it is.
    frame.scale = mob.scale();
    frame.modelYaw = type.modelYaw;

    // Sitting drops the whole animal onto its haunches. Lowering the
    // figure rather than re-posing each leg keeps every texture patch
    // the size the sheet says it is, which is the same reason a lamb is
    // scaled rather than rebuilt.
    if (mob.sitting()) frame.feet.y -= SIT_DROP * mob.scale();

    // A dead one keels over sideways rather than vanishing mid-step.
    const float fade = mob.deathFade();
    if (fade < 1.0f)
    {
        frame.jointAngle = 0.0f;
        frame.feet.y -= (1.0f - fade) * 0.15f;
    }

    const float phase = mob.gait();
    const float amount = mob.gaitAmount();

    // A shulker's lid, which is the one part in the game that moves by
    // sliding rather than swinging. Mojang gives an open one a slow bob
    // where it sits; the clock is the mob's own age, since a thing that
    // never walks has no stride to hang an animation off.
    const float peek = mob.peek();
    const float lidLift = Shulker::lidLift(peek) -
                          (peek > 0.5f ? std::sin(mob.age() * 2.0f) * 0.7f : 0.0f);
    const float lidTwist = Shulker::lidTwist(peek);

    std::vector<BoxMesh::Box> boxes;
    boxes.reserve(skin.boxes().size());

    for (const MobBox& source : skin.boxes())
    {
        if (source.layer != layer) continue;
        if (source.part == Part::Shell && !mob.rolled()) continue;
        if (mob.typeId() == MobId::Armadillo && mob.rolled() && source.part != Part::Shell) continue;

        BoxMesh::Box box;
        box.min = glm::vec3(source.origin);
        box.max = glm::vec3(source.origin + source.size);
        box.pivot = source.pivot;
        box.u = source.u;
        box.v = source.v;
        box.mirror = source.mirror;
        box.inflate = source.inflate + extraInflate;

        // The rest pose first -- a spider's legs are already splayed
        // before it takes a step -- and the walk on top of it.
        box.pitch = source.pitch + swingFor(source.part, phase, amount) * type.limbSwing;
        box.yaw = source.yaw;
        box.roll = source.roll + flapFor(source.part, phase, amount);

        if (source.part == Part::Lid)
        {
            box.min.y += lidLift;
            box.max.y += lidLift;
            box.yaw += lidTwist;
        }

        boxes.push_back(box);
    }

    m_vertices.clear();
    BoxMesh::appendAll(m_vertices, boxes, frame);
    if (mob.customName() == "Dinnerbone" || mob.customName() == "Grumm")
    {
        const glm::vec3 centre = frame.feet + glm::vec3(0.0f, mob.height() * 0.5f, 0.0f);
        for (size_t i = 0; i + 9 <= m_vertices.size(); i += 9)
        {
            m_vertices[i + 1] = 2.0f * centre.y - m_vertices[i + 1];
            m_vertices[i + 2] = 2.0f * centre.z - m_vertices[i + 2];
        }
    }
}

void MobModel::render(Shader& chunkShader, const World& world, const EntityManager& entities)
{
    if (entities.mobs().empty()) return;

    chunkShader.bind();
    chunkShader.setVec3("uChunkOffset", glm::vec3(0.0f)); // already in world space

    // One pass per species, so each hide is bound once however many of
    // them are wandering about.
    for (int speciesIndex = 0; speciesIndex < mobTypeCount(); ++speciesIndex)
    {
        const MobId id = static_cast<MobId>(speciesIndex);

        bool any = false;
        for (const Mob& mob : entities.mobs())
            if (mob.typeId() == id) { any = true; break; }
        if (!any) continue;

        const MobSkin& skin = skinFor(id);
        if (!skin.textureId()) continue;

        // A sheep is its hide and then its wool, each off its own sheet,
        // so a species can take two passes.
        for (int layer = 0; layer < 2; ++layer)
        {
        const unsigned int texture = layer == 0 ? skin.textureId() : skin.overlayTextureId();
        if (!texture) continue;

        std::vector<float> batch;
        std::vector<std::pair<unsigned int, std::vector<float>>> variants;

        for (const Mob& mob : entities.mobs())
        {
            if (mob.typeId() != id || mob.invisible() || (hiddenSerial != 0 && mob.serial() == hiddenSerial)) continue;

            // One light sample per mob, taken at its middle and nudged
            // clear of anything solid -- read inside its own body, or
            // inside the floor, it comes out almost black.
            const glm::vec3 centre = mob.position();
            int lx = static_cast<int>(std::floor(centre.x));
            int ly = static_cast<int>(std::floor(centre.y + mob.height() * 0.5f));
            int lz = static_cast<int>(std::floor(centre.z));
            for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step)
                ++ly;

            float sky = static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f;
            float blockLight = static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f;

            // Flash while it is being hit, and dim as it dies.
            blockLight = std::min(1.0f, blockLight + mob.hurtFlash() * 1.6f +
                                            (mob.burning() ? 0.9f : 0.0f));
            if (mob.glowing())
            {
                sky = 1.0f;
                blockLight = 1.0f;
            }
            const float selfLit = mob.glowLight();
            if (selfLit > 0.0f) blockLight = selfLit >= 1.0f ? 1.0f : std::max(blockLight, selfLit);
            const float fade = mob.deathFade();
            sky *= fade;
            blockLight *= fade;

            build(skin, mob, sky, blockLight, layer);
            if (mob.illusioned())
            {
                std::vector<float> copies;
                const float time = mob.age();
                for (int k = 0; k < 4; ++k)
                {
                    const float j = static_cast<float>(k);
                    m_shift = mob.mirrorOffset(k) + glm::vec3(std::cos(j + time * 10.0f) * 0.025f,
                                                              std::cos(j + time * 15.0f) * 0.0125f,
                                                              std::cos(j + time * 14.0f) * 0.025f);
                    build(skin, mob, sky, blockLight, layer);
                    copies.insert(copies.end(), m_vertices.begin(), m_vertices.end());
                }
                m_shift = glm::vec3(0.0f);
                m_vertices.swap(copies);
            }

            unsigned int own = 0;
            if (layer == 0)
            {
                const std::string path = variantOf(mob);
                if (!path.empty()) own = villagerTexture(path.c_str());
            }
            if (layer == 1 && id == MobId::Sheep)
            {
                if (mob.sheared()) continue;
                const int shade = mob.customName() == "jeb_" ? static_cast<int>(mob.age() * 4.0f) % 16 : mob.woolColour();
                if (shade != 0)
                {
                    char tint[8];
                    std::snprintf(tint, sizeof(tint), "%06X", Banners::dyeRgb(shade));
                    own = villagerTexture((std::string("assets/textures/entity/sheep/sheep_fur.png#") + tint).c_str());
                }
            }
            if (own)
            {
                auto found = std::find_if(variants.begin(), variants.end(),
                                          [&](const auto& v) { return v.first == own; });
                if (found == variants.end())
                {
                    variants.emplace_back(own, std::vector<float>());
                    found = variants.end() - 1;
                }
                found->second.insert(found->second.end(), m_vertices.begin(), m_vertices.end());
            }
            else
            {
                batch.insert(batch.end(), m_vertices.begin(), m_vertices.end());
            }
        }

        if (!batch.empty())
        {
            m_mesh.upload(batch, CHUNK_LAYOUT, true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            m_mesh.draw(GL_TRIANGLES);
        }
        for (const auto& variant : variants)
        {
            m_mesh.upload(variant.second, CHUNK_LAYOUT, true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, variant.first);
            m_mesh.draw(GL_TRIANGLES);
        }
        }

        if (id == MobId::Villager || id == MobId::ZombieVillager)
        {
            const std::string root = id == MobId::Villager ? "assets/textures/entity/villager/"
                                                           : "assets/textures/entity/zombie_villager/";
            static const char* BIOME_NAMES[] = { "plains", "desert", "jungle", "savanna", "snow", "swamp", "taiga" };
            static const char* RANK_NAMES[] = { "stone", "iron", "gold", "emerald", "diamond" };
            const int jobCount = static_cast<int>(Trading::Profession::Count);
            auto biomeIndex = [&](const Mob& mob) {
                const glm::vec3 at = mob.position();
                switch (world.generator().biomeAt(static_cast<int>(std::floor(at.x)), static_cast<int>(std::floor(at.z))))
                {
                    case Biome::Desert: return 1;
                    case Biome::Jungle: return 2;
                    case Biome::Savanna: return 3;
                    case Biome::Snowy: return 4;
                    case Biome::Swamp: return 5;
                    case Biome::Mountains:
                    case Biome::Forest: return 6;
                    default: return 0;
                }
            };

            glDepthFunc(GL_LEQUAL);
            for (int pass = 0; pass < 3; ++pass)
            {
                const int options = pass == 0 ? 7 : pass == 1 ? jobCount : 5;
                for (int option = 0; option < options; ++option)
                {
                    const std::string path =
                        pass == 0 ? root + "type/" + BIOME_NAMES[option] + ".png"
                      : pass == 1 ? root + "profession/" +
                                        Trading::textureName(static_cast<Trading::Profession>(option)) + ".png"
                                  : root + "profession_level/" + RANK_NAMES[option] + ".png";
                    const unsigned int overlay = villagerTexture(path.c_str());
                    if (!overlay) continue;
                    std::vector<float> batch;
                    for (const Mob& mob : entities.mobs())
                    {
                        if (mob.typeId() != id || mob.baby()) continue;
                        const int job = professionOf ? professionOf(mob) : 0;
                        if (pass == 2 && job == static_cast<int>(Trading::Profession::Nitwit)) continue;
                        const int which = pass == 0 ? biomeIndex(mob)
                                        : pass == 1 ? job
                                                    : (rankOf ? rankOf(mob) : 0);
                        if (which != option) continue;
                        const glm::vec3 centre = mob.position();
                        int lx = static_cast<int>(std::floor(centre.x));
                        int ly = static_cast<int>(std::floor(centre.y + mob.height() * 0.5f));
                        int lz = static_cast<int>(std::floor(centre.z));
                        for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step) ++ly;
                        const float fade = mob.deathFade();
                        build(skin, mob, static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f * fade,
                              std::min(1.0f, static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f +
                                                 mob.hurtFlash() * 1.6f) * fade, 0);
                        batch.insert(batch.end(), m_vertices.begin(), m_vertices.end());
                    }
                    if (batch.empty()) continue;
                    m_mesh.upload(batch, CHUNK_LAYOUT, true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, overlay);
                    m_mesh.draw(GL_TRIANGLES);
                }
            }
            glDepthFunc(GL_LESS);
        }

        renderExtras(id, skin, world, entities);
    }
}

void MobModel::renderExtras(MobId id, const MobSkin& skin, const World& world, const EntityManager& entities)
{
    struct Extra
    {
        MobId id;
        const char* path;
        float inflate;
    };
    static const Extra EXTRAS[] = {
        { MobId::Enderman, "assets/textures/entity/enderman/enderman_eyes.png", 0.0f },
        { MobId::Spider, "assets/textures/entity/spider_eyes.png", 0.0f },
        { MobId::CaveSpider, "assets/textures/entity/spider_eyes.png", 0.0f },
        { MobId::Phantom, "assets/textures/entity/phantom_eyes.png", 0.0f },
        { MobId::EnderDragon, "assets/textures/entity/enderdragon/dragon_eyes.png", 0.0f },
        { MobId::EnderDragon, "assets/textures/entity/enderdragon/dragon_exploding.png", 0.0f },
        { MobId::Warden, "assets/textures/entity/warden/warden_bioluminescent_layer.png", 0.0f },
        { MobId::Warden, "assets/textures/entity/warden/warden_pulsating_spots_1.png", 0.0f },
        { MobId::Warden, "assets/textures/entity/warden/warden_pulsating_spots_2.png", 0.0f },
        { MobId::Warden, "assets/textures/entity/warden/warden_heart.png", 0.0f },
        { MobId::IronGolem, "assets/textures/entity/iron_golem/iron_golem_crackiness_low.png", 0.0f },
        { MobId::IronGolem, "assets/textures/entity/iron_golem/iron_golem_crackiness_medium.png", 0.0f },
        { MobId::IronGolem, "assets/textures/entity/iron_golem/iron_golem_crackiness_high.png", 0.0f },
        { MobId::Wither, "assets/textures/entity/wither/wither_invulnerable.png", 0.0f },
        { MobId::Wither, "assets/textures/entity/wither/wither_armor.png", 0.5f },
        { MobId::Ghast, "assets/textures/entity/ghast/ghast_shooting.png", 0.0f },
        { MobId::Breeze, "assets/textures/entity/breeze/breeze_eyes.png", 0.0f },
        { MobId::Drowned, "assets/textures/entity/zombie/drowned_outer_layer.png", 0.25f },
        { MobId::Stray, "assets/textures/entity/skeleton/stray_overlay.png", 0.25f },
        { MobId::Bogged, "assets/textures/entity/skeleton/bogged_overlay.png", 0.25f },
        { MobId::Vex, "assets/textures/entity/illager/vex_charging.png", 0.0f },
        { MobId::Strider, "assets/textures/entity/strider/strider_cold.png", 0.0f },
        { MobId::Creeper, "assets/textures/entity/creeper/creeper_armor.png", 0.5f },
    };

    auto lit = [&](const Mob& mob, float& sky, float& blockLight) {
        const glm::vec3 centre = mob.position();
        const int lx = static_cast<int>(std::floor(centre.x));
        int ly = static_cast<int>(std::floor(centre.y + mob.height() * 0.5f));
        const int lz = static_cast<int>(std::floor(centre.z));
        for (int step = 0; step < 3 && isOpaque(world.getBlock(lx, ly, lz)); ++step) ++ly;
        const float fade = mob.deathFade();
        sky = static_cast<float>(world.skyLight(lx, ly, lz)) / 15.0f * fade;
        blockLight = std::min(1.0f, static_cast<float>(world.blockLightAt(lx, ly, lz)) / 15.0f +
                                        mob.hurtFlash() * 1.6f) * fade;
    };

    auto glowFor = [&](const char* path, const Mob& mob) -> float {
        const std::string p(path);
        const float health = static_cast<float>(mob.health()) / std::max(1, mob.type().maxHealth);
        if (p.find("exploding") != std::string::npos) return mob.alive() ? -1.0f : 1.0f;
        if (p.find("_eyes") != std::string::npos || p.find("bioluminescent") != std::string::npos)
            return mob.alive() ? 1.0f : -1.0f;
        if (p.find("spots_1") != std::string::npos)
        {
            const float pulse = std::cos(mob.age() * 0.9f);
            return pulse > 0.3f ? 0.6f + 0.4f * pulse : -1.0f;
        }
        if (p.find("spots_2") != std::string::npos)
        {
            const float pulse = std::cos(mob.age() * 0.9f + 3.14159f);
            return pulse > 0.3f ? 0.6f + 0.4f * pulse : -1.0f;
        }
        if (p.find("heart") != std::string::npos)
        {
            const float beat = std::sin(mob.age() * (mob.hostileNow() ? 9.0f : 5.0f));
            return beat > 0.2f ? 0.5f + 0.5f * beat : -1.0f;
        }
        if (p.find("crackiness_low") != std::string::npos) return health < 0.75f && health >= 0.5f ? 0.0f : -1.0f;
        if (p.find("crackiness_medium") != std::string::npos) return health < 0.5f && health >= 0.25f ? 0.0f : -1.0f;
        if (p.find("crackiness_high") != std::string::npos) return health < 0.25f ? 0.0f : -1.0f;
        if (p.find("invulnerable") != std::string::npos)
            return mob.spawnPhase() > 0.0f && (mob.spawnPhase() > 2.0f || std::fmod(mob.spawnPhase(), 0.5f) < 0.25f)
                       ? 0.0f : -1.0f;
        if (p.find("creeper_armor") != std::string::npos) return mob.charged() ? 1.0f : -1.0f;
        if (p.find("wither_armor") != std::string::npos) return health <= 0.5f && mob.spawnPhase() <= 0.0f ? 1.0f : -1.0f;
        if (p.find("shooting") != std::string::npos) return mob.aboutToShoot() ? 0.0f : -1.0f;
        if (p.find("charging") != std::string::npos) return mob.hostileNow() && mob.lastRange() < 6.0f ? 0.0f : -1.0f;
        if (p.find("strider_cold") != std::string::npos)
        {
            const glm::vec3 at = mob.position();
            const int x = static_cast<int>(std::floor(at.x));
            const int y = static_cast<int>(std::floor(at.y));
            const int z = static_cast<int>(std::floor(at.z));
            const bool warm = isLava(world.getBlock(x, y, z)) || isLava(world.getBlock(x, y - 1, z));
            return warm ? -1.0f : 0.0f;
        }
        return 0.0f;
    };

    bool started = false;

    std::vector<std::pair<std::string, std::vector<float>>> coats;
    for (const Mob& mob : entities.mobs())
    {
        if (mob.typeId() != id) continue;
        const std::vector<std::string> layers = coatOverlaysOf(mob);
        if (layers.empty()) continue;
        float sky = 0.0f, blockLight = 0.0f;
        lit(mob, sky, blockLight);
        build(skin, mob, sky, blockLight, 0, 0.0f);
        for (const std::string& path : layers)
        {
            auto found = std::find_if(coats.begin(), coats.end(), [&](const auto& c) { return c.first == path; });
            if (found == coats.end())
            {
                coats.emplace_back(path, std::vector<float>());
                found = coats.end() - 1;
            }
            found->second.insert(found->second.end(), m_vertices.begin(), m_vertices.end());
        }
    }
    for (const auto& coat : coats)
    {
        const unsigned int texture = villagerTexture(coat.first.c_str());
        if (!texture) continue;
        if (!started)
        {
            glDepthFunc(GL_LEQUAL);
            started = true;
        }
        m_mesh.upload(coat.second, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_mesh.draw(GL_TRIANGLES);
    }

    for (const Extra& extra : EXTRAS)
    {
        if (extra.id != id) continue;
        std::vector<float> batch;
        for (const Mob& mob : entities.mobs())
        {
            if (mob.typeId() != id) continue;
            const float glow = glowFor(extra.path, mob);
            if (glow < 0.0f) continue;
            float sky = 0.0f, blockLight = 0.0f;
            lit(mob, sky, blockLight);
            if (glow > 0.0f)
            {
                sky = 0.0f;
                blockLight = std::max(blockLight, glow * mob.deathFade());
            }
            build(skin, mob, sky, blockLight, 0, extra.inflate);
            batch.insert(batch.end(), m_vertices.begin(), m_vertices.end());
        }
        if (batch.empty()) continue;
        const unsigned int texture = villagerTexture(extra.path);
        if (!texture) continue;
        if (!started)
        {
            glDepthFunc(GL_LEQUAL);
            started = true;
        }
        m_mesh.upload(batch, CHUNK_LAYOUT, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_mesh.draw(GL_TRIANGLES);
    }
    if (started) glDepthFunc(GL_LESS);
}

MobModel::~MobModel()
{
    for (auto& entry : m_villagerTextures)
        if (entry.second) glDeleteTextures(1, &entry.second);
}

std::string MobModel::variantOf(const Mob& mob) const
{
    const uint32_t seed = mob.birthSeed() * 2654435761u;
    const uint32_t pick = (seed >> 8) % 1000u;
    const std::string root = "assets/textures/entity/";
    switch (mob.typeId())
    {
        case MobId::Wolf:
        {
            static const char* COATS[] = { "wolf", "wolf_ashen", "wolf_black", "wolf_chestnut", "wolf_rusty",
                                           "wolf_snowy", "wolf_spotted", "wolf_striped", "wolf_woods" };
            const std::string coat = COATS[pick % 9u];
            const char* mood = mob.tamed() ? "_tame" : mob.hostileNow() ? "_angry" : "";
            return root + "wolf/" + coat + mood + ".png";
        }
        case MobId::Shulker:
            if (mob.shulkerColour() < 16)
                return root + "shulker/shulker_" + colourFileName(mob.shulkerColour()) + ".png";
            return "";
        case MobId::Cat:
        {
            static const char* BREEDS[] = { "tabby", "black", "red", "siamese", "british_shorthair", "calico",
                                            "persian", "ragdoll", "white", "jellie", "all_black" };
            return root + "cat/" + BREEDS[pick % 11u] + ".png";
        }
        case MobId::Horse:
        {
            static const char* COLOURS[] = { "white", "creamy", "chestnut", "brown", "black", "gray", "darkbrown" };
            return root + "horse/horse_" + COLOURS[pick % 7u] + ".png";
        }
        case MobId::Llama:
        case MobId::TraderLlama:
        {
            static const char* COATS[] = { "creamy", "white", "brown", "gray" };
            return root + "llama/" + COATS[pick % 4u] + ".png";
        }
        case MobId::Rabbit:
        {
            static const char* COATS[] = { "brown", "white", "black", "white_splotched", "gold", "salt" };
            if (mob.customName() == "Toast") return root + "rabbit/toast.png";
            if (mob.variant() == 99) return root + "rabbit/caerbannog.png";
            return root + "rabbit/" + COATS[pick % 6u] + ".png";
        }
        case MobId::Parrot:
        {
            static const char* COLOURS[] = { "red_blue", "blue", "green", "yellow_blue", "grey" };
            return root + "parrot/parrot_" + COLOURS[pick % 5u] + ".png";
        }
        case MobId::Axolotl:
        {
            static const char* COLOURS[] = { "lucy", "wild", "gold", "cyan", "blue" };
            return root + "axolotl/axolotl_" + COLOURS[axolotlVariantOf(mob.birthSeed())] + ".png";
        }
        case MobId::Frog:
        {
            static const char* KINDS[] = { "temperate", "warm", "cold" };
            return root + "frog/" + KINDS[frogKindOf(mob.birthSeed())] + "_frog.png";
        }
        case MobId::Fox:
        {
            const bool snowy = pick % 4u == 0u;
            return root + "fox/" + (snowy ? "snow_fox" : "fox") + (mob.asleep() ? "_sleep" : "") + ".png";
        }
        case MobId::Panda:
        {
            const uint32_t roll = pick % 100u;
            const char* gene = roll < 2 ? "brown" : roll < 10 ? "lazy" : roll < 18 ? "worried" : roll < 26 ? "playful"
                             : roll < 34 ? "weak" : roll < 42 ? "aggressive" : "";
            return root + "panda/" + (*gene ? std::string(gene) + "_" : std::string()) + "panda.png";
        }
        case MobId::Bee:
            return mob.hostileNow() ? root + "bee/bee_angry.png" : std::string();
        case MobId::Tadpole:
            return root + "tadpole/tadpole.png";
        case MobId::Armadillo:
            return root + "armadillo.png";
        case MobId::Boat:
            return root + "boat/" + boatWoodName(mob.variant()) + ".png";
        case MobId::ChestBoat:
            return root + "chest_boat/" + boatWoodName(mob.variant()) + ".png";
        case MobId::Raft:
            return root + "boat/bamboo.png";
        case MobId::ChestRaft:
            return root + "chest_boat/bamboo.png";
        case MobId::TropicalFish:
        case MobId::TropicalFishLarge:
        {
            const bool large = mob.typeId() == MobId::TropicalFishLarge;
            const TropicalFish::Look look = TropicalFish::lookFor(mob.birthSeed(), large);
            char tint[8];
            std::snprintf(tint, sizeof(tint), "%06X", Banners::dyeRgb(look.base));
            return root + "fish/tropical_" + (large ? "b" : "a") + ".png#" + tint;
        }
        default:
            return std::string();
    }
}

std::vector<std::string> MobModel::coatOverlaysOf(const Mob& mob) const
{
    std::vector<std::string> layers;
    const uint32_t seed = mob.birthSeed() * 2654435761u;
    const uint32_t pick = (seed >> 8) % 1000u;
    const std::string root = "assets/textures/entity/";
    switch (mob.typeId())
    {
        case MobId::Horse:
        {
            static const char* MARKS[] = { "", "white", "whitefield", "whitedots", "blackdots" };
            const char* mark = MARKS[(pick / 7u) % 5u];
            if (*mark) layers.push_back(root + "horse/horse_markings_" + mark + ".png");
            switch (mob.horseArmour())
            {
                case Items::LeatherHorseArmor: layers.push_back(root + "horse/armor/horse_armor_leather.png#A06540"); break;
                case Items::IronHorseArmor: layers.push_back(root + "horse/armor/horse_armor_iron.png"); break;
                case Items::GoldHorseArmor: layers.push_back(root + "horse/armor/horse_armor_gold.png"); break;
                case Items::DiamondHorseArmor: layers.push_back(root + "horse/armor/horse_armor_diamond.png"); break;
                default: break;
            }
            break;
        }
        case MobId::Wolf:
            if (mob.tamed()) layers.push_back(root + "wolf/wolf_collar.png#B02E26");
            if (mob.horseArmour() == Items::WolfArmor)
            {
                layers.push_back(root + "wolf/wolf_armor.png");
                if (mob.armourDye() >= 0)
                {
                    char tint[8];
                    std::snprintf(tint, sizeof(tint), "%06X", Banners::dyeRgb(mob.armourDye()));
                    layers.push_back(root + "wolf/wolf_armor_overlay.png#" + tint);
                }
                const float left = 1.0f - static_cast<float>(mob.armourWear()) / 64.0f;
                if (left < 0.32f) layers.push_back(root + "wolf/wolf_armor_crackiness_high.png");
                else if (left < 0.69f) layers.push_back(root + "wolf/wolf_armor_crackiness_medium.png");
                else if (left < 0.95f) layers.push_back(root + "wolf/wolf_armor_crackiness_low.png");
            }
            break;
        case MobId::Pig:
            if (mob.saddled()) layers.push_back(root + "pig/pig_saddle.png");
            break;
        case MobId::Strider:
            if (mob.saddled()) layers.push_back(root + "strider/strider_saddle.png");
            break;
        case MobId::Cat:
            if (mob.tamed()) layers.push_back(root + "cat/cat_collar.png#B02E26");
            break;
        case MobId::TraderLlama:
            layers.push_back(root + "llama/decor/trader_llama.png");
            break;
        case MobId::TropicalFish:
        case MobId::TropicalFishLarge:
        {
            const bool large = mob.typeId() == MobId::TropicalFishLarge;
            const TropicalFish::Look look = TropicalFish::lookFor(mob.birthSeed(), large);
            char tint[8];
            std::snprintf(tint, sizeof(tint), "%06X", Banners::dyeRgb(look.pattern));
            layers.push_back(root + "fish/tropical_" + (large ? "b" : "a") + "_pattern_" +
                             std::to_string(look.variety % 6 + 1) + ".png#" + tint);
            break;
        }
        default:
            break;
    }
    return layers;
}

unsigned int MobModel::villagerTexture(const char* path)
{
    for (const auto& entry : m_villagerTextures)
        if (entry.first == path) return entry.second;

    std::string file(path);
    unsigned int tint = 0xFFFFFF;
    const size_t hash = file.find('#');
    if (hash != std::string::npos)
    {
        tint = static_cast<unsigned int>(std::strtoul(file.c_str() + hash + 1, nullptr, 16));
        file = file.substr(0, hash);
    }

    int width = 0, height = 0, channels = 0;
    unsigned char* data = Packs::load(file.c_str(), &width, &height, &channels, 4);
    unsigned int texture = 0;
    if (data)
    {
        if (tint != 0xFFFFFF)
            for (int i = 0; i < width * height; ++i)
            {
                data[i * 4 + 0] = static_cast<unsigned char>(data[i * 4 + 0] * ((tint >> 16) & 0xFF) / 255);
                data[i * 4 + 1] = static_cast<unsigned char>(data[i * 4 + 1] * ((tint >> 8) & 0xFF) / 255);
                data[i * 4 + 2] = static_cast<unsigned char>(data[i * 4 + 2] * (tint & 0xFF) / 255);
            }
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }
    m_villagerTextures.emplace_back(path, texture);
    return texture;
}
