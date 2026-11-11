#include "Piston.h"
#include <algorithm>

namespace
{
    const glm::ivec3 SIX[6] = { { 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 } };

    struct Resolver
    {
        const Redstone::Cells& cells;
        glm::ivec3 piston;
        glm::ivec3 dir;
        std::vector<glm::ivec3> push;
        std::vector<glm::ivec3> destroy;

        BlockId at(const glm::ivec3& p) const { return cells.at(p.x, p.y, p.z); }
        bool has(const glm::ivec3& p) const { return std::find(push.begin(), push.end(), p) != push.end(); }
        bool full() const { return static_cast<int>(push.size()) >= Piston::MAX_PUSH; }

        bool line(glm::ivec3 origin)
        {
            BlockId state = at(origin);
            if (Piston::replaceable(state) || !Piston::movable(state) || Piston::fragile(state)) return true;
            if (origin == piston || has(origin)) return true;

            int count = 1;
            if (count + static_cast<int>(push.size()) > Piston::MAX_PUSH) return false;
            while (Piston::sticky(state))
            {
                const glm::ivec3 back = origin - dir * count;
                const BlockId previous = state;
                state = at(back);
                if (Piston::replaceable(state) || !Piston::sticksTo(previous, state) || !Piston::movable(state) ||
                    Piston::fragile(state) || back == piston || has(back))
                    break;
                if (++count + static_cast<int>(push.size()) > Piston::MAX_PUSH) return false;
            }
            for (int j = count - 1; j >= 0; --j) push.push_back(origin - dir * j);

            for (int k = 1;; ++k)
            {
                const glm::ivec3 next = origin + dir * k;
                if (has(next)) return true;
                const BlockId ahead = at(next);
                if (Piston::replaceable(ahead)) return true;
                if (next == piston || !Piston::movable(ahead)) return false;
                if (Piston::fragile(ahead))
                {
                    if (std::find(destroy.begin(), destroy.end(), next) == destroy.end()) destroy.push_back(next);
                    return true;
                }
                if (full()) return false;
                push.push_back(next);
            }
        }

        bool branches(glm::ivec3 from)
        {
            const BlockId self = at(from);
            for (const glm::ivec3& side : SIX)
            {
                if ((side.x != 0 && dir.x != 0) || (side.y != 0 && dir.y != 0) || (side.z != 0 && dir.z != 0)) continue;
                const glm::ivec3 next = from + side;
                if (Piston::sticksTo(at(next), self) && !line(next)) return false;
            }
            return true;
        }

        bool resolve(const glm::ivec3& start, bool extending)
        {
            const BlockId first = at(start);
            if (Piston::replaceable(first)) return true;
            if (!Piston::movable(first)) return false;
            if (Piston::fragile(first))
            {
                if (!extending) return false;
                destroy.push_back(start);
                return true;
            }
            if (!line(start)) return false;
            for (size_t i = 0; i < push.size(); ++i)
                if (Piston::sticky(at(push[i])) && !branches(push[i])) return false;
            return true;
        }

        void apply(Redstone::Cells& out, Piston::Moved* moved) const
        {
            for (const glm::ivec3& p : destroy)
            {
                if (moved) moved->broken.emplace_back(p, out.at(p.x, p.y, p.z));
                out.set(p.x, p.y, p.z, Blocks::Air);
            }
            std::vector<BlockId> carried;
            carried.reserve(push.size());
            for (const glm::ivec3& p : push) carried.push_back(out.at(p.x, p.y, p.z));
            for (const glm::ivec3& p : push) out.set(p.x, p.y, p.z, Blocks::Air);
            for (size_t i = 0; i < push.size(); ++i)
            {
                const glm::ivec3 to = push[i] + dir;
                out.set(to.x, to.y, to.z, carried[i]);
                if (moved) moved->cells.push_back(to);
            }
            if (moved) moved->direction = dir;
        }
    };
}

glm::ivec3 Piston::offset(int facing)
{
    return SIX[facing < 0 || facing > 5 ? 0 : facing];
}

void Piston::step(int facing, int& dx, int& dz)
{
    const glm::ivec3 o = offset(facing);
    dx = o.x;
    dz = o.z;
}

bool Piston::replaceable(BlockId id)
{
    return id == Blocks::Air || isLiquid(id);
}

bool Piston::sticky(BlockId id)
{
    return id == Blocks::SlimeBlock || id == Blocks::HoneyBlock;
}

bool Piston::sticksTo(BlockId a, BlockId b)
{
    if ((a == Blocks::SlimeBlock && b == Blocks::HoneyBlock) || (a == Blocks::HoneyBlock && b == Blocks::SlimeBlock)) return false;
    return sticky(a) || sticky(b);
}

bool Piston::movable(BlockId id)
{
    if (replaceable(id)) return false;
    if (blockInfo(id).hardness < 0.0f) return false;
    if (pistonOut(id) || isPistonHead(id)) return false;
    if (isChestFamily(id) || isDoor(id) || isBed(id) || isFurnaceBlock(id)) return false;
    if (isContainerBlock(id) || isShulkerBox(id) || isHopper(id) || isDispenser(id) || isDropper(id)) return false;
    if (isBarrel(id) || isLectern(id) || id == Blocks::Obsidian || id == Blocks::CryingObsidian ||
        isRespawnAnchor(id) || id == Blocks::ReinforcedDeepslate || id == Blocks::EndPortalFrame || id == Blocks::EnchantingTable ||
        id == Blocks::Beacon || id == Blocks::Jukebox || id == Blocks::Lectern || id == Blocks::BrewingStand || id == Blocks::Conduit ||
        id == Blocks::MonsterSpawner || id == Blocks::SculkShrieker || id == Blocks::SculkCatalyst || id == Blocks::SculkCatalystBloom ||
        id == Blocks::NetherPortal || id == Blocks::EndPortal || id == Blocks::EndGateway)
        return false;
    if (isHive(id) || isDaylightDetector(id) || isCommandBlock(id) || isStructureBlock(id) || isSculkSensor(id) || isCrafter(id) ||
        isTrialSpawner(id) || isVault(id) || isChiseledBookshelf(id) || isCampfire(id) || isSign(id) || isHangingSign(id) ||
        isBanner(id) || isDecoratedPot(id))
        return false;
    return true;
}

bool Piston::fragile(BlockId id)
{
    if (replaceable(id) || !movable(id)) return false;
    if (isRail(id)) return false;
    if (isHead(id) || isCake(id) || isCandle(id) || isFlowerPot(id) || isLantern(id) || isBell(id) || isAmethystGrowth(id) ||
        isPointedDripstone(id) || isTurtleEgg(id) || isCocoa(id) || isCarpet(id) || isLadder(id) || isSnowLayer(id))
        return true;
    if (id == Blocks::Pumpkin || id == Blocks::Melon || isCarvedPumpkin(id) || isJackOLantern(id) || id == Blocks::DragonEgg ||
        id == Blocks::Cactus || id == Blocks::Cobweb)
        return true;
    const BlockInfo& info = blockInfo(id);
    return !info.collides || info.render == RenderType::Cross;
}

int Piston::lineLength(const Redstone::Cells& cells, int x, int y, int z, int facing)
{
    const glm::ivec3 here(x, y, z);
    Resolver r{ cells, here, offset(facing), {}, {} };
    if (!r.resolve(here + offset(facing), true)) return -1;
    return static_cast<int>(r.push.size());
}

bool Piston::extend(Redstone::Cells& cells, int x, int y, int z, Moved* moved)
{
    const BlockId body = cells.at(x, y, z);
    if (!isPistonBody(body) || pistonOut(body)) return false;

    const int facing = pistonFacing(body);
    const glm::ivec3 here(x, y, z);
    const glm::ivec3 d = offset(facing);
    Resolver r{ cells, here, d, {}, {} };
    if (!r.resolve(here + d, true)) return false;

    r.apply(cells, moved);
    const glm::ivec3 head = here + d;
    cells.set(head.x, head.y, head.z, pistonHead(facing, pistonSticky(body)));
    cells.set(x, y, z, pistonBody(facing, pistonSticky(body), true));
    if (moved)
    {
        moved->direction = d;
        moved->cells.push_back(head);
    }
    return true;
}

bool Piston::retract(Redstone::Cells& cells, int x, int y, int z, Moved* moved)
{
    const BlockId body = cells.at(x, y, z);
    if (!isPistonBody(body) || !pistonOut(body)) return false;

    const int facing = pistonFacing(body);
    const glm::ivec3 here(x, y, z);
    const glm::ivec3 d = offset(facing);
    const bool stuck = pistonSticky(body);

    const glm::ivec3 head = here + d;
    if (isPistonHead(cells.at(head.x, head.y, head.z))) cells.set(head.x, head.y, head.z, Blocks::Air);
    cells.set(x, y, z, pistonBody(facing, stuck, false));

    if (stuck)
    {
        Resolver r{ cells, here, -d, {}, {} };
        if (r.resolve(here + d * 2, false)) r.apply(cells, moved);
    }
    return true;
}
