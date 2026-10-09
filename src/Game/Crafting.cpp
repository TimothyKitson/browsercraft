#include "Crafting.h"
#include "Player/Inventory.h"
#include "Items.h"
#include <algorithm>

namespace
{
    constexpr StackId _ = Blocks::Air;   // an empty cell, readably

    struct Recipe
    {
        bool shapeless;
        int width, height;        // shaped only
        StackId pattern[9];       // shaped: row-major; shapeless: the ingredient list
        int inputCount;           // shapeless only
        StackId result;
        int resultCount;
    };

    // The recipe book is still short: there are items now, but no
    // sticks, ingots or tools, so most of Minecraft's recipes have no
    // ingredients to be made from yet.
    const Recipe RECIPES[] = {
        // One log of either wood becomes four planks.
        { true,  0, 0, { Blocks::Log },      1, Blocks::Planks,    4 },
        { true,  0, 0, { Blocks::BirchLog }, 1, Blocks::Planks,    4 },

        // Three wheat becomes a loaf. Minecraft wants them in a row,
        // which needs a bench this game has not got yet, so it takes
        // them in any arrangement for the same three wheat.
        { true,  0, 0, { Items::Wheat, Items::Wheat, Items::Wheat }, 3, Items::Bread, 1 },

        // Four sand in a square becomes one sandstone.
        { false, 2, 2, { Blocks::Sand, Blocks::Sand,
                         Blocks::Sand, Blocks::Sand }, 0, Blocks::Sandstone, 1 },
    };

    // Shrinks the used area of the grid down to its bounding box, so a
    // recipe placed in the bottom-right corner still matches.
    bool trim(const ItemStack* grid, int size, int& outX, int& outY, int& outW, int& outH)
    {
        int minX = size, minY = size, maxX = -1, maxY = -1;

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                if (!grid[y * 3 + x].empty())
                {
                    minX = std::min(minX, x); maxX = std::max(maxX, x);
                    minY = std::min(minY, y); maxY = std::max(maxY, y);
                }

        if (maxX < 0) return false;   // nothing in the grid

        outX = minX; outY = minY;
        outW = maxX - minX + 1;
        outH = maxY - minY + 1;
        return true;
    }

    bool matchesShaped(const Recipe& recipe, const ItemStack* grid, int size)
    {
        int x0 = 0, y0 = 0, width = 0, height = 0;
        if (!trim(grid, size, x0, y0, width, height)) return false;
        if (width != recipe.width || height != recipe.height) return false;

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const ItemStack& cell = grid[(y0 + y) * 3 + (x0 + x)];
                const StackId wanted = recipe.pattern[y * recipe.width + x];
                if (wanted == _ && !cell.empty()) return false;
                if (wanted != _ && (cell.empty() || cell.id != wanted)) return false;
            }

        return true;
    }

    bool matchesShapeless(const Recipe& recipe, const ItemStack* grid, int size)
    {
        // Every filled cell has to be accounted for by exactly one
        // ingredient, and every ingredient has to be found.
        bool used[9] = { false };
        for (int i = 0; i < recipe.inputCount; ++i)
        {
            bool found = false;
            for (int y = 0; y < size && !found; ++y)
                for (int x = 0; x < size && !found; ++x)
                {
                    const int cell = y * 3 + x;
                    if (used[cell]) continue;
                    if (grid[cell].empty() || grid[cell].id != recipe.pattern[i]) continue;
                    used[cell] = true;
                    found = true;
                }
            if (!found) return false;
        }

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const int cell = y * 3 + x;
                if (!grid[cell].empty() && !used[cell]) return false;
            }

        return true;
    }
}

namespace Crafting
{
    CraftOutput match(const ItemStack* grid, int size)
    {
        for (const Recipe& recipe : RECIPES)
        {
            const bool hit = recipe.shapeless ? matchesShapeless(recipe, grid, size)
                                              : matchesShaped(recipe, grid, size);
            if (hit) return CraftOutput{ recipe.result, recipe.resultCount };
        }
        return CraftOutput{};
    }

    void consume(ItemStack* grid, int size)
    {
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                ItemStack& cell = grid[y * 3 + x];
                if (cell.empty()) continue;
                cell.count -= 1;
                if (cell.count <= 0) cell.clear();
            }
    }
}
