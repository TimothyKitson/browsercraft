#include "BlockDrops.h"
#include "Tools.h"
#include <algorithm>
#include <initializer_list>

namespace BlockDrops
{
    namespace
    {
        uint32_t step(uint32_t& state)
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }

        int below(uint32_t& state, int bound)
        {
            return bound <= 1 ? 0 : static_cast<int>(step(state) % static_cast<uint32_t>(bound));
        }

        float unit(uint32_t& state) { return static_cast<float>(step(state) >> 8) / 16777216.0f; }

        bool tableBonus(std::initializer_list<float> chances, int fortune, uint32_t& state)
        {
            const int last = static_cast<int>(chances.size()) - 1;
            const float chance = *(chances.begin() + std::clamp(fortune, 0, last));
            return unit(state) < chance;
        }

        bool isOreDropSeam(BlockId seam)
        {
            return seam == Blocks::CoalOre || seam == Blocks::CopperOre || seam == Blocks::DiamondOre || seam == Blocks::EmeraldOre ||
                   seam == Blocks::GoldOre || seam == Blocks::IronOre || seam == Blocks::LapisOre || seam == Blocks::NetherGoldOre ||
                   seam == Blocks::NetherQuartzOre;
        }

        bool isWeepingOrTwisting(BlockId id)
        {
            return id == Blocks::WeepingVines || id == Blocks::WeepingVinesPlant || id == Blocks::TwistingVines ||
                   id == Blocks::TwistingVinesPlant;
        }

        bool isGrassyDouble(BlockId id)
        {
            return isDoublePlant(id) && (doublePlantKind(id) == DoublePlant::TallGrass || doublePlantKind(id) == DoublePlant::LargeFern);
        }
    }

    int oreDrops(int count, int fortune, uint32_t& state)
    {
        if (fortune <= 0) return count;
        const int bonus = std::max(0, below(state, fortune + 2) - 1);
        return count * (bonus + 1);
    }

    int uniformBonus(int count, int fortune, int multiplier, uint32_t& state)
    {
        return count + below(state, multiplier * std::max(0, fortune) + 1);
    }

    int binomialBonus(int count, int fortune, int extra, float probability, uint32_t& state)
    {
        for (int i = 0; i < std::max(0, fortune) + extra; ++i)
            if (unit(state) < probability) ++count;
        return count;
    }

    bool handles(BlockId id)
    {
        const BlockId seam = oreBase(id);
        return isLeaves(id) || id == Blocks::FloweringAzaleaLeaves || id == Blocks::Gravel || id == Blocks::GildedBlackstone ||
               id == Blocks::Glowstone || id == Blocks::SeaLantern || id == Blocks::Melon || seam == Blocks::RedstoneOre ||
               isOreDropSeam(seam) || id == Blocks::AmethystCluster || id == Blocks::TallGrass || id == Blocks::Fern ||
               isGrassyDouble(id) || id == Blocks::BrownMushroomBlock || id == Blocks::RedMushroomBlock || id == Blocks::DeadBush ||
               isWeepingOrTwisting(id) || isNetherWart(id) || isSweetBerryBush(id) || isCocoa(id);
    }

    std::vector<Drop> roll(BlockId id, const Breaker& breaker, uint32_t seed)
    {
        std::vector<Drop> out;
        uint32_t state = seed ? seed : 1u;
        for (int i = 0; i < 3; ++i) step(state);
        const int fortune = breaker.silk ? 0 : std::max(0, breaker.fortune);
        const bool shears = breaker.tool == Items::Shears;
        auto add = [&](StackId what, int count) {
            if (count > 0 && what != Blocks::Air) out.push_back(Drop{ what, count });
        };
        const BlockId seam = oreBase(id);

        if (isLeaves(id) || id == Blocks::FloweringAzaleaLeaves)
        {
            if (shears || breaker.silk)
            {
                add(id, 1);
                return out;
            }
            StackId sapling = Blocks::Air;
            bool jungle = false;
            if (id == Blocks::AzaleaLeaves) sapling = Blocks::Azalea;
            else if (id == Blocks::FloweringAzaleaLeaves) sapling = Blocks::FloweringAzalea;
            else
            {
                const int wood = woodOfLeaves(id);
                if (wood >= 0 && id != Blocks::MangroveLeaves) sapling = woodSet(wood).sapling;
                jungle = id == Blocks::JungleLeaves;
            }
            if (sapling != Blocks::Air)
            {
                const bool fell = jungle ? tableBonus({ 0.025f, 0.027777778f, 0.03125f, 0.041666668f, 0.1f }, fortune, state)
                                         : tableBonus({ 0.05f, 0.0625f, 0.083333336f, 0.1f }, fortune, state);
                if (fell) add(sapling, 1);
            }
            if (tableBonus({ 0.02f, 0.022222223f, 0.025f, 0.033333335f, 0.1f }, fortune, state)) add(Items::Stick, 1 + below(state, 2));
            if ((id == Blocks::Leaves || id == Blocks::DarkOakLeaves) &&
                tableBonus({ 0.005f, 0.0055555557f, 0.00625f, 0.008333334f, 0.025f }, fortune, state))
                add(Items::Apple, 1);
            return out;
        }

        if (id == Blocks::Gravel)
        {
            if (!breaker.silk && tableBonus({ 0.1f, 0.14285715f, 0.25f, 1.0f }, fortune, state)) add(Items::Flint, 1);
            else add(Blocks::Gravel, 1);
            return out;
        }
        if (id == Blocks::GildedBlackstone)
        {
            if (!breaker.silk && tableBonus({ 0.1f, 0.14285715f, 0.25f, 1.0f }, fortune, state)) add(Items::GoldNugget, 2 + below(state, 4));
            else add(Blocks::GildedBlackstone, 1);
            return out;
        }
        if (breaker.silk && (id == Blocks::Glowstone || id == Blocks::SeaLantern || id == Blocks::Melon || seam == Blocks::RedstoneOre ||
                             isOreDropSeam(seam) || id == Blocks::AmethystCluster || id == Blocks::BrownMushroomBlock ||
                             id == Blocks::RedMushroomBlock))
        {
            add(isObtainable(id) ? id : seam, 1);
            return out;
        }
        if (id == Blocks::Glowstone)
        {
            add(Items::GlowstoneDust, std::clamp(uniformBonus(2 + below(state, 3), fortune, 1, state), 1, 4));
            return out;
        }
        if (id == Blocks::SeaLantern)
        {
            add(Items::PrismarineCrystals, std::clamp(uniformBonus(2 + below(state, 2), fortune, 1, state), 1, 5));
            return out;
        }
        if (id == Blocks::Melon)
        {
            add(Items::MelonSlice, std::min(uniformBonus(3 + below(state, 5), fortune, 1, state), 9));
            return out;
        }
        if (seam == Blocks::RedstoneOre)
        {
            add(Items::Redstone, uniformBonus(4 + below(state, 2), fortune, 1, state));
            return out;
        }
        if (isOreDropSeam(seam))
        {
            int base = 1;
            StackId what = Blocks::Air;
            switch (seam)
            {
                case Blocks::CoalOre: what = Items::Coal; break;
                case Blocks::CopperOre: what = Items::RawCopper; base = 2 + below(state, 4); break;
                case Blocks::DiamondOre: what = Items::Diamond; break;
                case Blocks::EmeraldOre: what = Items::Emerald; break;
                case Blocks::GoldOre: what = Items::RawGold; break;
                case Blocks::IronOre: what = Items::RawIron; break;
                case Blocks::LapisOre: what = Items::LapisLazuli; base = 4 + below(state, 6); break;
                case Blocks::NetherGoldOre: what = Items::GoldNugget; base = 2 + below(state, 5); break;
                case Blocks::NetherQuartzOre: what = Items::NetherQuartz; break;
                default: break;
            }
            add(what, oreDrops(base, fortune, state));
            return out;
        }
        if (id == Blocks::AmethystCluster)
        {
            if (Tools::kindOf(breaker.tool) == Tools::Kind::Pickaxe) add(Items::AmethystShard, oreDrops(4, fortune, state));
            else add(Items::AmethystShard, 2);
            return out;
        }
        if (id == Blocks::TallGrass || id == Blocks::Fern)
        {
            if (shears) add(id, 1);
            else if (unit(state) < 0.125f) add(Items::WheatSeeds, uniformBonus(1, fortune, 2, state));
            return out;
        }
        if (isGrassyDouble(id))
        {
            if (doublePlantUpper(id)) return out;
            const bool fern = doublePlantKind(id) == DoublePlant::LargeFern;
            if (shears) add(fern ? static_cast<StackId>(Blocks::Fern) : static_cast<StackId>(Blocks::TallGrass), 2);
            else if (unit(state) < 0.125f) add(Items::WheatSeeds, 1);
            return out;
        }
        if (id == Blocks::BrownMushroomBlock || id == Blocks::RedMushroomBlock)
        {
            add(id == Blocks::BrownMushroomBlock ? static_cast<StackId>(Blocks::BrownMushroom) : static_cast<StackId>(Blocks::RedMushroom),
                std::max(0, below(state, 9) - 6));
            return out;
        }
        if (id == Blocks::DeadBush)
        {
            if (shears) add(Blocks::DeadBush, 1);
            else add(Items::Stick, below(state, 3));
            return out;
        }
        if (isWeepingOrTwisting(id))
        {
            const StackId vine = (id == Blocks::WeepingVines || id == Blocks::WeepingVinesPlant) ? static_cast<StackId>(Blocks::WeepingVines)
                                                                                                  : static_cast<StackId>(Blocks::TwistingVines);
            if (shears || breaker.silk || tableBonus({ 0.33f, 0.55f, 0.77f, 1.0f }, fortune, state)) add(vine, 1);
            return out;
        }
        if (isNetherWart(id))
        {
            add(Items::NetherWart, id == Blocks::NetherWart ? uniformBonus(2 + below(state, 3), fortune, 1, state) : 1);
            return out;
        }
        if (isSweetBerryBush(id))
        {
            const int stage = sweetBerryStage(id);
            if (stage == 3) add(Items::SweetBerries, uniformBonus(2 + below(state, 2), fortune, 1, state));
            else if (stage == 2) add(Items::SweetBerries, uniformBonus(1 + below(state, 2), fortune, 1, state));
            return out;
        }
        if (isCocoa(id))
        {
            add(Items::CocoaBeans, cocoaStage(id) >= 2 ? 3 : 1);
            return out;
        }
        return out;
    }
}
