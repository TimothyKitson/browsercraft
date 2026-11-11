#pragma once
#include <array>
#include <string>
#include <vector>

namespace Attributes
{
    enum class Kind : int
    {
        Armor,
        ArmorToughness,
        AttackDamage,
        AttackKnockback,
        AttackSpeed,
        BlockBreakSpeed,
        BlockInteractionRange,
        BurningTime,
        ExplosionKnockbackResistance,
        EntityInteractionRange,
        FallDamageMultiplier,
        FlyingSpeed,
        FollowRange,
        Gravity,
        JumpStrength,
        KnockbackResistance,
        Luck,
        MaxAbsorption,
        MaxHealth,
        MiningEfficiency,
        MovementEfficiency,
        MovementSpeed,
        OxygenBonus,
        SafeFallDistance,
        Scale,
        SneakingSpeed,
        SpawnReinforcements,
        StepHeight,
        SubmergedMiningSpeed,
        SweepingDamageRatio,
        WaterMovementEfficiency,
        Count
    };
    constexpr int COUNT = static_cast<int>(Kind::Count);

    struct Info
    {
        const char* id;
        const char* name;
        double base;
        double min;
        double max;
    };
    const Info& info(Kind kind);
    bool fromId(const std::string& text, Kind& out);

    enum class Operation { AddValue, AddMultipliedBase, AddMultipliedTotal };
    bool operationFromId(const std::string& text, Operation& out);

    struct Modifier
    {
        std::string id;
        double amount = 0.0;
        Operation operation = Operation::AddValue;
    };

    double compute(double base, const std::vector<Modifier>& modifiers, const Info& info);
    class Set;
    bool command(Set& set, const std::vector<std::string>& args, const std::string& who, std::string& output);

    class Set
    {
    public:
        double value(Kind kind, double extraBase = 0.0) const;
        double base(Kind kind) const;
        double ratio(Kind kind) const { return value(kind) / info(kind).base; }
        void setBase(Kind kind, double value);
        void resetBase(Kind kind);
        bool addModifier(Kind kind, const Modifier& modifier);
        bool removeModifier(Kind kind, const std::string& id);
        const Modifier* modifier(Kind kind, const std::string& id) const;
        bool customised() const;
        std::string save() const;
        void load(const std::string& text);

    private:
        struct Instance
        {
            bool baseSet = false;
            double base = 0.0;
            std::vector<Modifier> modifiers;
        };
        std::array<Instance, COUNT> m_instances{};
    };
}
