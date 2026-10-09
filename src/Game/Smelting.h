#pragma once
#include "Items.h"

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

    // What `input` becomes, or Air if a furnace makes nothing of it.
    StackId resultOf(StackId input);

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
