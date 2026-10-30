#pragma once
#include "Items.h"
#include "World/Block.h"
#include <cstdint>

struct ItemStack;

// The furnace: what turns into what, what burns and for how long, and
// the little state machine that runs one.
//
// Furnace::tick is a pure step over the furnace's own three slots, so
// --selftest can smelt a stack of ore and watch the fuel run out without
// a world to put the furnace in. Application owns the furnaces and calls
// tick; nothing in here knows where it is.
namespace Smelting
{
    // Seconds one item takes, the same for everything, as in Minecraft.
    constexpr float COOK_SECONDS = 10.0f;

    // Which of the three machines this is. They are one block as far
    // as the state machine is concerned; the kind decides only what
    // goes in and how long it takes.
    enum class Kind : uint8_t { Furnace, Smoker, BlastFurnace };

    // The two specialists run at twice the speed and take half as
    // much, which is Minecraft's trade and the only reason to build
    // one when you already have a furnace.
    constexpr float FAST_COOK_SECONDS = COOK_SECONDS / 2.0f;
    inline float cookSeconds(Kind kind)
    {
        return (kind == Kind::Furnace) ? COOK_SECONDS : FAST_COOK_SECONDS;
    }

    // What `input` becomes, or Air if a furnace makes nothing of it.
    StackId resultOf(StackId input);

    // Whether this machine will take it at all. A furnace takes
    // everything; a smoker takes food and a blast furnace takes metal.
    bool suits(Kind kind, StackId input);

    // What this machine makes of it, which is resultOf once it has
    // agreed to take it.
    inline StackId resultIn(Kind kind, StackId input)
    {
        return suits(kind, input) ? resultOf(input) : Blocks::Air;
    }

    // Which kind a block is. Anything that is not one of the three
    // reads as a plain furnace, which is the harmless answer.
    Kind kindOf(BlockId block);

    // How long one of `fuel` keeps the fire in, or 0 if it does not burn.
    float burnSeconds(StackId fuel);

    inline bool isFuel(StackId id) { return burnSeconds(id) > 0.0f; }
    inline bool isSmeltable(StackId id) { return resultOf(id) != Blocks::Air; }
}

// One furnace's contents and how far along it is. Three slots, the same
// three Minecraft has.
struct Furnace
{
    ItemStack* input = nullptr;
    ItemStack* fuel = nullptr;
    ItemStack* output = nullptr;

    // Which machine this is. A furnace by default, so everything that
    // made one before these existed still makes a furnace.
    Smelting::Kind kind = Smelting::Kind::Furnace;

    float burnLeft = 0.0f;    // seconds of fire remaining
    float burnTotal = 0.0f;   // what the current fuel gave, for the flame gauge
    float cooked = 0.0f;      // seconds into the item being smelted

    bool lit() const { return burnLeft > 0.0f; }

    // 0..1 along the arrow, and 0..1 down the flame.
    float cookFraction() const;
    float burnFraction() const;

    // One step. Lights the fire when there is something to smelt and
    // fuel to smelt it with, moves the finished item into the output,
    // and lets the fire burn down when there is nothing left to do.
    void tick(float deltaTime);
};
