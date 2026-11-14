#include "RecipeBook.h"
#include "Discs.h"
#include <cstring>
#include "Crafting.h"
#include "Smelting.h"
#include "Food.h"
#include "Tools.h"
#include "Armour.h"
#include "Player/Inventory.h"
#include <algorithm>
#include <cctype>

namespace
{
    using RecipeBook::Book;
    using RecipeBook::Group;
    using RecipeBook::Recipe;

    constexpr int PLANKS_FAMILY = 100000;
    constexpr int DYE_FAMILY = 200000;
    constexpr int CHEST_BOAT_FAMILY = 300000;
    constexpr int COOKING_FAMILY = 400000;

    std::vector<StackId> allPlanks()
    {
        std::vector<StackId> out;
        for (int wood = 0; wood < WOOD_COUNT; ++wood)
        {
            const StackId planks = planksOfWood(wood);
            if (std::find(out.begin(), out.end(), planks) == out.end()) out.push_back(planks);
        }
        return out;
    }

    std::vector<StackId> optionsFor(StackId wanted, StackId result)
    {
        if (wanted == Blocks::Air) return {};
        if (wanted == Blocks::Planks) return allPlanks();
        if (wanted == Items::Coal && result != Blocks::CoalBlock) return { Items::Coal, Items::Charcoal };
        return { wanted };
    }

    bool woodFamily(StackId result)
    {
        return result == Items::Boat || result == Blocks::DoorLowShutNorth || result == Blocks::OakTrapdoor || result == Items::Sign ||
               result == Blocks::PlankSlab || result == Blocks::PlankStairsNorth || result == Blocks::OakFence ||
               result == gateOf(0, false, 0) || result == buttonOf(2, 0, 0, false) || result == plateOf(4, false);
    }

    StackId inWood(StackId result, int wood)
    {
        if (result == Items::Boat) return wood < BOAT_WOODS ? boatOfWood(wood, false) : static_cast<StackId>(Blocks::Air);
        if (result == Blocks::DoorLowShutNorth) return doorBaseOf(wood);
        if (result == Blocks::OakTrapdoor) return trapdoorBaseOf(wood);
        if (result == Items::Sign) return signItemOfWood(wood);
        if (wood == 0) return result;
        if (result == Blocks::PlankSlab)
        {
            for (int m = 0; m < SLAB_MATERIALS; ++m)
                if (slabMaterialWood(m) == wood) return slabOf(m, 0);
            return static_cast<StackId>(Blocks::Air);
        }
        if (result == Blocks::PlankStairsNorth)
        {
            for (int m = 0; m < STAIRS_MATERIALS; ++m)
                if (stairsMaterialWood(m) == wood) return stairsOf(m, false, 0);
            return static_cast<StackId>(Blocks::Air);
        }
        if (result == Blocks::OakFence) return fenceOfWood(wood);
        if (result == gateOf(0, false, 0)) return gateOf(wood, false, 0);
        if (result == buttonOf(2, 0, 0, false)) return buttonOf(2 + wood, 0, 0, false);
        if (result == plateOf(4, false)) return plateOf(4 + wood, false);
        return result;
    }

    std::vector<std::vector<StackId>> ring(StackId around, StackId middle)
    {
        std::vector<std::vector<StackId>> cells(9, std::vector<StackId>{ around });
        cells[4] = { middle };
        return cells;
    }

    Recipe shaped(int width, int height, std::vector<std::vector<StackId>> cells, StackId result, int count, int family)
    {
        Recipe recipe;
        recipe.shapeless = false;
        recipe.width = width;
        recipe.height = height;
        recipe.cells = std::move(cells);
        recipe.result = result;
        recipe.count = count;
        recipe.family = family;
        recipe.group = RecipeBook::craftingGroup(result);
        return recipe;
    }

    Recipe shapeless(std::vector<std::vector<StackId>> cells, StackId result, int count, int family)
    {
        Recipe recipe = shaped(0, 0, std::move(cells), result, count, family);
        recipe.shapeless = true;
        return recipe;
    }

    std::vector<Recipe> buildCrafting()
    {
        std::vector<Recipe> out;
        for (int i = 0; i < Crafting::tableSize(); ++i)
        {
            const Crafting::TableRecipe table = Crafting::tableRecipe(i);
            if (table.result == Blocks::Air || table.resultCount <= 0) continue;
            const int cellCount = table.shapeless ? table.inputCount : table.width * table.height;
            bool usesPlanks = false;
            for (int c = 0; c < cellCount; ++c) usesPlanks = usesPlanks || table.pattern[c] == Blocks::Planks;
            const bool isPlankResult = isBlockStack(table.result) && isPlanks(asBlock(table.result));
            const int family = isPlankResult ? PLANKS_FAMILY : i;
            if (woodFamily(table.result) && usesPlanks)
            {
                for (int wood = 0; wood < WOOD_COUNT; ++wood)
                {
                    const StackId result = inWood(table.result, wood);
                    if (result == Blocks::Air) continue;
                    std::vector<std::vector<StackId>> cells;
                    for (int c = 0; c < cellCount; ++c)
                        cells.push_back(table.pattern[c] == Blocks::Planks ? std::vector<StackId>{ planksOfWood(wood) }
                                                                          : optionsFor(table.pattern[c], table.result));
                    out.push_back(table.shapeless ? shapeless(cells, result, table.resultCount, family)
                                                  : shaped(table.width, table.height, cells, result, table.resultCount, family));
                }
                continue;
            }
            std::vector<std::vector<StackId>> cells;
            for (int c = 0; c < cellCount; ++c) cells.push_back(optionsFor(table.pattern[c], table.result));
            out.push_back(table.shapeless ? shapeless(cells, table.result, table.resultCount, family)
                                          : shaped(table.width, table.height, cells, table.result, table.resultCount, family));
        }

        std::vector<StackId> wools;
        for (int colour = 0; colour < DYE_COLOURS; ++colour) wools.push_back(woolOfColour(colour));
        for (int colour = 0; colour < DYE_COLOURS; ++colour)
        {
            const StackId dye = dyeOfColour(colour);
            std::vector<StackId> others;
            for (StackId wool : wools)
                if (wool != woolOfColour(colour)) others.push_back(wool);
            out.push_back(shapeless({ { dye }, others }, woolOfColour(colour), 1, DYE_FAMILY + 0));
            out.push_back(shaped(3, 3, ring(Blocks::Terracotta, dye), colourIn(Blocks::TerracottaFirst, colour), 8, DYE_FAMILY + 1));
            out.push_back(shaped(3, 3, ring(Blocks::Glass, dye), colourIn(Blocks::StainedGlassFirst, colour), 8, DYE_FAMILY + 2));
            out.push_back(shaped(3, 3, ring(Blocks::GlassPane, dye), colourIn(Blocks::StainedGlassPaneFirst, colour), 8, DYE_FAMILY + 3));
            out.push_back(shapeless({ { dye }, { Blocks::Sand }, { Blocks::Sand }, { Blocks::Sand }, { Blocks::Sand },
                                      { Blocks::Gravel }, { Blocks::Gravel }, { Blocks::Gravel }, { Blocks::Gravel } },
                                    colourIn(Blocks::ConcretePowderFirst, colour), 8, DYE_FAMILY + 4));
            out.push_back(shapeless({ { Blocks::Candle }, { dye } }, candleOf(colour + 1, 1, false), 1, DYE_FAMILY + 5));
        }
        for (int wood = 0; wood < BOAT_WOODS; ++wood)
            out.push_back(shapeless({ { Blocks::Chest }, { boatOfWood(wood, false) } }, boatOfWood(wood, true), 1, CHEST_BOAT_FAMILY));
        std::vector<std::vector<StackId>> frame(9, std::vector<StackId>{ Items::Stick });
        frame[4] = wools;
        out.push_back(shaped(3, 3, frame, Items::Painting, 1, CHEST_BOAT_FAMILY + 1));
        return out;
    }

    Smelting::Kind kindOf(Book book)
    {
        return book == Book::Smoker ? Smelting::Kind::Smoker
             : book == Book::BlastFurnace ? Smelting::Kind::BlastFurnace
                                          : Smelting::Kind::Furnace;
    }

    std::vector<Recipe> buildCooking(Book book)
    {
        std::vector<Recipe> out;
        auto consider = [&](StackId input) {
            if (!Smelting::suits(kindOf(book), input)) return;
            const StackId result = Smelting::resultOf(input);
            if (result == Blocks::Air) return;
            Recipe recipe;
            recipe.shapeless = true;
            recipe.cells = { { input } };
            recipe.result = result;
            recipe.count = 1;
            recipe.family = COOKING_FAMILY + static_cast<int>(result);
            recipe.group = RecipeBook::cookingGroup(book, result);
            out.push_back(recipe);
        };
        for (int block = 1; isBlockId(block); block = nextBlockId(block)) consider(static_cast<StackId>(block));
        for (StackId item = Items::FIRST; item < Items::Count; ++item) consider(item);
        return out;
    }
}

namespace
{
    struct VanillaName
    {
        const char* id;
        const char* name;
    };
    const VanillaName VANILLA_NAMES[] = {
        { "acacia_chest_boat", "Acacia Boat with Chest" },
        { "amethyst_block", "Block of Amethyst" },
        { "axolotl_bucket", "Bucket of Axolotl" },
        { "bamboo_block", "Block of Bamboo" },
        { "bamboo_chest_raft", "Bamboo Raft with Chest" },
        { "bamboo_sapling", "Bamboo Shoot" },
        { "beef", "Raw Beef" },
        { "birch_chest_boat", "Birch Boat with Chest" },
        { "black_candle_cake", "Cake with Black Candle" },
        { "blue_candle_cake", "Cake with Blue Candle" },
        { "brown_candle_cake", "Cake with Brown Candle" },
        { "candle_cake", "Cake with Candle" },
        { "cherry_chest_boat", "Cherry Boat with Chest" },
        { "chest_minecart", "Minecart with Chest" },
        { "chicken", "Raw Chicken" },
        { "coal_block", "Block of Coal" },
        { "cod", "Raw Cod" },
        { "cod_bucket", "Bucket of Cod" },
        { "command_block_minecart", "Minecart with Command Block" },
        { "comparator", "Redstone Comparator" },
        { "cooked_beef", "Steak" },
        { "copper_block", "Block of Copper" },
        { "cyan_candle_cake", "Cake with Cyan Candle" },
        { "dark_oak_chest_boat", "Dark Oak Boat with Chest" },
        { "deepslate_lapis_ore", "Deepslate Lapis Lazuli Ore" },
        { "diamond_block", "Block of Diamond" },
        { "disc_fragment_5", "Disc Fragment" },
        { "dragon_breath", "Dragon's Breath" },
        { "emerald_block", "Block of Emerald" },
        { "ender_eye", "Eye of Ender" },
        { "experience_bottle", "Bottle o' Enchanting" },
        { "filled_map", "Map" },
        { "furnace_minecart", "Minecart with Furnace" },
        { "gold_block", "Block of Gold" },
        { "gray_candle_cake", "Cake with Gray Candle" },
        { "green_candle_cake", "Cake with Green Candle" },
        { "hay_block", "Hay Bale" },
        { "hopper_minecart", "Minecart with Hopper" },
        { "iron_block", "Block of Iron" },
        { "jack_o_lantern", "Jack o'Lantern" },
        { "jigsaw", "Jigsaw Block" },
        { "jungle_chest_boat", "Jungle Boat with Chest" },
        { "lapis_block", "Block of Lapis Lazuli" },
        { "lapis_ore", "Lapis Lazuli Ore" },
        { "leather_chestplate", "Leather Tunic" },
        { "leather_helmet", "Leather Cap" },
        { "leather_leggings", "Leather Pants" },
        { "light_blue_candle_cake", "Cake with Light Blue Candle" },
        { "light_gray_candle_cake", "Cake with Light Gray Candle" },
        { "lime_candle_cake", "Cake with Lime Candle" },
        { "magenta_candle_cake", "Cake with Magenta Candle" },
        { "mangrove_chest_boat", "Mangrove Boat with Chest" },
        { "map", "Empty Map" },
        { "mutton", "Raw Mutton" },
        { "netherite_block", "Block of Netherite" },
        { "oak_chest_boat", "Oak Boat with Chest" },
        { "orange_candle_cake", "Cake with Orange Candle" },
        { "pink_candle_cake", "Cake with Pink Candle" },
        { "porkchop", "Raw Porkchop" },
        { "potted_azalea_bush", "Potted Azalea" },
        { "potted_flowering_azalea_bush", "Potted Flowering Azalea" },
        { "pottery_shard_archer", "Archer Pottery Shard" },
        { "pottery_shard_arms_up", "Arms Up Pottery Shard" },
        { "pottery_shard_prize", "Prize Pottery Shard" },
        { "pottery_shard_skull", "Skull Pottery Shard" },
        { "pufferfish_bucket", "Bucket of Pufferfish" },
        { "purple_candle_cake", "Cake with Purple Candle" },
        { "quartz", "Nether Quartz" },
        { "quartz_block", "Block of Quartz" },
        { "rabbit", "Raw Rabbit" },
        { "rabbit_foot", "Rabbit's Foot" },
        { "raw_copper_block", "Block of Raw Copper" },
        { "raw_gold_block", "Block of Raw Gold" },
        { "raw_iron_block", "Block of Raw Iron" },
        { "red_candle_cake", "Cake with Red Candle" },
        { "redstone", "Redstone Dust" },
        { "redstone_block", "Block of Redstone" },
        { "repeater", "Redstone Repeater" },
        { "salmon", "Raw Salmon" },
        { "salmon_bucket", "Bucket of Salmon" },
        { "set_spawn", "Respawn point set" },
        { "slime_ball", "Slimeball" },
        { "smooth_quartz", "Smooth Quartz Block" },
        { "spawner", "Monster Spawner" },
        { "spruce_chest_boat", "Spruce Boat with Chest" },
        { "stripped_bamboo_block", "Block of Stripped Bamboo" },
        { "tadpole_bucket", "Bucket of Tadpole" },
        { "tnt_minecart", "Minecart with TNT" },
        { "tropical_fish_bucket", "Bucket of Tropical Fish" },
        { "turtle_helmet", "Turtle Shell" },
        { "vine", "Vines" },
        { "waxed_copper_block", "Waxed Block of Copper" },
        { "wheat", "Wheat Crops" },
        { "white_candle_cake", "Cake with White Candle" },
        { "writable_book", "Book and Quill" },
        { "yellow_candle_cake", "Cake with Yellow Candle" }
    };
}

std::string RecipeBook::vanillaIdOf(StackId id)
{
    if (Discs::discOf(id) >= 0) return std::string("music_disc_") + Discs::song(Discs::discOf(id)).key;
    if (id >= Items::NetheriteUpgradeTemplate && id <= Items::BoltTemplate) return idOf(id) + "_smithing_template";
    static const std::pair<StackId, const char*> PATTERNS[] = {
        { Items::FlowerBannerPattern, "flower_banner_pattern" }, { Items::CreeperBannerPattern, "creeper_banner_pattern" },
        { Items::SkullBannerPattern, "skull_banner_pattern" }, { Items::MojangBannerPattern, "mojang_banner_pattern" },
        { Items::PiglinBannerPattern, "piglin_banner_pattern" },
    };
    for (const auto& pattern : PATTERNS)
        if (pattern.first == id) return pattern.second;
    const char* shown = displayName(id);
    if (shown)
        for (const VanillaName& entry : VANILLA_NAMES)
            if (std::strcmp(entry.name, shown) == 0) return entry.id;
    return idOf(id);
}

StackId RecipeBook::stackForVanillaId(const std::string& id)
{
    if (id.rfind("music_disc_", 0) == 0)
        for (int disc = 0; disc < Discs::COUNT; ++disc)
            if (id.substr(11) == Discs::song(disc).key) return static_cast<StackId>(Items::MusicDiscFirst + disc);
    for (const VanillaName& entry : VANILLA_NAMES)
        if (id == entry.id)
        {
            for (int block = 1; isBlockId(block); block = nextBlockId(block))
                if (displayName(static_cast<StackId>(block)) && std::strcmp(displayName(static_cast<StackId>(block)), entry.name) == 0)
                    return static_cast<StackId>(block);
            for (StackId item = Items::FIRST; item < Items::Count; ++item)
                if (displayName(item) && std::strcmp(displayName(item), entry.name) == 0) return item;
        }
    return Blocks::Air;
}

std::string RecipeBook::idOf(StackId id)
{
    std::string out;
    for (const char* c = displayName(id); c && *c; ++c)
    {
        const unsigned char ch = static_cast<unsigned char>(*c);
        if (std::isalnum(ch)) out.push_back(static_cast<char>(std::tolower(ch)));
        else if (ch == ' ' || ch == '-') out.push_back('_');
    }
    return out;
}

RecipeBook::Group RecipeBook::craftingGroup(StackId result)
{
    const std::string id = vanillaIdOf(result);
    const char mojang = mojangGroup(id);
    auto ends = [&](const char* suffix) {
        const std::string tail(suffix);
        return id.size() >= tail.size() && id.compare(id.size() - tail.size(), tail.size(), tail) == 0;
    };
    char group = mojang;
    if (!group)
    {
        if (ends("_door") || ends("_trapdoor") || ends("_button") || ends("_pressure_plate") || ends("_fence_gate")) group = 'R';
        else if (ends("_sign") || ends("_fence") || ends("_wall") || ends("_boat") || ends("_raft")) group = 'M';
        else if (ends("_slab") || ends("_stairs") || ends("_planks")) group = 'B';
    }
    if (!group)
    {
        if (Tools::kindOf(result) != Tools::Kind::None || Armour::isArmour(result) || result == Items::Bow ||
            result == Items::Crossbow || result == Items::Shield || result == Items::Arrow)
            group = 'E';
        else if (isBlockStack(result)) group = 'B';
        else group = 'M';
    }
    switch (group)
    {
        case 'B': return Group::Building;
        case 'R': return Group::Redstone;
        case 'E': return Group::Equipment;
        default: return Group::Misc;
    }
}

RecipeBook::Group RecipeBook::cookingGroup(Book book, StackId result)
{
    if (book == Book::Smoker) return Group::Food;
    if (book == Book::Furnace && Food::isEdible(result)) return Group::Food;
    return isBlockStack(result) ? Group::Blocks : Group::Misc;
}

const std::vector<RecipeBook::Recipe>& RecipeBook::recipes(Book book)
{
    static const std::vector<Recipe> crafting = buildCrafting();
    static const std::vector<Recipe> furnace = buildCooking(Book::Furnace);
    static const std::vector<Recipe> blast = buildCooking(Book::BlastFurnace);
    static const std::vector<Recipe> smoker = buildCooking(Book::Smoker);
    switch (book)
    {
        case Book::Furnace: return furnace;
        case Book::BlastFurnace: return blast;
        case Book::Smoker: return smoker;
        default: return crafting;
    }
}

std::vector<RecipeBook::Group> RecipeBook::tabs(Book book)
{
    switch (book)
    {
        case Book::Furnace: return { Group::Search, Group::Food, Group::Blocks, Group::Misc };
        case Book::BlastFurnace: return { Group::Search, Group::Blocks, Group::Misc };
        case Book::Smoker: return { Group::Search, Group::Food };
        default: return { Group::Search, Group::Building, Group::Redstone, Group::Equipment, Group::Misc };
    }
}

std::vector<StackId> RecipeBook::tabIcons(Book book, Group group)
{
    switch (group)
    {
        case Group::Search: return { Items::Compass };
        case Group::Building: return { Blocks::Bricks };
        case Group::Redstone: return { Items::Redstone };
        case Group::Equipment: return { Items::IronAxe, Items::GoldSword };
        case Group::Food: return { Items::RawPorkchop };
        case Group::Blocks: return { book == Book::BlastFurnace ? static_cast<StackId>(Blocks::IronOre) : static_cast<StackId>(Blocks::Stone) };
        case Group::Misc:
            if (book == Book::Furnace) return { Items::LavaBucket, Items::Emerald };
            if (book == Book::BlastFurnace) return { Items::IronShovel, Items::GoldLeggings };
            return { Items::LavaBucket, Items::Apple };
    }
    return {};
}

bool RecipeBook::fits(const Recipe& recipe, int gridSize)
{
    if (recipe.shapeless) return static_cast<int>(recipe.cells.size()) <= gridSize * gridSize;
    return recipe.width <= gridSize && recipe.height <= gridSize;
}

RecipeBook::Stock RecipeBook::stockOf(const ItemStack* slots, int count)
{
    Stock stock;
    for (int i = 0; i < count; ++i)
        if (!slots[i].empty() && slots[i].damage == 0 && slots[i].enchantments == 0) stock[slots[i].id] += slots[i].count;
    return stock;
}

bool RecipeBook::allocate(const Recipe& recipe, const Stock& stock, int times, std::vector<StackId>& picks)
{
    Stock left = stock;
    picks.assign(recipe.cells.size(), Blocks::Air);
    for (size_t c = 0; c < recipe.cells.size(); ++c)
    {
        const std::vector<StackId>& options = recipe.cells[c];
        if (options.empty()) continue;
        StackId best = Blocks::Air;
        int most = 0;
        for (StackId option : options)
        {
            const auto found = left.find(option);
            const int have = found == left.end() ? 0 : found->second;
            if (have >= times && have > most)
            {
                best = option;
                most = have;
            }
        }
        if (best == Blocks::Air) return false;
        left[best] -= times;
        picks[c] = best;
    }
    return true;
}

int RecipeBook::timesCraftable(const Recipe& recipe, const Stock& stock, int limit)
{
    std::vector<StackId> picks;
    int times = 0;
    while (times < limit && allocate(recipe, stock, times + 1, picks)) ++times;
    return times;
}

std::vector<StackId> RecipeBook::fuels()
{
    std::vector<StackId> out;
    for (int block = 1; isBlockId(block); block = nextBlockId(block))
        if (Smelting::isFuel(static_cast<StackId>(block))) out.push_back(static_cast<StackId>(block));
    for (StackId item = Items::FIRST; item < Items::Count; ++item)
        if (Smelting::isFuel(item)) out.push_back(item);
    return out;
}

std::vector<RecipeBook::Slot> RecipeBook::slots(Book book, Group tab, int gridSize, const std::string& search,
                                                bool craftableOnly, const Stock& stock, const std::set<StackId>* known)
{
    std::string wanted = search;
    for (char& c : wanted) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const std::vector<Recipe>& all = recipes(book);
    std::vector<Slot> out;
    std::map<int, size_t> byFamily;
    for (int i = 0; i < static_cast<int>(all.size()); ++i)
    {
        const Recipe& recipe = all[static_cast<size_t>(i)];
        if (tab != Group::Search && recipe.group != tab) continue;
        if (book == Book::Crafting && !fits(recipe, gridSize)) continue;
        if (known && !known->count(recipe.result)) continue;
        if (!wanted.empty())
        {
            std::string name = displayName(recipe.result);
            for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (name.find(wanted) == std::string::npos) continue;
        }
        const bool can = timesCraftable(recipe, stock, 1) > 0;
        if (craftableOnly && !can) continue;
        const auto found = byFamily.find(recipe.family);
        if (found == byFamily.end())
        {
            byFamily[recipe.family] = out.size();
            out.push_back(Slot{});
        }
        Slot& slot = out[byFamily[recipe.family]];
        if (can)
        {
            slot.recipes.insert(slot.recipes.begin() + slot.craftable, i);
            ++slot.craftable;
        }
        else
            slot.recipes.push_back(i);
    }
    return out;
}

int RecipeBook::cellIndex(const Recipe& recipe, int cell, int gridSize)
{
    if (recipe.shapeless) return (cell / std::max(1, gridSize)) * 3 + cell % std::max(1, gridSize);
    const int width = std::max(1, recipe.width);
    return (cell / width) * 3 + cell % width;
}

StackId RecipeBook::stackNamed(const std::string& id)
{
    static const std::map<std::string, StackId> NAMES = [] {
        std::map<std::string, StackId> out;
        for (int block = 1; isBlockId(block); block = nextBlockId(block))
            out.emplace(idOf(static_cast<StackId>(block)), static_cast<StackId>(block));
        for (StackId item = Items::FIRST; item < Items::Count; ++item) out.emplace(idOf(item), item);
        static const std::pair<const char*, StackId> ALIASES[] = {
            { "grass_block", Blocks::Grass }, { "oak_planks", Blocks::Planks }, { "crafting_table", Blocks::CraftingTable },
            { "ender_eye", Items::EyeOfEnder }, { "dragon_breath", Items::DragonsBreath }, { "hay_block", Blocks::HayBale },
            { "wooden_pickaxe", Items::WoodPickaxe }, { "golden_carrot", Items::GoldenCarrot }, { "potion", Items::Potion },
            { "map", Items::EmptyMap }, { "cod", Items::RawCod }, { "salmon", Items::RawSalmon }, { "beef", Items::RawBeef },
            { "porkchop", Items::RawPorkchop }, { "chicken", Items::RawChicken }, { "mutton", Items::RawMutton },
            { "rabbit", Items::RawRabbit }, { "cooked_beef", Items::Steak }, { "red_bed", static_cast<StackId>(Blocks::BedFirst) },
            { "oak_boat", Items::Boat }, { "white_banner", static_cast<StackId>(Blocks::BannerFirst) },
            { "lava_bucket", Items::LavaBucket }, { "water_bucket", Items::WaterBucket }, { "milk_bucket", Items::MilkBucket },
            { "pufferfish_bucket", Items::PufferfishBucket }, { "axolotl_bucket", Items::AxolotlBucket },
            { "tropical_fish_bucket", Items::TropicalFishBucket }, { "tadpole_bucket", Items::TadpoleBucket },
            { "cod_bucket", Items::CodBucket }, { "salmon_bucket", Items::SalmonBucket }, { "dragon_head", static_cast<StackId>(Blocks::HeadRunFirst + 100) },
            { "creeper_banner_pattern", Items::CreeperBannerPattern }, { "skull_banner_pattern", Items::SkullBannerPattern },
            { "flower_banner_pattern", Items::FlowerBannerPattern }, { "piglin_banner_pattern", Items::PiglinBannerPattern }, { "mojang_banner_pattern", Items::MojangBannerPattern },
        };
        for (const auto& alias : ALIASES) out[alias.first] = alias.second;
        return out;
    }();
    const auto found = NAMES.find(id);
    if (found != NAMES.end()) return found->second;
    if (const StackId named = stackForVanillaId(id); named != Blocks::Air) return named;
    const std::string tail = "_smithing_template";
    if (id.size() > tail.size() && id.compare(id.size() - tail.size(), tail.size(), tail) == 0)
        return stackNamed(id.substr(0, id.size() - tail.size()));
    return Blocks::Air;
}

