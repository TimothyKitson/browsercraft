#pragma once
#include "Chunk.h"
#include "Noise.h"
#include "Village.h"
#include "TrialChamber.h"
#include "Stronghold.h"

#include "Dimension.h"
const char* dimensionFolder(Dimension dimension);

// Which tree is being planted. Spruce grows tall and narrow in the
// cold, dark oak short and broad in the thick of the forest, which is
// most of what tells one wood from another at a distance.
enum class TreeKind : uint8_t { Oak, Birch, Spruce, DarkOak, Jungle, Acacia };

enum class Biome : uint8_t
{
    Ocean,
    Beach,
    Plains,
    Forest,
    Desert,
    Mountains,
    Snowy,

    // Hot and wet, and hot and dry but not desert: the two that used to
    // come out as forest and plains wherever the sun was strongest.
    Jungle,
    Savanna
};

const char* biomeName(Biome biome);

// Terrain generation. Every method is const and uses only the seeded noise
// tables, so worker threads can generate different chunks in parallel from
// a single shared instance.
class WorldGen
{
public:
    // Which sort of cave a column stands over. The whole of a cave's
    // character in one value, read off low-frequency noise so a cave
    // keeps the same character for a few hundred blocks rather than
    // changing every chunk.
    enum class CaveKind : uint8_t { Plain, Dripstone, Lush };
    CaveKind caveKindAt(int worldX, int worldZ) const;

    static constexpr int SEA_LEVEL = 62;
    // The lava sea the Nether is built around, and the height the End's
    // central plateau is centred on. Public because spawning somewhere
    // survivable in either dimension has to know where the ground is.
    static constexpr int NETHER_LAVA_LEVEL = 31;


    // How high a fortress's walkways run above the lava, and how far
    // down a mineshaft's corridors are.
    static constexpr int FORTRESS_FLOOR = NETHER_LAVA_LEVEL + 12;
    static constexpr int MINESHAFT_MIN = 14;
    static constexpr int MINESHAFT_MAX = 44;
    static constexpr int END_ISLAND_LEVEL = 58;

    WorldGen(uint32_t seed, Dimension dimension = Dimension::Overworld);

    Dimension dimension() const { return m_dimension; }

    void generate(Chunk& chunk) const;

    int surfaceHeight(int worldX, int worldZ) const;
    Biome biomeAt(int worldX, int worldZ) const;

    // The nearest village to a place, searching outwards a few squares.
    // Public because a dev screen and a command both want to find one,
    // and because walking to the nearest village is exactly the thing
    // a player will ask for first.
    Village::Site nearestVillage(int fromX, int fromZ, int squares = 3) const;

    // The nearest trial chamber, the same way. Squares are bigger and
    // chambers rarer, so the default search is wider.
    TrialChamber::Site nearestChamber(int fromX, int fromZ, int squares = 3) const;

    // The stronghold nearest a place. There is always one: they sit on
    // rings round the origin so an eye of ender always has something
    // to point at.
    Stronghold::Site nearestStronghold(int fromX, int fromZ) const;

    // Why a site was turned down, for --villageinfo. Counting the
    // reasons is the only way to tell "the land is too steep" from
    // "there are no sites at all".
    struct VillageTally
    {
        int sites = 0;
        int wrongBiome = 0;
        int tooWet = 0;
        int tooSteep = 0;
        int built = 0;
    };
    VillageTally surveyVillages(int fromX, int fromZ, int squares) const;

    // Picks a reasonable, dry spawn point near the origin.
    void findSpawn(int& outX, int& outY, int& outZ) const;

private:
    uint32_t m_seed;
    Dimension m_dimension;
    Noise m_height;
    Noise m_hills;
    Noise m_mountains;
    Noise m_temperature;
    Noise m_humidity;
    Noise m_caves;
    Noise m_caves2;

    float temperatureAt(int x, int z) const;
    float humidityAt(int x, int z) const;

    void generateColumns(Chunk& chunk) const;

    // Villages. Each is wider than a chunk, so every chunk it reaches
    // into lays out the whole thing and stamps only its own part --
    // which works because the layout depends on nothing but the seed.
    void buildVillages(Chunk& chunk) const;
    void buildTrialChambers(Chunk& chunk) const;
    void buildStrongholds(Chunk& chunk) const;

    // The Nether's own structure: a bridge and a keep of nether brick
    // over the lava, with a blaze cage in it.
    void buildNetherFortresses(Chunk& chunk) const;

    // And the overworld's oldest one: corridors of plank and fence
    // with rails down them, placed by the spread rule rather than the
    // grid -- two mineshafts are allowed to meet, and do.
    void buildMineshafts(Chunk& chunk) const;
    void stampStrongholdRoom(Chunk& chunk, const Stronghold::Site& site,
                             const Stronghold::Room& room) const;
    void stampChamberRoom(Chunk& chunk, const TrialChamber::Site& site,
                          const TrialChamber::Room& room) const;
    void layPaths(Chunk& chunk, const Village::Site& site,
                  int fromX, int fromZ, int toX, int toZ) const;
    // Whether the ground where this village would stand could carry
    // it. Decided from the site alone, so every chunk agrees.
    bool villageFits(const Village::Site& site) const;
    // What a village is built out of, which is decided by where it
    // stands. A desert village in oak planks looks imported.
    // Minecraft's desert villages have flat roofs with a parapet, and
    // its taiga and snowy ones pitch theirs more steeply than the
    // plains do. A sandstone gable looked like an oak house painted
    // yellow.
    //
    // A steeper pitch was tried for the snow and taken out again: two
    // blocks of rise a step put ten blocks of roof over a four-block
    // wall and the houses read as stone mounds. The claim that snow
    // country pitches steeper came from the weaker sources anyway.
    // Snow differs by what it is built of instead.
    enum class RoofStyle { Gable, Flat };

    struct Palette
    {
        RoofStyle style = RoofStyle::Gable;
        BlockId wall = Blocks::Planks;
        BlockId footing = Blocks::Cobblestone;
        BlockId corner = Blocks::Log;
        BlockId floor = Blocks::Planks;
        BlockId roof = Blocks::Planks;      // the ridge row
        // Stairs come in fours, north first, so the facing is added to
        // this rather than looked up.
        BlockId roofStairs = Blocks::PlankStairsNorth;
        BlockId post = Blocks::Log;         // lamp posts and well frames
    };

    Palette paletteFor(Biome biome) const;

    void stampBuilding(Chunk& chunk, const Village::Building& building,
                       const Palette& palette) const;
    void stampInterior(Chunk& chunk, const Village::Building& building, int ground,
                       int x0, int z0, int x1, int z1, int doorX, int doorZ,
                       const Palette& palette) const;
    void carveCaves(Chunk& chunk) const;
    void layDeepslate(Chunk& chunk) const;

    // What a cave is made of where it is not just grey. Run after the
    // ores, so a seam cut through by a cave is dressed rather than
    // dressed over.
    void dressCaves(Chunk& chunk) const;
    void placeOres(Chunk& chunk) const;
    void decorate(Chunk& chunk) const;

    void generateNether(Chunk& chunk) const;
    void generateEnd(Chunk& chunk) const;

    void buildTree(Chunk& chunk, int worldX, int surfaceY, int worldZ, TreeKind kind) const;
};
