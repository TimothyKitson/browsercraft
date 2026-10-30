#pragma once
#include "Chunk.h"
#include "Noise.h"
#include "Village.h"
#include "TrialChamber.h"
#include "Stronghold.h"
#include "Structures.h"

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
    Savanna,

    // Low, warm and wet: the flats just above the sea where the water
    // stands in the hollows rather than draining off them.
    Swamp
};

const char* biomeName(Biome biome);

// Which froglight a frog that grew up here leaves behind. Minecraft
// decides it by the climate the tadpole grew up in: warm gives the
// pearl, cold gives the green, and everywhere in between gives the
// ochre one.
BlockId froglightFor(Biome biome);

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
    // The small things Minecraft scatters across the surface: one
    // building, one biome, one chest. They share a stamp because they
    // are the same shape of problem -- a box of a particular material
    // put down where the land suits it.
    enum class Landmark : uint8_t
    {
        DesertPyramid,
        JungleTemple,
        Igloo,
        RuinedPortal,
        PillagerOutpost,
        WoodlandMansion,
        SwampHut,
        Count
    };

    // Where a landmark keeps its chests, as offsets from the middle of
    // it -- dy from the ground under it. One table rather than two
    // copies of the same arithmetic: the stamp reads it to put the
    // chests down and the loot reads it to recognise one later.
    struct ChestSpot { int dx, dy, dz; };
    static int landmarkChests(Landmark kind, ChestSpot out[2]);

    // What kind of thing a chest found standing in the world belongs
    // to. Worked out from where it is rather than stored, like
    // everything else here, so a chest holds the same thing however
    // many times the chunk round it is thrown away and remade.
    enum class Hoard : uint8_t
    {
        None,
        DesertPyramid,
        JungleTemple,
        Igloo,
        RuinedPortal,
        PillagerOutpost,
        WoodlandMansion,
        Shipwreck,
        BuriedTreasure,
        Monument,
        Mineshaft,
        Stronghold,
        AncientCity,
        SwampHut,
        EndCity,
        Bastion,
        TrialChamber,
    };

    Hoard hoardAt(int x, int y, int z) const;

    static const char* landmarkName(Landmark kind);

    // Which grid it goes down on, and whether this biome will have it.
    static const Structures::Grid& landmarkGrid(Landmark kind);
    static bool landmarkSuits(Landmark kind, Biome biome);

    // Whether one can actually stand at a place: the right land under
    // it and out of the sea. The search and the stamp both read this,
    // so they cannot disagree about where a structure is.
    bool landmarkStands(Landmark kind, int midX, int midZ) const;

    // The nearest one of a kind, for the screens that go and look at
    // them. Empty when there is none within the squares searched.
    // Where the nearest shipwreck and monument are, which is what the
    // debug screen reports and what the tests walk to.
    Structures::Spot nearestShipwreck(int blockX, int blockZ, int squares = 4) const;

    // Whether a ship could have come to rest here: on the sea floor,
    // or run aground on a beach. Anything further inland is a ship in
    // a field, which is a different game.
    bool shipwreckFits(int midX, int midZ) const;
    Structures::Spot nearestMonument(int blockX, int blockZ, int squares = 4) const;

    // Whether the sea floor here is deep enough and flat enough for a
    // monument to stand on, which is the only thing keeping one out of
    // a puddle.
    bool monumentFits(int midX, int midZ) const;

    Structures::Spot nearestLandmark(Landmark kind, int blockX, int blockZ,
                                     int squares = 4) const;

    enum class CaveKind : uint8_t { Plain, Dripstone, Lush };
    CaveKind caveKindAt(int worldX, int worldZ) const;

    static constexpr int SEA_LEVEL = 62;

    // A monument is the one building here wide enough that where it
    // will fit is a real question: it needs this much water over the
    // whole of its footprint before it is allowed to stand.
    //
    // Minecraft's is 58 blocks tall and its oceans are deep enough for
    // that. These are not: the deepest sea floor anywhere in this
    // world is nineteen blocks down, measured rather than guessed. So
    // the monument is built to the water it has -- as tall as will fit
    // under the surface, between these two -- instead of to
    // Minecraft's dimensions, which would leave a prismarine roof
    // standing proud of the waves.
    static constexpr int MONUMENT_HALF = 11;
    static constexpr int MONUMENT_DEPTH = 12;
    static constexpr int MONUMENT_MIN_TALL = 8;
    static constexpr int MONUMENT_MAX_TALL = 14;

    // How tall the one at this place comes out.
    int monumentHeight(int midX, int midZ) const;

    // Coral only grows in warm water, which is what keeps a reef out
    // of the sea off a snowfield.
    static constexpr float CORAL_WARMTH = 0.18f;
    bool reefWater(int worldX, int worldZ) const;

    // The outpost's height, which the chest table needs as well as the
    // stamp: the chest is three down from the top of the tower.
    static constexpr int OUTPOST_TALL = 11;

    // The mansion's half-width. It is the widest thing that stands on
    // land here, so it is what sets the reach the landmark pass needs.
    static constexpr int MANSION_HALF = 11;

    // The ancient city, which is the one thing here that lives below
    // the ores. Minecraft puts it at the bottom of the deep dark; this
    // world's floor is y=0, so it sits just above the bedrock.
    // A geode: a hollow ball of amethyst in a shell of calcite and
    // smooth basalt. Minecraft puts them anywhere below the surface;
    // these sit in the band between the ores and the deep, which is
    // where somebody digging will actually run into one.
    static constexpr float GEODE_CHANCE = 0.006f;
    static constexpr uint32_t GEODE_SALT = 55123u;
    static constexpr int GEODE_MIN = 16;
    static constexpr int GEODE_MAX = 46;

    static constexpr int CITY_FLOOR = 8;
    static constexpr int CITY_HALF = 20;
    static constexpr int CITY_TALL = 14;

    // Where the nearest one is, which is what the debug screen reports
    // and what the tests walk to.
    Structures::Spot nearestAncientCity(int blockX, int blockZ, int squares = 4) const;

    // The End's city and the Nether's bastion. Both have to ask about
    // the ground as they go -- an End city needs an island under it
    // and most of the End is void -- so both search past the squares
    // they cannot use, the way the two sea ones do.
    Structures::Spot nearestEndCity(int blockX, int blockZ, int squares = 6) const;
    Structures::Spot nearestBastion(int blockX, int blockZ, int squares = 4) const;

    // The top of the End island under a place, or -1 for open void.
    int endGroundAt(int worldX, int worldZ) const;

    // How cold a place has to be for snow to lie on it and for the
    // water in it to freeze. One number, read by the biome test and
    // by the fill that lays the ice, so a frozen sea is always the
    // sea beside a snowfield rather than somewhere else entirely.
    static constexpr float SNOW_TEMPERATURE = -0.45f;

    // Where ancient debris hides, and how thinly. Minecraft puts it
    // between eight and twenty-two and never lets it touch air; the
    // band here is the same shape against this Nether's lava level.
    static constexpr int ANCIENT_DEBRIS_LOW = 8;
    static constexpr int ANCIENT_DEBRIS_HIGH = 22;
    static constexpr float ANCIENT_DEBRIS_CHANCE = 0.0016f;

    static constexpr int END_CITY_TALL = 26;

    // Half the tower's footprint. The top storey hangs one block out
    // past this, so anything asking "am I at the city" wants one more.
    static constexpr int END_CITY_HALF = 5;

    // How far out the outer islands start. A city inside this is on
    // the island you arrive on, where Minecraft puts none either.
    static constexpr long long END_CITY_VOID = 460;

    // The ring of obsidian pillars round the middle of the main
    // island, each with a crystal on top that heals the dragon.
    // Minecraft has ten; so does this.
    static constexpr int END_PILLARS = 10;
    static constexpr int END_PILLAR_RING = 43;

    // Where the nth pillar stands and how tall it is. Pure arithmetic
    // over the index, so the dragon and the generator agree about
    // where the crystals are without either telling the other.
    static void endPillar(int index, int& outX, int& outZ, int& outTall);

    bool endCityStands(int midX, int midZ) const;

    // Which of a bastion and a fortress a square drew. Public because
    // the test has to be able to ask: it was being asked in two places
    // with two different answers.
    bool bastionWins(int chunkX, int chunkZ) const;
    static constexpr int BASTION_HALF = 13;

    // The mineshaft spread, shared by the stamp and by the lookup that
    // decides what is in the chest at the end of a corridor.
    static constexpr float MINESHAFT_CHANCE = 0.012f;
    static constexpr uint32_t MINESHAFT_SALT = 445731u;
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

    // Whether a tree grows out of this column. Public so the spawn
    // finder and --selftest can ask the same question the decorator
    // asks, rather than each keeping its own copy of the dice.
    bool treeStandsAt(int wx, int wz) const;

    // Whether the water at a place freezes over. Asked by the fill
    // that lays the ice and by the audit that walks out looking for
    // some: one source of truth, so a walk that finds nowhere cold
    // means there is nowhere cold rather than that the walk and the
    // generator disagreed about what cold means.
    bool freezesAt(int worldX, int worldZ) const;

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
    void buildLandmarks(Chunk& chunk) const;
    void stampLandmark(Chunk& chunk, Landmark kind, int midX, int midZ) const;
    void stampSwampHut(Chunk& chunk, int midX, int midZ, int ground) const;
    void hangVines(Chunk& chunk, int worldX, int ground, int worldZ) const;
    void hangCocoa(Chunk& chunk, int worldX, int ground, int worldZ) const;
    void dressSeabed(Chunk& chunk) const;
    void buildDesertWells(Chunk& chunk) const;
    void buildGeodes(Chunk& chunk) const;
    void buildAncientCities(Chunk& chunk) const;
    void buildEndCities(Chunk& chunk) const;
    void buildEndPillars(Chunk& chunk) const;
    void stampEndCity(Chunk& chunk, int midX, int midZ, int ground) const;
    void buildBastions(Chunk& chunk) const;

    void stampBastion(Chunk& chunk, int midX, int midZ) const;
    void stampAncientCity(Chunk& chunk, int midX, int midZ) const;
    void buildOceanStructures(Chunk& chunk) const;
    void stampShipwreck(Chunk& chunk, int midX, int midZ) const;
    void stampMonument(Chunk& chunk, int midX, int midZ) const;
    void stampBuriedTreasure(Chunk& chunk, int chunkX, int chunkZ) const;
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
