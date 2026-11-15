#include "Fire.h"
#include "RecipeBook.h"
#include <string>
#include <unordered_map>
#include <utility>

namespace
{
    struct Flame
    {
        const char* name;
        int burn;
        int spread;
    };

#include "FireData.inl"

    const std::unordered_map<BlockId, std::pair<int, int>>& table()
    {
        static const std::unordered_map<BlockId, std::pair<int, int>> TABLE = [] {
            std::unordered_map<std::string, std::pair<int, int>> byName;
            for (const Flame& flame : FLAMMABLE) byName.emplace(flame.name, std::make_pair(flame.burn, flame.spread));
            std::unordered_map<BlockId, std::pair<int, int>> out;
            for (int id = 1; isBlockId(id); id = nextBlockId(id))
            {
                const auto found = byName.find(RecipeBook::vanillaIdOf(static_cast<StackId>(id)));
                if (found != byName.end()) out.emplace(static_cast<BlockId>(id), found->second);
            }
            return out;
        }();
        return TABLE;
    }
}

int Fire::burnChance(BlockId id)
{
    const auto found = table().find(id);
    return found == table().end() ? 0 : found->second.first;
}

int Fire::spreadChance(BlockId id)
{
    const auto found = table().find(id);
    return found == table().end() ? 0 : found->second.second;
}
