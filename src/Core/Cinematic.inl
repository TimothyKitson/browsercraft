#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <stb_image_write.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace
{
    constexpr float CINE_FPS = 30.0f;

    glm::vec3 cineHermite(const glm::vec3& a, const glm::vec3& ta, const glm::vec3& b, const glm::vec3& tb, float span, float u)
    {
        const float u2 = u * u, u3 = u2 * u;
        return (2.0f * u3 - 3.0f * u2 + 1.0f) * a + (u3 - 2.0f * u2 + u) * span * ta + (-2.0f * u3 + 3.0f * u2) * b +
               (u3 - u2) * span * tb;
    }

    glm::vec3 cineTangent(const std::vector<Application::CineKey>& keys, size_t i, bool look, bool easeIn, bool easeOut)
    {
        auto at = [&](size_t k) { return look ? keys[k].look : keys[k].pos; };
        const size_t last = keys.size() - 1;
        if (i == 0)
            return easeIn ? glm::vec3(0.0f) : (at(1) - at(0)) / std::max(0.001f, keys[1].t - keys[0].t);
        if (i == last)
            return easeOut ? glm::vec3(0.0f) : (at(last) - at(last - 1)) / std::max(0.001f, keys[last].t - keys[last - 1].t);
        return (at(i + 1) - at(i - 1)) / std::max(0.001f, keys[i + 1].t - keys[i - 1].t);
    }

    void cineSample(const std::vector<Application::CineKey>& keys, bool easeIn, bool easeOut, float t, glm::vec3& pos, glm::vec3& look)
    {
        if (keys.size() == 1 || t <= keys.front().t)
        {
            pos = keys.front().pos;
            look = keys.front().look;
            return;
        }
        if (t >= keys.back().t)
        {
            pos = keys.back().pos;
            look = keys.back().look;
            return;
        }
        size_t i = 0;
        while (i + 2 < keys.size() && keys[i + 1].t <= t) ++i;
        const float span = keys[i + 1].t - keys[i].t;
        const float u = (t - keys[i].t) / span;
        pos = cineHermite(keys[i].pos, cineTangent(keys, i, false, easeIn, easeOut), keys[i + 1].pos,
                          cineTangent(keys, i + 1, false, easeIn, easeOut), span, u);
        look = cineHermite(keys[i].look, cineTangent(keys, i, true, easeIn, easeOut), keys[i + 1].look,
                           cineTangent(keys, i + 1, true, easeIn, easeOut), span, u);
    }
}

int Application::cineGround(int x, int fromY, int z, int lowest) const
{
    for (int y = fromY; y > lowest; --y)
    {
        const BlockId here = m_world->getBlock(x, y, z);
        if (isSolid(here) && !isLeaves(here) && m_world->getBlock(x, y + 1, z) == Blocks::Air && m_world->getBlock(x, y + 2, z) == Blocks::Air)
            return y + 1;
    }
    return fromY;
}

void Application::cineBuildPortal(int x0, int y0, int z0)
{
    for (int dx = 0; dx < 4; ++dx)
        for (int dy = 0; dy < 5; ++dy)
            if (dx == 0 || dx == 3 || dy == 0 || dy == 4) m_world->setBlock(x0 + dx, y0 + dy, z0, Blocks::Obsidian);
    for (int dx = 1; dx < 3; ++dx)
        for (int dy = 1; dy < 4; ++dy) m_world->setBlock(x0 + dx, y0 + dy, z0, Blocks::NetherPortal);
}

void Application::cineSummon(MobId type, int x, int fromY, int z, int lowest)
{
    const int feet = cineGround(x, fromY, z, lowest);
    m_entities.summonAt(type, glm::vec3(x + 0.5f, static_cast<float>(feet), z + 0.5f));
}

void Application::cineAim(const glm::vec3& eye, const glm::vec3& look)
{
    const glm::vec3 offset = m_player.eyePosition() - m_player.position;
    m_player.position = eye - offset;
    m_player.velocity = glm::vec3(0.0f);
    m_camera.position = eye;
    glm::vec3 dir = look - eye;
    if (glm::length(dir) < 0.0001f) dir = glm::vec3(0.0f, 0.0f, 1.0f);
    dir = glm::normalize(dir);
    m_camera.setOrientation(glm::degrees(std::atan2(dir.z, dir.x)), glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f))));
}

bool Application::cineLoaded(const glm::vec3& around, int radius) const
{
    if (!m_world || !m_worldReady || m_world->pendingJobs() > 2) return false;
    const int cx = static_cast<int>(std::floor(around.x / 16.0f));
    const int cz = static_cast<int>(std::floor(around.z / 16.0f));
    for (int dz = -radius; dz <= radius; ++dz)
        for (int dx = -radius; dx <= radius; ++dx)
            if (m_world->chunkAt(cx + dx, cz + dz) == nullptr) return false;
    return true;
}

float Application::cineSight(const glm::vec3& from, const glm::vec3& to) const
{
    const glm::vec3 span = to - from;
    const float length = glm::length(span);
    if (length < 1.0f) return 1.0f;
    const int steps = static_cast<int>(length * 2.0f);
    int open = 0, counted = 0;
    for (int k = 1; k < steps; ++k)
    {
        const float t = static_cast<float>(k) / static_cast<float>(steps);
        if (t * length > length - 6.0f) break;
        const glm::vec3 p = from + span * t;
        ++counted;
        if (!isOpaque(m_world->getBlock(static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y)), static_cast<int>(std::floor(p.z)))))
            ++open;
    }
    return counted > 0 ? static_cast<float>(open) / static_cast<float>(counted) : 1.0f;
}

bool Application::cineOpenAir(const glm::vec3& at) const
{
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz)
                if (isSolid(m_world->getBlock(static_cast<int>(std::floor(at.x)) + dx, static_cast<int>(std::floor(at.y)) + dy,
                                              static_cast<int>(std::floor(at.z)) + dz)))
                    return false;
    return true;
}

glm::vec3 Application::cineCentroid(const glm::vec3& around, int reach, int low, int high, bool (*match)(const char*), int& count, int& top) const
{
    glm::vec3 sum(0.0f);
    count = 0;
    top = low;
    for (int dz = -reach; dz <= reach; dz += 2)
        for (int dx = -reach; dx <= reach; dx += 2)
            for (int y = low; y <= high; ++y)
            {
                const int x = static_cast<int>(around.x) + dx, z = static_cast<int>(around.z) + dz;
                const BlockId here = m_world->getBlock(x, y, z);
                if (here == Blocks::Air || !match(blockInfo(here).name)) continue;
                sum += glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                ++count;
                top = std::max(top, y);
            }
    return count > 0 ? sum / static_cast<float>(count) : around;
}

void Application::cineClearPath()
{
    const int frames = static_cast<int>(m_cineShot.seconds * CINE_FPS);
    for (int frame = 0; frame <= frames; frame += 2)
    {
        glm::vec3 eye, look;
        cineSample(m_cineShot.keys, m_cineShot.easeIn, m_cineShot.easeOut, static_cast<float>(frame) / CINE_FPS, eye, look);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    const int x = static_cast<int>(std::floor(eye.x)) + dx;
                    const int y = static_cast<int>(std::floor(eye.y)) + dy;
                    const int z = static_cast<int>(std::floor(eye.z)) + dz;
                    const glm::vec3 centre(x + 0.5f, y + 0.5f, z + 0.5f);
                    if (glm::length(centre - eye) > 1.3f) continue;
                    const BlockId here = m_world->getBlock(x, y, z);
                    if (here == Blocks::Air || here == Blocks::Obsidian || here == Blocks::NetherPortal) continue;
                    m_world->setBlock(x, y, z, Blocks::Air);
                }
        for (int dy = -4; dy <= 4; ++dy)
            for (int dz = -4; dz <= 4; ++dz)
                for (int dx = -4; dx <= 4; ++dx)
                {
                    const int x = static_cast<int>(std::floor(eye.x)) + dx;
                    const int y = static_cast<int>(std::floor(eye.y)) + dy;
                    const int z = static_cast<int>(std::floor(eye.z)) + dz;
                    if (glm::length(glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f) - eye) > 4.0f) continue;
                    const BlockId here = m_world->getBlock(x, y, z);
                    const bool tree = isLeaves(here) || axisBase(here) == logOfWood(0) || axisBase(here) == logOfWood(2) ||
                                      axisBase(here) == logOfWood(1) || axisBase(here) == logOfWood(5) || here == Blocks::Vine;
                    if (tree) m_world->setBlock(x, y, z, Blocks::Air);
                }
    }
}

bool Application::cineStage(int shot)
{
    const WorldGen& gen = m_world->generator();
    CineShot& s = m_cineShot;
    s.keys.clear();
    s.easeIn = true;
    s.easeOut = true;
    s.trackDragon = false;
    const glm::vec3 a = m_cineAnchor;

    if (shot == 0)
    {
        int ax = static_cast<int>(a.x), az = static_cast<int>(a.z);
        int bestSpread = 1000;
        int bestDistance = 1000;
        for (int oz = -20; oz <= 20; ++oz)
            for (int ox = -20; ox <= 20; ++ox)
            {
                int lowest = 1000, highest = -1000;
                bool wet = false;
                for (int dx = -1; dx <= 4 && !wet; ++dx)
                    for (int dz = -2; dz <= 1; ++dz)
                    {
                        const int g = cineGround(static_cast<int>(a.x) + ox + dx, 200, static_cast<int>(a.z) + oz + dz, -60);
                        const BlockId under = m_world->getBlock(static_cast<int>(a.x) + ox + dx, g - 1, static_cast<int>(a.z) + oz + dz);
                        if (isWater(under) || under == Blocks::Sand) wet = true;
                        lowest = std::min(lowest, g);
                        highest = std::max(highest, g);
                    }
                if (wet) continue;
                const int spread = highest - lowest;
                const int distance = std::abs(ox) + std::abs(oz);
                if (spread < bestSpread || (spread == bestSpread && distance < bestDistance))
                {
                    bestSpread = spread;
                    bestDistance = distance;
                    ax = static_cast<int>(a.x) + ox;
                    az = static_cast<int>(a.z) + oz;
                }
            }
        m_cineAnchor = glm::vec3(static_cast<float>(ax), 0.0f, static_cast<float>(az));
        int ground = 1000;
        for (int dx = 0; dx <= 3; ++dx) ground = std::min(ground, cineGround(ax + dx, 200, az, -60));
        const int x0 = ax, y0 = ground, z0 = az;
        for (int dx = 0; dx <= 3; ++dx)
        {
            for (int dy = 0; dy <= 5; ++dy) m_world->setBlock(x0 + dx, y0 + dy, z0, Blocks::Air);
            for (int dz = -1; dz <= 1; ++dz)
                for (int dy = 0; dy <= 3; ++dy)
                {
                    const BlockId here = m_world->getBlock(x0 + dx, y0 + dy, z0 + dz);
                    if (!isSolid(here) || isLeaves(here)) m_world->setBlock(x0 + dx, y0 + dy, z0 + dz, Blocks::Air);
                }
        }
        cineBuildPortal(x0, y0, z0);
        m_cinePortal = glm::vec3(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(z0));
        const MobId herd[] = { MobId::Sheep, MobId::Sheep, MobId::Sheep, MobId::Cow, MobId::Cow, MobId::Pig, MobId::Pig,
                               MobId::Horse, MobId::Chicken, MobId::Chicken };
        for (int k = 0; k < static_cast<int>(std::size(herd)); ++k)
            cineSummon(herd[k], x0 + 2 + ((k * 7) % 17) - 8 + (k % 2) * 6, y0 + 20, z0 - 9 - (k * 5) % 13, y0 - 20);
        const float cy = static_cast<float>(y0) + 2.5f;
        const float zf = static_cast<float>(z0);
        const float xc = static_cast<float>(x0) + 3.5f;
        s.seconds = 8.0f;
        s.easeOut = false;
        s.keys = {
            { 0.0f, { x0 + 26.0f, y0 + 15.0f, zf - 40.0f }, { x0 + 2.0f, y0 + 2.0f, zf } },
            { 3.6f, { x0 + 13.0f, y0 + 7.0f, zf - 17.0f }, { x0 + 2.0f, y0 + 2.5f, zf } },
            { 6.0f, { x0 + 6.4f, cy, zf - 3.2f }, { x0 + 4.0f, cy, zf + 6.0f } },
            { 7.1f, { xc + 1.7f, cy, zf - 0.3f }, { xc + 1.7f, cy, zf + 12.0f } },
            { 8.0f, { xc, cy, zf - 0.3f }, { xc, cy, zf + 12.0f } },
        };
        return true;
    }

    if (shot == 1)
    {
        int count = 0, top = 0;
        const glm::vec3 bastion = cineCentroid(a, 48, WorldGen::NETHER_LAVA_LEVEL - 4, WorldGen::NETHER_LAVA_LEVEL + 70,
                                               [](const char* name) { return std::strstr(name, "Polished Blackstone") != nullptr ||
                                                                             std::strstr(name, "Gilded Blackstone") != nullptr; },
                                               count, top);
        const glm::vec3 focus(bastion.x, std::max(bastion.y, static_cast<float>(top) - 8.0f), bastion.z);
        float best = -1.0f;
        glm::ivec3 stand(static_cast<int>(a.x), WorldGen::NETHER_LAVA_LEVEL + 25, static_cast<int>(a.z) - 40);
        bool grounded = false;
        for (int step = 0; step < 36; ++step)
            for (const float radius : { 30.0f, 38.0f, 46.0f })
            {
                const float angle = glm::radians(10.0f * static_cast<float>(step));
                const int x = static_cast<int>(focus.x + std::cos(angle) * radius);
                const int z = static_cast<int>(focus.z + std::sin(angle) * radius);
                const int feet = cineGround(x, static_cast<int>(focus.y) + 14, z, WorldGen::NETHER_LAVA_LEVEL);
                if (feet <= WorldGen::NETHER_LAVA_LEVEL + 1) continue;
                const BlockId under = m_world->getBlock(x, feet - 1, z);
                if (under == Blocks::Lava || isWater(under)) continue;
                bool room = true;
                for (int dy = 0; dy < 6 && room; ++dy)
                    for (int dx = -1; dx <= 4 && room; ++dx)
                        if (isSolid(m_world->getBlock(x + dx, feet + dy, z))) room = false;
                if (!room) continue;
                const glm::vec3 eye(static_cast<float>(x) + 0.5f, static_cast<float>(feet) + 2.5f, static_cast<float>(z) + 0.5f);
                const float score = cineSight(eye, focus) + 0.5f * cineSight(eye + glm::vec3(0.0f, 6.0f, 0.0f), focus) -
                                    0.01f * std::abs(eye.y - focus.y);
                if (score > best)
                {
                    best = score;
                    stand = glm::ivec3(x, feet, z);
                    grounded = true;
                }
            }
        const glm::vec3 toward = glm::normalize(glm::vec3(focus.x - stand.x, 0.0f, focus.z - stand.z));
        const glm::vec3 side(-toward.z, 0.0f, toward.x);
        const int x1 = stand.x, y1 = stand.y, z1 = stand.z;
        if (!grounded)
            for (int dx = 0; dx < 4; ++dx)
                for (int dy = 1; dy < 50; ++dy)
                {
                    const BlockId below = m_world->getBlock(x1 + dx, y1 - dy, z1);
                    if (isSolid(below)) break;
                    m_world->setBlock(x1 + dx, y1 - dy, z1, Blocks::Netherrack);
                }
        const bool alongX = std::abs(toward.z) >= std::abs(toward.x);
        for (int k = 0; k < 4; ++k)
            for (int dy = 0; dy < 5; ++dy)
            {
                const int px = alongX ? x1 + k : x1, pz = alongX ? z1 : z1 + k;
                const bool edge = k == 0 || k == 3 || dy == 0 || dy == 4;
                m_world->setBlock(px, y1 + dy, pz, edge ? Blocks::Obsidian : Blocks::Air);
            }
        for (int k = 1; k < 3; ++k)
            for (int dy = 1; dy < 4; ++dy)
                m_world->setBlock(alongX ? x1 + k : x1, y1 + dy, alongX ? z1 : z1 + k, Blocks::NetherPortal);
        m_cinePortal = glm::vec3(static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(z1));
        const MobId crowd[] = { MobId::Piglin, MobId::Piglin, MobId::Piglin, MobId::Piglin, MobId::PiglinBrute, MobId::Hoglin,
                                MobId::ZombiePigman, MobId::ZombiePigman };
        for (int k = 0; k < static_cast<int>(std::size(crowd)); ++k)
        {
            const glm::vec3 spot = glm::mix(glm::vec3(stand), focus, 0.35f + 0.08f * static_cast<float>(k % 4)) + side * (static_cast<float>(k) * 2.0f - 7.0f);
            cineSummon(crowd[k], static_cast<int>(spot.x), static_cast<int>(focus.y) + 10, static_cast<int>(spot.z), WorldGen::NETHER_LAVA_LEVEL);
        }
        const glm::vec3 axis = alongX ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 facing = alongX ? glm::vec3(0.0f, 0.0f, toward.z >= 0.0f ? 1.0f : -1.0f)
                                        : glm::vec3(toward.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
        const glm::vec3 right(-facing.z, 0.0f, facing.x);
        const bool firstColumn = glm::dot(right, axis) < 0.0f;
        const glm::vec3 slide = firstColumn ? -axis : axis;
        const float cy = static_cast<float>(y1) + 2.5f;
        const glm::vec3 base(static_cast<float>(x1) + 0.5f, cy, static_cast<float>(z1) + 0.5f);
        const glm::vec3 near = base + axis * (firstColumn ? 0.0f : 3.0f) - facing * 0.8f;
        s.seconds = 7.0f;
        s.easeIn = false;
        s.keys = {
            { 0.0f, near, near + facing * 12.0f },
            { 0.9f, near + slide * 1.9f, near + slide * 1.9f + facing * 12.0f },
            { 3.0f, near + slide * 4.0f - facing * 2.0f + glm::vec3(0.0f, 2.5f, 0.0f), focus + glm::vec3(0.0f, 4.0f, 0.0f) },
            { 7.0f, glm::mix(near, focus, 0.45f) + glm::vec3(0.0f, 6.0f, 0.0f) + side * 6.0f, focus },
        };
        return true;
    }

    if (shot == 2)
    {
        int bricks = 0, stairs = 0, top = 0, stairTop = 0;
        const int floorY = WorldGen::FORTRESS_FLOOR;
        const glm::vec3 hall = cineCentroid(a, 64, floorY - 16, floorY + 24,
                                            [](const char* name) { return std::strstr(name, "Nether Brick") != nullptr; }, bricks, top);
        const glm::vec3 steps = cineCentroid(hall, 40, floorY - 16, floorY + 24,
                                             [](const char* name) { return std::strstr(name, "Nether Brick Stairs") != nullptr; }, stairs, stairTop);
        const glm::vec3 focus = stairs >= 6 ? steps : hall;
        float best = -1.0f;
        float bestAngle = 0.0f, bestRadius = 22.0f, bestLift = 8.0f;
        for (int step = 0; step < 24; ++step)
            for (const float radius : { 16.0f, 22.0f, 28.0f })
                for (const float lift : { 4.0f, 9.0f, 14.0f })
                {
                    const float angle = glm::radians(15.0f * static_cast<float>(step));
                    const glm::vec3 eye = focus + glm::vec3(std::cos(angle) * radius, lift, std::sin(angle) * radius);
                    if (!cineOpenAir(eye)) continue;
                    const float later = glm::radians(15.0f * static_cast<float>(step) + 50.0f);
                    const glm::vec3 after = focus + glm::vec3(std::cos(later) * (radius - 6.0f), lift - 3.0f, std::sin(later) * (radius - 6.0f));
                    const float score = cineSight(eye, focus) + cineSight(after, focus) + (cineOpenAir(after) ? 0.5f : 0.0f);
                    if (score > best)
                    {
                        best = score;
                        bestAngle = 15.0f * static_cast<float>(step);
                        bestRadius = radius;
                        bestLift = lift;
                    }
                }
        m_cineAnchor = focus;
        const MobId guards[] = { MobId::Blaze, MobId::Blaze, MobId::Blaze, MobId::WitherSkeleton, MobId::WitherSkeleton,
                                 MobId::Piglin, MobId::ZombiePigman, MobId::ZombiePigman, MobId::MagmaCube };
        for (int k = 0; k < static_cast<int>(std::size(guards)); ++k)
        {
            const float angle = glm::radians(bestAngle + 20.0f * static_cast<float>(k) - 60.0f);
            const int gx = static_cast<int>(focus.x + std::cos(angle) * (3.0f + static_cast<float>(k % 3) * 2.0f));
            const int gz = static_cast<int>(focus.z + std::sin(angle) * (3.0f + static_cast<float>(k % 3) * 2.0f));
            const int feet = cineGround(gx, static_cast<int>(focus.y) + 6, gz, static_cast<int>(focus.y) - 12);
            if (guards[k] == MobId::Blaze)
                m_entities.summonAt(guards[k], glm::vec3(gx + 0.5f, static_cast<float>(feet) + 2.5f, gz + 0.5f));
            else
                m_entities.summonAt(guards[k], glm::vec3(gx + 0.5f, static_cast<float>(feet), gz + 0.5f));
        }
        s.seconds = 7.0f;
        for (int k = 0; k <= 4; ++k)
        {
            const float angle = glm::radians(bestAngle + 12.5f * static_cast<float>(k));
            const float radius = bestRadius - 1.5f * static_cast<float>(k);
            s.keys.push_back({ 7.0f * static_cast<float>(k) / 4.0f,
                               focus + glm::vec3(std::cos(angle) * radius, bestLift - 0.75f * static_cast<float>(k), std::sin(angle) * radius),
                               focus + glm::vec3(0.0f, 1.0f, 0.0f) });
        }
        return true;
    }

    if (shot == 3)
    {
        const int top = WorldGen::END_ISLAND_LEVEL + 30;
        const int ring[8][2] = { { 14, 6 }, { -12, 10 }, { 8, -16 }, { -18, -8 }, { 24, -4 }, { -6, 22 }, { 20, 18 }, { -26, 14 } };
        for (const auto& spot : ring) cineSummon(MobId::Enderman, spot[0], top, spot[1], WorldGen::END_ISLAND_LEVEL - 30);
        const glm::vec3 centre(0.5f, static_cast<float>(WorldGen::END_ISLAND_LEVEL) + 6.0f, 0.5f);
        s.seconds = 10.0f;
        s.trackDragon = true;
        for (int k = 0; k <= 5; ++k)
        {
            const float angle = glm::radians(205.0f + 14.0f * static_cast<float>(k));
            const float radius = 62.0f - 7.0f * static_cast<float>(k);
            s.keys.push_back({ 10.0f * static_cast<float>(k) / 5.0f,
                               centre + glm::vec3(std::cos(angle) * radius, 26.0f - 3.6f * static_cast<float>(k), std::sin(angle) * radius),
                               centre + glm::vec3(0.0f, 4.0f, 0.0f) });
        }
        return true;
    }
    return false;
}

bool Application::cineLocate(int shot)
{
    const WorldGen& gen = m_world->generator();
    if (shot == 0)
    {
        int sx = 0, sy = 80, sz = 0;
        gen.findSpawn(sx, sy, sz);
        const Biomes::Kind wanted[] = { Biomes::fromId("sunflower_plains"), Biomes::fromId("meadow"), Biomes::fromId("plains"),
                                        Biomes::fromId("flower_forest") };
        for (const Biomes::Kind kind : wanted)
            for (int ring = 0; ring < 200; ring += 2)
                for (int step = 0; step < 24; ++step)
                {
                    const float angle = static_cast<float>(step) * 0.2618f;
                    const int x = sx + static_cast<int>(std::cos(angle) * static_cast<float>(ring * 16));
                    const int z = sz + static_cast<int>(std::sin(angle) * static_cast<float>(ring * 16));
                    if (gen.kindIn(Dimension::Overworld, x, 120, z) != kind) continue;
                    if (gen.kindIn(Dimension::Overworld, x + 26, 120, z - 40) != kind) continue;
                    m_cineAnchor = glm::vec3(static_cast<float>(x), 0.0f, static_cast<float>(z));
                    m_cineWait = glm::vec3(x + 13.0f, static_cast<float>(gen.surfaceHeight(x, z) + 8), z - 17.0f);
                    return true;
                }
        m_cineAnchor = glm::vec3(static_cast<float>(sx), 0.0f, static_cast<float>(sz));
        m_cineWait = glm::vec3(sx + 13.0f, static_cast<float>(sy + 8), sz - 17.0f);
        return true;
    }
    if (shot == 1)
    {
        for (int squares = 4; squares <= 16; squares += 4)
        {
            const Structures::Spot spot = gen.nearestBastion(0, 0, squares);
            if (!spot.exists) continue;
            const int bx = spot.chunkX * Structures::CHUNK + Structures::CHUNK / 2;
            const int bz = spot.chunkZ * Structures::CHUNK + Structures::CHUNK / 2;
            m_cineAnchor = glm::vec3(static_cast<float>(bx), 0.0f, static_cast<float>(bz));
            m_cineWait = glm::vec3(bx - 6.0f, static_cast<float>(WorldGen::NETHER_LAVA_LEVEL + 30), bz - 30.0f);
            return true;
        }
        return false;
    }
    if (shot == 2)
    {
        int midX = 0, midZ = 0, turn = 0;
        for (int ring = 0; ring < 10; ++ring)
            for (int sz = -ring; sz <= ring; ++sz)
                for (int sx = -ring; sx <= ring; ++sx)
                    if (std::max(std::abs(sx), std::abs(sz)) == ring && gen.fortressIn(sx, sz, midX, midZ, turn))
                    {
                        m_cineAnchor = glm::vec3(static_cast<float>(midX), 0.0f, static_cast<float>(midZ));
                        m_cineWait = glm::vec3(midX + 18.0f, static_cast<float>(WorldGen::FORTRESS_FLOOR + 14), midZ + 12.0f);
                        return true;
                    }
        return false;
    }
    if (shot == 3)
    {
        m_cineAnchor = glm::vec3(0.0f);
        m_cineWait = glm::vec3(-60.0f, static_cast<float>(WorldGen::END_ISLAND_LEVEL + 40), -20.0f);
        return true;
    }
    return false;
}

void Application::tickCinematic()
{
    if (!m_world) return;
    static const Dimension DIMENSIONS[4] = { Dimension::Overworld, Dimension::Nether, Dimension::Nether, Dimension::End };
    m_hudHidden = true;
    m_showDebug = false;
    if (!m_shadersOn)
    {
        m_shadersOn = true;
        if (!shadersActive()) m_shadersOn = false;
    }
    if (m_cineStride == 0)
    {
        const char* stills = std::getenv("BC_CINE_STILLS");
        m_cineStillDir = stills ? stills : "";
        m_cineStride = m_cineStillDir.empty() ? 1 : 6;
    }
    m_viewBobbing = false;
    m_weather = Weather::State{};
    m_timeOfDay = 0.33f;
    m_cineCapture = false;

    if (m_cineShotIndex >= 4)
    {
        finishCinematic();
        m_running = false;
        return;
    }

    switch (m_cinePhase)
    {
        case CinePhase::Travel:
        {
            const Dimension wanted = DIMENSIONS[m_cineShotIndex];
            if (m_dimension != wanted) travelTo(wanted);
            if (!m_world) return;
            if (m_player.mode != GameMode::Spectator && m_worldReady) setGameMode(GameMode::Spectator);
            m_world->setRenderDistance(12);
            if (!cineLocate(m_cineShotIndex))
            {
                std::printf("[cinematic] shot %d has nowhere to film, skipped\n", m_cineShotIndex);
                std::fflush(stdout);
                ++m_cineShotIndex;
                return;
            }
            m_cineCounter = 0;
            m_cinePhase = CinePhase::Load;
            cineAim(m_cineWait, glm::vec3(m_cineAnchor.x, m_cineWait.y - 6.0f, m_cineAnchor.z));
            return;
        }
        case CinePhase::Load:
            if (m_worldReady && m_player.mode != GameMode::Spectator) setGameMode(GameMode::Spectator);
            cineAim(m_cineWait, glm::vec3(m_cineAnchor.x, m_cineWait.y - 6.0f, m_cineAnchor.z));
            ++m_cineSpent;
            m_cineCounter = cineLoaded(m_cineWait, 5) ? m_cineCounter + 1 : 0;
            if (m_cineSpent < 90 || (m_cineCounter < 20 && m_cineSpent < 3000)) return;
            m_cineSpent = 0;
            if (!cineStage(m_cineShotIndex))
            {
                ++m_cineShotIndex;
                m_cinePhase = CinePhase::Travel;
                return;
            }
            cineClearPath();
            m_cineCounter = 0;
            m_cinePhase = CinePhase::Settle;
            [[fallthrough]];
        case CinePhase::Settle:
        {
            glm::vec3 eye, look;
            cineSample(m_cineShot.keys, m_cineShot.easeIn, m_cineShot.easeOut, 0.0f, eye, look);
            cineAim(eye, look);
            ++m_cineSpent;
            m_cineCounter = cineLoaded(eye, 4) ? m_cineCounter + 1 : 0;
            if (m_cineSpent < 60 || (m_cineCounter < 20 && m_cineSpent < 600)) return;
            m_cineSpent = 0;
            m_cineFrame = 0;
            m_cinePhase = CinePhase::Shoot;
            std::printf("[cinematic] filming shot %d at %.0f %.0f\n", m_cineShotIndex, m_cineAnchor.x, m_cineAnchor.z);
            std::fflush(stdout);
            [[fallthrough]];
        }
        case CinePhase::Shoot:
        {
            const float t = static_cast<float>(m_cineFrame) / CINE_FPS;
            glm::vec3 eye, look;
            cineSample(m_cineShot.keys, m_cineShot.easeIn, m_cineShot.easeOut, t, eye, look);
            if (m_cineShot.trackDragon)
                for (const Mob& mob : m_entities.mobs())
                    if (mob.alive() && mob.typeId() == MobId::EnderDragon)
                    {
                        const glm::vec3 dragon = mob.position();
                        if (!m_cineDragonSeen)
                        {
                            m_cineDragonAim = dragon;
                            m_cineDragonSeen = true;
                        }
                        m_cineDragonAim += (dragon - m_cineDragonAim) * 0.06f;
                        look = glm::mix(look, m_cineDragonAim, 0.45f);
                        break;
                    }
            cineAim(eye, look);
            if (m_cineFrame == 0) cineExportShot();
            cineTrackFrame(eye, look);
            m_cineCapture = true;
            m_cineFrame += m_cineStride;
            if (m_cineFrame >= static_cast<int>(m_cineShot.seconds * CINE_FPS))
            {
                ++m_cineShotIndex;
                m_cinePhase = CinePhase::Travel;
                m_cineDragonSeen = false;
            }
            return;
        }
    }
}

void Application::cineSaveTexture(unsigned int texture, const std::string& path)
{
#ifndef __EMSCRIPTEN__
    GLint w = 0, h = 0;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    if (w <= 0 || h <= 0) return;
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_write_png(path.c_str(), w, h, 4, pixels.data(), w * 4);
#endif
}

void Application::cineExportShot()
{
    if (m_cineExportDir.empty() || !m_world) return;
    namespace fs = std::filesystem;
    const std::string dir = m_cineExportDir + "/shot" + std::to_string(m_cineShotIndex);
    fs::create_directories(dir);
    if (!m_cineAtlasSaved)
    {
        cineSaveTexture(m_atlas.textureId(), m_cineExportDir + "/atlas.png");
        m_cineAtlasSaved = true;
    }

    glm::vec3 centre(0.0f);
    for (const CineKey& key : m_cineShot.keys) centre += key.pos;
    centre /= static_cast<float>(std::max<size_t>(1, m_cineShot.keys.size()));
    const int cx = static_cast<int>(std::floor(centre.x / 16.0f));
    const int cz = static_cast<int>(std::floor(centre.z / 16.0f));
    const int reach = m_cineShotIndex == 3 ? 7 : 5;

    std::ofstream solid(dir + "/terrain.bin", std::ios::binary);
    std::ofstream clear(dir + "/terrain_alpha.bin", std::ios::binary);
    auto writeVertices = [](std::ofstream& out, const std::vector<float>& v, const glm::vec3& offset) {
        constexpr size_t STRIDE = 10;
        for (size_t i = 0; i + STRIDE <= v.size(); i += STRIDE)
        {
            const float light = std::max(v[i + 6], v[i + 7]);
            const uint32_t packed = static_cast<uint32_t>(std::max(v[i + 9], 0.0f) + 0.5f);
            const float record[10] = {
                v[i] + offset.x, v[i + 1] + offset.y, v[i + 2] + offset.z, v[i + 3], v[i + 4], v[i + 5] * (0.08f + 0.92f * light),
                static_cast<float>((packed >> 14u) & 127u) / 64.0f, static_cast<float>((packed >> 7u) & 127u) / 64.0f,
                static_cast<float>(packed & 127u) / 64.0f, static_cast<float>((packed >> 21u) & 1u) };
            out.write(reinterpret_cast<const char*>(record), sizeof(record));
        }
    };
    for (int dz = -reach; dz <= reach; ++dz)
        for (int dx = -reach; dx <= reach; ++dx)
        {
            const Chunk* chunk = m_world->chunkAt(cx + dx, cz + dz);
            if (!chunk) continue;
            MeshSnapshot snapshot;
            m_world->snapshotFor(*chunk, snapshot);
            MeshResult mesh;
            buildChunkMesh(snapshot, mesh);
            const glm::vec3 offset(static_cast<float>(chunk->position().x * Chunk::SX), 0.0f, static_cast<float>(chunk->position().z * Chunk::SZ));
            writeVertices(solid, mesh.opaqueVertices, offset);
            writeVertices(clear, mesh.transparentVertices, offset);
        }

    std::ofstream mobs(dir + "/mobs.bin", std::ios::binary);
    std::ofstream list(dir + "/mobs.txt");
    for (const Mob& mob : m_entities.mobs())
    {
        if (!mob.alive() || glm::distance(mob.position(), centre) > 160.0f) continue;
        const auto layers = m_mobModel.figure(mob);
        for (const auto& layer : layers)
        {
            const std::string texture = "tex_" + std::to_string(layer.first) + ".png";
            if (m_cineSavedTextures.insert(layer.first).second) cineSaveTexture(layer.first, m_cineExportDir + "/" + texture);
            const uint32_t header[2] = { mob.serial(), static_cast<uint32_t>(layer.second.size() / 9) };
            mobs.write(reinterpret_cast<const char*>(header), sizeof(header));
            const uint32_t nameLength = static_cast<uint32_t>(texture.size());
            mobs.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            mobs.write(texture.data(), static_cast<std::streamsize>(texture.size()));
            for (size_t i = 0; i + 9 <= layer.second.size(); i += 9)
            {
                const float record[6] = { layer.second[i] - mob.position().x, layer.second[i + 1] - mob.position().y,
                                          layer.second[i + 2] - mob.position().z, layer.second[i + 3], layer.second[i + 4], layer.second[i + 5] };
                mobs.write(reinterpret_cast<const char*>(record), sizeof(record));
            }
        }
        list << mob.serial() << ' ' << mobType(mob.typeId()).name << '\n';
    }
    m_cineTrack.close();
    m_cineTrack.open(dir + "/track.txt");
    std::ofstream info(dir + "/shot.txt");
    info << "dimension " << static_cast<int>(m_dimension) << "\n";
    info << "frames " << static_cast<int>(m_cineShot.seconds * CINE_FPS) << "\n";
    info << "anchor " << m_cineAnchor.x << ' ' << m_cineAnchor.y << ' ' << m_cineAnchor.z << "\n";
    info << "portal " << m_cinePortal.x << ' ' << m_cinePortal.y << ' ' << m_cinePortal.z << "\n";
    const glm::vec3 sky = dimensionBackdrop();
    info << "sky " << sky.r << ' ' << sky.g << ' ' << sky.b << "\n";
    m_crystalScan = 0;
    for (const glm::ivec3& crystal : crystalSpots())
        info << "crystal " << crystal.x << ' ' << crystal.y << ' ' << crystal.z << "\n";
    info << "ticks " << m_elapsedSeconds * 20.0f << "\n";
}

void Application::cineTrackFrame(const glm::vec3& eye, const glm::vec3& look)
{
    if (!m_cineTrack.is_open()) return;
    m_cineTrack << "camera " << m_cineFrame << ' ' << eye.x << ' ' << eye.y << ' ' << eye.z << ' ' << look.x << ' ' << look.y << ' ' << look.z << '\n';
    for (const Mob& mob : m_entities.mobs())
        if (mob.alive())
            m_cineTrack << "mob " << m_cineFrame << ' ' << mob.serial() << ' ' << mob.position().x << ' ' << mob.position().y << ' '
                        << mob.position().z << ' ' << mob.yaw() << '\n';
}

void Application::captureCinematicFrame()
{
#ifndef __EMSCRIPTEN__
    const int w = m_window.width();
    const int h = m_window.height();
    if (!m_cineStillDir.empty())
    {
        const int total = static_cast<int>(m_cineShot.seconds * CINE_FPS);
        const int frame = m_cineFrame - m_cineStride;
        for (const float part : { 0.2f, 0.55f, 0.9f })
        {
            const int wanted = static_cast<int>(part * static_cast<float>(total));
            if (frame > wanted || frame + m_cineStride <= wanted) continue;
            std::vector<unsigned char> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
            std::vector<unsigned char> flipped(pixels.size());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            for (int row = 0; row < h; ++row)
                std::copy(pixels.begin() + static_cast<long>(row) * w * 3, pixels.begin() + static_cast<long>(row + 1) * w * 3,
                          flipped.begin() + static_cast<long>(h - 1 - row) * w * 3);
            const std::string path = m_cineStillDir + "/shot" + std::to_string(m_cineShotIndex) + "_" +
                                     std::to_string(static_cast<int>(part * 100.0f)) + ".png";
            stbi_write_png(path.c_str(), w, h, 3, flipped.data(), w * 3);
        }
        return;
    }
    if (!m_cinePipe)
    {
        const std::string command = "ffmpeg -y -loglevel error -f rawvideo -pixel_format rgb24 -video_size " + std::to_string(w) + "x" +
                                    std::to_string(h) + " -framerate 30 -i - -vf vflip -c:v libx264 -preset slow -crf 15 -pix_fmt yuv420p "
                                    "-movflags +faststart \"" + m_cinematicPath + "\"";
        m_cinePipe = popen(command.c_str(), "w");
        if (!m_cinePipe)
        {
            std::printf("[cinematic] could not start ffmpeg\n");
            m_running = false;
            return;
        }
    }
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::fwrite(pixels.data(), 1, pixels.size(), m_cinePipe);
    ++m_cineWritten;
#endif
}

void Application::finishCinematic()
{
#ifndef __EMSCRIPTEN__
    m_cineTrack.close();
    if (m_cinePipe)
    {
        pclose(m_cinePipe);
        m_cinePipe = nullptr;
        std::printf("[cinematic] wrote %d frames to %s\n", m_cineWritten, m_cinematicPath.c_str());
        std::fflush(stdout);
    }
#endif
}
