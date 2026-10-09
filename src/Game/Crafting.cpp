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

        // Four planks make the bench, which is the only way to reach a
        // 3x3 grid and so the only way to reach anything three wide.
        { false, 2, 2, { Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, Blocks::Planks }, 0, Blocks::CraftingTable, 1 },

        // Eight cobblestone round an empty middle make a furnace.
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone,
                         Blocks::Cobblestone, _,                   Blocks::Cobblestone,
                         Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::Furnace, 1 },

        // Eight planks round an empty middle make a chest.
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks,
                         Blocks::Planks, _,              Blocks::Planks,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Chest, 1 },

        // Five gunpowder and four sand, crossed, makes TNT.
        { false, 3, 3, { Items::Gunpowder, Blocks::Sand,     Items::Gunpowder,
                         Blocks::Sand,     Items::Gunpowder, Blocks::Sand,
                         Items::Gunpowder, Blocks::Sand,     Items::Gunpowder },
          0, Blocks::Tnt, 1 },

        // Three of a block in a row makes six slabs of it.
        { false, 3, 1, { Blocks::Stone, Blocks::Stone, Blocks::Stone },
          0, Blocks::StoneSlab, 6 },
        { false, 3, 1, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone },
          0, Blocks::CobblestoneSlab, 6 },
        { false, 3, 1, { Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::PlankSlab, 6 },
        { false, 3, 1, { Blocks::Sandstone, Blocks::Sandstone, Blocks::Sandstone },
          0, Blocks::SandstoneSlab, 6 },

        // Three sticks and three string make a bow.
        { false, 3, 3, { _,                Items::Stick,      Items::StringItem,
                         Items::Stick,     _,                 Items::StringItem,
                         _,                Items::Stick,      Items::StringItem },
          0, Items::Bow, 1 },

        // Flint, stick and feather make four arrows.
        { false, 1, 3, { Items::Flint,
                         Items::Stick,
                         Items::Feather }, 0, Items::Arrow, 4 },

        // Coal over a stick makes four torches, which is the recipe the
        // whole early game turns on -- there was no way to light a cave
        // before it.
        { false, 1, 2, { Items::Coal,
                         Items::Stick }, 0, Blocks::Torch, 4 },

        // Four string makes a block of wool, so a bed does not depend on
        // finding sheep.
        { false, 2, 2, { Items::StringItem, Items::StringItem,
                         Items::StringItem, Items::StringItem }, 0, Blocks::Wool, 1 },

        // Three wool over three planks makes a bed.
        { false, 3, 2, { Blocks::Wool,   Blocks::Wool,   Blocks::Wool,
                         Blocks::Planks, Blocks::Planks, Blocks::Planks },
          0, Blocks::Bed, 1 },

        // Two planks, one above the other, make four sticks.
        { false, 1, 2, { Blocks::Planks,
                         Blocks::Planks }, 0, Items::Stick, 4 },


        // Armour. Every piece is its material beaten into shape;
        // all four are three wide, so all four want the bench.
        { false, 3, 2, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherHelmet, 1 },
        { false, 3, 3, { Items::Leather, _, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather, Items::Leather }, 0, Items::LeatherChestplate, 1 },
        { false, 3, 3, { Items::Leather, Items::Leather, Items::Leather, Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherLeggings, 1 },
        { false, 3, 2, { Items::Leather, _, Items::Leather, Items::Leather, _, Items::Leather }, 0, Items::LeatherBoots, 1 },

        { false, 3, 2, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldHelmet, 1 },
        { false, 3, 3, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot }, 0, Items::GoldChestplate, 1 },
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldLeggings, 1 },
        { false, 3, 2, { Items::GoldIngot, _, Items::GoldIngot, Items::GoldIngot, _, Items::GoldIngot }, 0, Items::GoldBoots, 1 },

        { false, 3, 2, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronHelmet, 1 },
        { false, 3, 3, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot }, 0, Items::IronChestplate, 1 },
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronLeggings, 1 },
        { false, 3, 2, { Items::IronIngot, _, Items::IronIngot, Items::IronIngot, _, Items::IronIngot }, 0, Items::IronBoots, 1 },

        { false, 3, 2, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondHelmet, 1 },
        { false, 3, 3, { Items::Diamond, _, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond }, 0, Items::DiamondChestplate, 1 },
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondLeggings, 1 },
        { false, 3, 2, { Items::Diamond, _, Items::Diamond, Items::Diamond, _, Items::Diamond }, 0, Items::DiamondBoots, 1 },

        // Tools. Every one is its head material over a stick or two,
        // which is why none of them could be made before the bench --
        // all but the shovel and the sword are wider than a 2x2 grid.
        // Wood
        { false, 3, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::WoodPickaxe, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, Blocks::Planks, Items::Stick, _, Items::Stick }, 0, Items::WoodAxe, 1 },
        { false, 1, 3, { Blocks::Planks, Items::Stick, Items::Stick }, 0, Items::WoodShovel, 1 },
        { false, 1, 3, { Blocks::Planks, Blocks::Planks, Items::Stick }, 0, Items::WoodSword, 1 },
        { false, 2, 3, { Blocks::Planks, Blocks::Planks, _, Items::Stick, _, Items::Stick }, 0, Items::WoodHoe, 1 },

        // Stone
        { false, 3, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::StonePickaxe, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick, _, Items::Stick }, 0, Items::StoneAxe, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Items::Stick, Items::Stick }, 0, Items::StoneShovel, 1 },
        { false, 1, 3, { Blocks::Cobblestone, Blocks::Cobblestone, Items::Stick }, 0, Items::StoneSword, 1 },
        { false, 2, 3, { Blocks::Cobblestone, Blocks::Cobblestone, _, Items::Stick, _, Items::Stick }, 0, Items::StoneHoe, 1 },

        // Iron
        { false, 3, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::IronPickaxe, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, Items::IronIngot, Items::Stick, _, Items::Stick }, 0, Items::IronAxe, 1 },
        { false, 1, 3, { Items::IronIngot, Items::Stick, Items::Stick }, 0, Items::IronShovel, 1 },
        { false, 1, 3, { Items::IronIngot, Items::IronIngot, Items::Stick }, 0, Items::IronSword, 1 },
        { false, 2, 3, { Items::IronIngot, Items::IronIngot, _, Items::Stick, _, Items::Stick }, 0, Items::IronHoe, 1 },

        // Gold
        { false, 3, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::GoldPickaxe, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, Items::GoldIngot, Items::Stick, _, Items::Stick }, 0, Items::GoldAxe, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::Stick, Items::Stick }, 0, Items::GoldShovel, 1 },
        { false, 1, 3, { Items::GoldIngot, Items::GoldIngot, Items::Stick }, 0, Items::GoldSword, 1 },
        { false, 2, 3, { Items::GoldIngot, Items::GoldIngot, _, Items::Stick, _, Items::Stick }, 0, Items::GoldHoe, 1 },

        // Diamond
        { false, 3, 3, { Items::Diamond, Items::Diamond, Items::Diamond, _, Items::Stick, _, _, Items::Stick, _ }, 0, Items::DiamondPickaxe, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, Items::Diamond, Items::Stick, _, Items::Stick }, 0, Items::DiamondAxe, 1 },
        { false, 1, 3, { Items::Diamond, Items::Stick, Items::Stick }, 0, Items::DiamondShovel, 1 },
        { false, 1, 3, { Items::Diamond, Items::Diamond, Items::Stick }, 0, Items::DiamondSword, 1 },
        { false, 2, 3, { Items::Diamond, Items::Diamond, _, Items::Stick, _, Items::Stick }, 0, Items::DiamondHoe, 1 },
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
