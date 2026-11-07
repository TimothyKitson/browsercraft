#pragma once
#include "Items.h"
#include "World/Block.h"
#include <map>
#include <string>
#include <vector>

struct ItemStack;

namespace RecipeBook
{
    enum class Book : uint8_t { Crafting, Furnace, BlastFurnace, Smoker };
    enum class Group : uint8_t { Search, Building, Redstone, Equipment, Misc, Food, Blocks };

    constexpr int PER_PAGE = 20;
    constexpr float CYCLE_SECONDS = 1.5f;

    struct Recipe
    {
        bool shapeless = false;
        int width = 0;
        int height = 0;
        std::vector<std::vector<StackId>> cells;
        StackId result = Blocks::Air;
        int count = 1;
        int family = -1;
        Group group = Group::Misc;
    };

    const std::vector<Recipe>& recipes(Book book);
    std::vector<Group> tabs(Book book);
    std::vector<StackId> tabIcons(Book book, Group group);
    char mojangGroup(const std::string& id);
    std::string idOf(StackId id);
    Group craftingGroup(StackId result);
    Group cookingGroup(Book book, StackId result);
    bool fits(const Recipe& recipe, int gridSize);

    using Stock = std::map<StackId, int>;
    Stock stockOf(const ItemStack* slots, int count);
    bool allocate(const Recipe& recipe, const Stock& stock, int times, std::vector<StackId>& picks);
    int timesCraftable(const Recipe& recipe, const Stock& stock, int limit);
    std::vector<StackId> fuels();

    struct Slot
    {
        std::vector<int> recipes;
        int craftable = 0;
    };
    std::vector<Slot> slots(Book book, Group tab, int gridSize, const std::string& search, bool craftableOnly,
                            const Stock& stock);
    int cellIndex(const Recipe& recipe, int cell, int gridSize);
}
