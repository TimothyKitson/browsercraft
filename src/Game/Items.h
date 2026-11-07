#pragma once
#include "World/Block.h"
#include "Renderer/AtlasTiles.h"
#include <cstdint>

// What can sit in an inventory slot: a block, or an item.
//
// Deliberately wider than BlockId. A block id is a single byte because
// the world stores millions of them and doubling that would double the
// memory a loaded world costs; an inventory slot stores a few dozen, so
// it can afford the extra byte it takes to tell a block from an item.
//
// Below Items::FIRST the number means a block and casts straight back to
// a BlockId. From Items::FIRST up it means an item. Because FIRST is
// exactly one past the largest value a BlockId can hold, no block can
// ever grow into the item range.
using StackId = uint16_t;

namespace Items
{
    // Where the items begin. Blocks have everything below this, so
    // this is also the ceiling on how many blocks there can be.
    //
    // It was 256, which was the whole of a uint8_t and so the whole of
    // the block space. Both moved together: a block id is sixteen bits
    // now, and the items start far enough up that running out again is
    // not something anybody has to think about.
    constexpr StackId FIRST = 4096;

    enum : StackId
    {
        WheatSeeds = FIRST,
        Wheat,
        Leather,
        RawBeef,
        RawPorkchop,
        RawChicken,
        RawMutton,
        Feather,
        Bone,
        StringItem,     // "String" alone collides with too much
        Gunpowder,
        Bread,

        // Materials. Coal and diamond come straight out of the ground;
        // the two ingots want a furnace, which is why nothing smelted
        // can be crafted yet even though the recipes are here.
        Steak,
        CookedPorkchop,
        CookedChicken,
        CookedMutton,

        Stick,
        Coal,
        Flint,
        Bow,
        Arrow,
        FlintAndSteel,
        Bucket,
        WaterBucket,
        LavaBucket,
        Diamond,
        IronIngot,
        GoldIngot,

        // Armour, grouped by tier so Armour::pieceOf is arithmetic
        // rather than a switch with sixteen cases. Keep the four pieces
        // in this order within every tier.
        LeatherHelmet, LeatherChestplate, LeatherLeggings, LeatherBoots,
        GoldHelmet, GoldChestplate, GoldLeggings, GoldBoots,
        IronHelmet, IronChestplate, IronLeggings, IronBoots,
        DiamondHelmet, DiamondChestplate, DiamondLeggings, DiamondBoots,

        // Tools, grouped by tier so Tools::tierOf is arithmetic rather
        // than a switch with twenty-five cases in it. Keep the five
        // kinds in this order within every tier.
        WoodPickaxe, WoodAxe, WoodShovel, WoodSword, WoodHoe,
        StonePickaxe, StoneAxe, StoneShovel, StoneSword, StoneHoe,
        IronPickaxe, IronAxe, IronShovel, IronSword, IronHoe,
        GoldPickaxe, GoldAxe, GoldShovel, GoldSword, GoldHoe,
        DiamondPickaxe, DiamondAxe, DiamondShovel, DiamondSword, DiamondHoe,

        // Sugar cane is a block, so it is not here; what it is made
        // into is.
        Paper,
        Book,
        Emerald,
        EnderPearl,
        EyeOfEnder,

        // Brewing. The potion is one id for every drink there is: which
        // one it holds rides in the stack's damage field, the way wear
        // does on a tool, so adding a potion costs no item id and a
        // saved one comes back without the format changing.
        GlassBottle,
        Potion,
        NetherWart,
        Sugar,
        BlazePowder,
        GlowstoneDust,

        // Held up against what is coming at you.
        Shield,

        // Set down on water and got into.
        Boat,
        // Set down on a rail and got into.
        Minecart,

        // What a blaze is carrying. Ground down, it is what every
        // brewing stand and every potion in the game starts from.
        BlazeRod,

        // Netherrack, fired. Four of them make the block a fortress is
        // built of, which is the only thing to build with down there
        // that is not the ground itself.
        NetherBrick,

        // What a magma cube leaves. Brewed, it is the only way to walk
        // through fire, which is what makes the Nether survivable.
        MagmaCream,

        // Redstone dust, in the hand. Put down it becomes the block
        // that carries power along the floor.
        Redstone,

        // What a breeze is made of, and what it leaves. The rod makes
        // the mace; the charges are what it was throwing at you.
        BreezeRod,
        WindCharge,

        // 1.21's weapon. It hits for what it is worth standing still
        // and for a great deal more at the end of a fall, which is the
        // whole of how it is meant to be used.
        Mace,

        // What a trial spawner pays out when its waves are done, and
        // the only thing that opens a vault.
        TrialKey,

        // Copper, out of the ground and out of the furnace.
        RawCopper,
        CopperIngot,

        // The one thing you can eat that grows underground.
        GlowBerries,

        // The hard way round a trial chamber. The bottle is what a
        // vault pays you for doing it the easy way; drinking it turns
        // the next chamber ominous, and an ominous spawner pays the
        // key that opens the vaults worth opening.
        OminousBottle,
        OminousTrialKey,

        // Off an oak, now and then. The one food in the game you get
        // by cutting down a tree rather than by farming or killing.
        Apple,

        // Ground bone. Thrown on a crop it brings it on, which is the
        // difference between a field and a wait.
        BoneMeal,

        // The two other crops. A carrot and a potato are both the
        // seed and the food, which is Minecraft's and saves two item
        // ids: you plant the thing you eat.
        Carrot,
        Potato,
        BakedPotato,

        // Three planks. The one thing in the game that is a container
        // rather than a tool or a food, and the only way to carry a
        // soup about.
        Bowl,

        // The fourth crop, and the one thing you can make of it. A
        // beetroot is barely food; six of them in a bowl is a meal.
        Beetroot,
        BeetrootSeeds,
        BeetrootSoup,

        // A melon is a block, so it is not here; what it is cut into
        // is, and the seed that goes back in the ground.
        MelonSlice,
        MelonSeeds,

        // What a chicken lays, and what a bucket gets out of a cow.
        // Both are here for the cake, which cannot be made without
        // them, and the milk is also the one thing that clears what
        // is on you.
        Egg,
        MilkBucket,

        // Set down and eaten where it stands, which nothing else in
        // the game is.
        Cake,

        // Two wheat and a bean. The bean grows on the side of a
        // jungle trunk and is the only thing in the game that does.
        Cookie,
        CocoaBeans,

        // A board on a wall with words on it. One item, four ways of
        // hanging it, the same as a ladder.
        Sign,

        // A frame hung on a wall to show one thing off, or to say what
        // is in the chest under it.
        ItemFrame,

        // Worn where a chestplate goes and stops nothing. What it does
        // is turn a fall into a flight, which is the only thing in the
        // game that changes how you get anywhere.
        Elytra,

        // Off a shulker. Two of them round a chest make a box that
        // can be carried.
        ShulkerShell,

        // What a ghast leaves. Brewed, it is the only thing in the
        // game that puts hearts back on their own.
        GhastTear,

        // Off a drowned, now and then. The hardest swing in the game
        // short of a mace at the end of a fall. Minecraft throws it
        // as well; nothing here can throw a thing and get it back.
        Trident,

        // The sixteen dyes, in Minecraft's order. White is nought and
        // black is fifteen, and the wool a dye makes shares its
        // number -- which is what lets one recipe cover all sixteen
        // rather than sixteen recipes covering one each.
        DyeWhite, DyeOrange, DyeMagenta, DyeLightBlue, DyeYellow, DyeLime,
        DyePink, DyeGray, DyeLightGray, DyeCyan, DyePurple, DyeBlue,
        DyeBrown, DyeGreen, DyeRed, DyeBlack,

        // Out of the ground, and the only blue there is.
        LapisLazuli,

        // Out of a squid, and the only black there is.
        InkSac,

        // Netherite, and the tier above diamond it makes.
        //
        // The nine tools and pieces are at the end of the list rather
        // than beside the diamond ones they follow, because an item's
        // number is what a saved inventory holds: slotting them in
        // where they belong would have renumbered half the game and
        // emptied every world anybody had. The arithmetic that finds
        // a tier reads them specially instead, which is one `if` a
        // side against breaking every save there is.
        NetheriteScrap,
        NetheriteIngot,

        // What a wither leaves. One of them makes a beacon, which is
        // the only thing in the game it is for.
        NetherStar,

        NetheritePickaxe, NetheriteAxe, NetheriteShovel, NetheriteSword, NetheriteHoe,
        NetheriteHelmet, NetheriteChestplate, NetheriteLeggings, NetheriteBoots,

        // Off an evoker. Held when the blow that would kill you lands,
        // it spends itself instead: Minecraft's one second chance, and
        // the only reason to go into a mansion at all.
        TotemOfUndying,

        // Off a slime. Nine make a block, one makes a piston sticky,
        // and with blaze powder it is magma cream.
        Slimeball,

        // Off a piglin. Nine make an ingot, which is the only thing
        // in the game that turns what a mob leaves into a metal.
        GoldNugget,

        // Out of the sea. Two worth cooking, and one that is worth
        // nothing at all and will make you ill.
        RawCod,
        CookedCod,
        RawSalmon,
        CookedSalmon,
        Pufferfish,

        // Off a rabbit, and the wax off a bee.
        RawRabbit,
        CookedRabbit,
        Honeycomb,

        // Off a snow block, and what a snow golem throws. Four of
        // them pack back into the block they came from.
        Snowball,
        Saddle,
        LeatherHorseArmor,
        IronHorseArmor,
        GoldHorseArmor,
        DiamondHorseArmor,

        SpruceSign, BirchSign, JungleSign, AcaciaSign, DarkOakSign,
        CherrySign,
        MangroveSign,
        BambooSign,
        CrimsonSign,
        WarpedSign,
        NetherQuartz,
        RawIron,
        RawGold,
        AmethystShard,
        TripwireHook,
        TorchflowerSeeds,
        PitcherPod,
        ClayBall,
        Brick,
        PumpkinSeeds,
        Shears,
        HoneyBottle,
        SweetBerries,
        DriedKelp,
        NautilusShell,
        HeartOfTheSea,
        TurtleScute,
        TurtleHelmet,
        PowderSnowBucket,
        IronNugget,
        ChorusFruit,
        PoppedChorusFruit,
        Brush,
        NetheriteUpgradeTemplate,
        SentryTemplate,
        VexTemplate,
        WildTemplate,
        CoastTemplate,
        DuneTemplate,
        WayfinderTemplate,
        RaiserTemplate,
        ShaperTemplate,
        HostTemplate,
        WardTemplate,
        SilenceTemplate,
        TideTemplate,
        SnoutTemplate,
        RibTemplate,
        EyeTemplate,
        SpireTemplate,
        FlowTemplate,
        BoltTemplate,
        AnglerSherd,
        ArcherSherd,
        ArmsUpSherd,
        BladeSherd,
        BrewerSherd,
        BurnSherd,
        DangerSherd,
        ExplorerSherd,
        FlowSherd,
        FriendSherd,
        GusterSherd,
        HeartSherd,
        HeartbreakSherd,
        HowlSherd,
        MinerSherd,
        MournerSherd,
        PlentySherd,
        PrizeSherd,
        ScrapeSherd,
        SheafSherd,
        ShelterSherd,
        SkullSherd,
        SnortSherd,
        Compass,
        EmptyMap,
        FilledMap,
        FlowerBannerPattern,
        SkullBannerPattern,
        GlobeBannerPattern,
        PiglinBannerPattern,
        FlowBannerPattern,
        GusterBannerPattern,
        FishingRod,
        CarrotOnAStick,
        WarpedFungusOnAStick,
        TropicalFish,
        NameTag,
        Lead,
        Spyglass,
        Clock,
        RecoveryCompass,
        EchoShard,
        GoatHorn,
        Bundle,
        Crossbow,
        SpectralArrow,
        TippedArrow,
        ChainmailHelmet,
        ChainmailChestplate,
        ChainmailLeggings,
        ChainmailBoots,
        GoldenApple,
        EnchantedGoldenApple,
        GoldenCarrot,
        GlisteringMelon,
        MushroomStew,
        RabbitStew,
        SuspiciousStew,
        PumpkinPie,
        RottenFlesh,
        SpiderEye,
        FermentedSpiderEye,
        PoisonousPotato,
        RabbitHide,
        RabbitFoot,
        PhantomMembrane,
        PrismarineShard,
        PrismarineCrystals,
        DiscFragment,
        DragonsBreath,
        Charcoal,
        ExperienceBottle,
        CodBucket,
        SalmonBucket,
        PufferfishBucket,
        TropicalFishBucket,
        AxolotlBucket,
        TadpoleBucket,
        EnchantedBook,
        WritableBook,
        WrittenBook,
        KnowledgeBook,
        SpruceBoat,
        BirchBoat,
        JungleBoat,
        AcaciaBoat,
        DarkOakBoat,
        CherryBoat,
        MangroveBoat,
        BambooRaft,
        OakChestBoat,
        SpruceChestBoat,
        BirchChestBoat,
        JungleChestBoat,
        AcaciaChestBoat,
        DarkOakChestBoat,
        CherryChestBoat,
        MangroveChestBoat,
        BambooChestRaft,
        ChestMinecart,
        FurnaceMinecart,
        HopperMinecart,
        TntMinecart,
        CommandBlockMinecart,
        SpawnEgg,
        FireworkRocket,
        FireworkStar,
        FireCharge,
        MusicDiscFirst,
        MusicDiscLast = MusicDiscFirst + 18,
        SplashPotion,
        LingeringPotion,
        ArmadilloScute,
        WolfArmor,
        GlowInkSac,
        Painting,
        Count
    };
}

struct ItemInfo
{
    const char* name = "";
    int tile = Tiles::Blank;
    int maxStack = 64;
};

inline bool isItem(StackId id) { return id >= Items::FIRST && id < Items::Count; }

constexpr int BOAT_WOODS = 9;
inline StackId boatOfWood(int wood, bool chest)
{
    if (wood < 0 || wood >= BOAT_WOODS) return 0;
    if (chest) return static_cast<StackId>(Items::OakChestBoat + wood);
    return wood == 0 ? static_cast<StackId>(Items::Boat) : static_cast<StackId>(Items::SpruceBoat + wood - 1);
}
inline int boatItemWood(StackId id)
{
    if (id == Items::Boat) return 0;
    if (id >= Items::SpruceBoat && id <= Items::BambooRaft) return static_cast<int>(id - Items::SpruceBoat) + 1;
    if (id >= Items::OakChestBoat && id <= Items::BambooChestRaft) return static_cast<int>(id - Items::OakChestBoat);
    return -1;
}
inline bool isChestBoatItem(StackId id) { return id >= Items::OakChestBoat && id <= Items::BambooChestRaft; }
inline bool isMinecartItem(StackId id)
{
    return id == Items::Minecart || (id >= Items::ChestMinecart && id <= Items::CommandBlockMinecart);
}
inline int minecartItemKind(StackId id)
{
    return id == Items::Minecart ? 0 : (id >= Items::ChestMinecart && id <= Items::CommandBlockMinecart)
                                           ? static_cast<int>(id - Items::ChestMinecart) + 1 : -1;
}
inline StackId signItemOfWood(int wood)
{
    static const StackId SIGNS[] = { Items::Sign, Items::SpruceSign, Items::BirchSign, Items::JungleSign,
                                     Items::AcaciaSign, Items::DarkOakSign, Items::CherrySign,
                                     Items::MangroveSign, Items::BambooSign, Items::CrimsonSign,
                                     Items::WarpedSign };
    return SIGNS[wood < 0 || wood >= static_cast<int>(sizeof(SIGNS) / sizeof(SIGNS[0])) ? 0 : wood];
}
inline int woodOfSignItem(StackId id)
{
    for (int wood = 0; wood < WOOD_COUNT; ++wood)
        if (signItemOfWood(wood) == id) return wood;
    return -1;
}
// Everything below the items is a block, Air included.
inline bool isBlockStack(StackId id) { return id < Items::FIRST; }
// Only call this once isBlockStack has said yes.
inline BlockId asBlock(StackId id) { return static_cast<BlockId>(id); }

const ItemInfo& itemInfo(StackId id);

// What a block turns into when you mine it. Air means it drops
// nothing; anything else is what lands on the floor.
//
// Declared here rather than beside the rest of what a block is,
// because a drop can be an item and Block.h cannot see StackId --
// Items.h is the one that includes Block.h, not the other way round.
StackId blockDrop(BlockId id);

// These three work on blocks and items alike, which is what lets the
// inventory, the hotbar and the dropped-item renderer stay unaware of
// which they are holding.
const char* displayName(StackId id);
int atlasTileFor(StackId id);
int flatIconFor(BlockId id);
int maxStackOf(StackId id);

inline bool isHorseArmour(StackId id)
{
    return id >= Items::LeatherHorseArmor && id <= Items::DiamondHorseArmor;
}
inline int horseArmourPoints(StackId id)
{
    switch (id)
    {
        case Items::LeatherHorseArmor: return 3;
        case Items::IronHorseArmor: return 5;
        case Items::GoldHorseArmor: return 7;
        case Items::DiamondHorseArmor: return 11;
        default: return 0;
    }
}

// A dye, and which of the sixteen colours it is. The dyes are
// contiguous and in the same order as the wools, so this is
// arithmetic rather than a table.
inline bool isDye(StackId id)
{
    return id >= Items::DyeWhite && id <= Items::DyeBlack;
}
inline int dyeColour(StackId id) { return isDye(id) ? id - Items::DyeWhite : 0; }
inline StackId dyeOfColour(int colour)
{
    if (colour < 0) colour = 0;
    if (colour > 15) colour = 15;
    return static_cast<StackId>(Items::DyeWhite + colour);
}

// Whether an animal will follow and breed for it.
bool isBreedingFood(StackId id);

int itemDurability(StackId id);

namespace Crossbows
{
    enum Load { Empty = 0, Arrow = 1, Firework = 2, Spectral = 3, Tipped = 4 };
    constexpr float CHARGE_SECONDS = 1.25f;
    constexpr float ARROW_SPEED = 52.0f;
    inline int loadOf(uint32_t enchantments) { return static_cast<int>((enchantments >> 24) & 7u); }
    inline int effectOf(uint32_t enchantments) { return static_cast<int>((enchantments >> 27) & 31u); }
    inline uint32_t withLoad(uint32_t enchantments, int load, int effect = 0)
    {
        return (enchantments & 0x00FFFFFFu) | (static_cast<uint32_t>(load & 7) << 24) | (static_cast<uint32_t>(effect & 31) << 27);
    }
}
