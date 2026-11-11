#include "Attributes.h"
#include <algorithm>
#include <sstream>

namespace
{
    const Attributes::Info INFO[Attributes::COUNT] = {
        { "generic.armor", "Armor", 0.0, 0.0, 30.0 },
        { "generic.armor_toughness", "Armor Toughness", 0.0, 0.0, 20.0 },
        { "generic.attack_damage", "Attack Damage", 1.0, 0.0, 2048.0 },
        { "generic.attack_knockback", "Attack Knockback", 0.0, 0.0, 5.0 },
        { "generic.attack_speed", "Attack Speed", 4.0, 0.0, 1024.0 },
        { "player.block_break_speed", "Block Break Speed", 1.0, 0.0, 1024.0 },
        { "player.block_interaction_range", "Block Interaction Range", 4.5, 0.0, 64.0 },
        { "generic.burning_time", "Burning Time", 1.0, 0.0, 1024.0 },
        { "generic.explosion_knockback_resistance", "Explosion Knockback Resistance", 0.0, 0.0, 1.0 },
        { "player.entity_interaction_range", "Entity Interaction Range", 3.0, 0.0, 64.0 },
        { "generic.fall_damage_multiplier", "Fall Damage Multiplier", 1.0, 0.0, 100.0 },
        { "generic.flying_speed", "Flying Speed", 0.4, 0.0, 1024.0 },
        { "generic.follow_range", "Follow Range", 32.0, 0.0, 2048.0 },
        { "generic.gravity", "Gravity", 0.08, -1.0, 1.0 },
        { "generic.jump_strength", "Jump Strength", 0.42, 0.0, 32.0 },
        { "generic.knockback_resistance", "Knockback Resistance", 0.0, 0.0, 1.0 },
        { "generic.luck", "Luck", 0.0, -1024.0, 1024.0 },
        { "generic.max_absorption", "Max Absorption", 0.0, 0.0, 2048.0 },
        { "generic.max_health", "Max Health", 20.0, 1.0, 1024.0 },
        { "player.mining_efficiency", "Mining Efficiency", 0.0, 0.0, 1024.0 },
        { "generic.movement_efficiency", "Movement Efficiency", 0.0, 0.0, 1.0 },
        { "generic.movement_speed", "Speed", 0.1, 0.0, 1024.0 },
        { "generic.oxygen_bonus", "Oxygen Bonus", 0.0, 0.0, 1024.0 },
        { "generic.safe_fall_distance", "Safe Fall Distance", 3.0, -1024.0, 1024.0 },
        { "generic.scale", "Scale", 1.0, 0.0625, 16.0 },
        { "player.sneaking_speed", "Sneaking Speed", 0.3, 0.0, 1.0 },
        { "zombie.spawn_reinforcements", "Zombie Reinforcements", 0.0, 0.0, 1.0 },
        { "generic.step_height", "Step Height", 0.6, 0.0, 10.0 },
        { "player.submerged_mining_speed", "Submerged Mining Speed", 0.2, 0.0, 20.0 },
        { "player.sweeping_damage_ratio", "Sweeping Damage Ratio", 0.0, 0.0, 1.0 },
        { "generic.water_movement_efficiency", "Water Movement Efficiency", 0.0, 0.0, 1.0 },
    };
}

const Attributes::Info& Attributes::info(Kind kind)
{
    const int k = std::clamp(static_cast<int>(kind), 0, COUNT - 1);
    return INFO[k];
}

bool Attributes::fromId(const std::string& text, Kind& out)
{
    std::string wanted = text;
    if (wanted.rfind("minecraft:", 0) == 0) wanted = wanted.substr(10);
    for (int k = 0; k < COUNT; ++k)
    {
        const std::string id = INFO[k].id;
        const std::string bare = id.substr(id.find('.') + 1);
        if (wanted == id || wanted == bare)
        {
            out = static_cast<Kind>(k);
            return true;
        }
    }
    return false;
}

bool Attributes::operationFromId(const std::string& text, Operation& out)
{
    if (text == "add_value" || text == "addition") { out = Operation::AddValue; return true; }
    if (text == "add_multiplied_base" || text == "multiply_base") { out = Operation::AddMultipliedBase; return true; }
    if (text == "add_multiplied_total" || text == "multiply") { out = Operation::AddMultipliedTotal; return true; }
    return false;
}

double Attributes::compute(double base, const std::vector<Modifier>& modifiers, const Info& info)
{
    double value = base;
    for (const Modifier& m : modifiers)
        if (m.operation == Operation::AddValue) value += m.amount;
    double scaled = value;
    for (const Modifier& m : modifiers)
        if (m.operation == Operation::AddMultipliedBase) scaled += value * m.amount;
    for (const Modifier& m : modifiers)
        if (m.operation == Operation::AddMultipliedTotal) scaled *= 1.0 + m.amount;
    return std::clamp(scaled, info.min, info.max);
}

double Attributes::Set::base(Kind kind) const
{
    const Instance& one = m_instances[static_cast<size_t>(kind)];
    return one.baseSet ? one.base : info(kind).base;
}

double Attributes::Set::value(Kind kind, double extraBase) const
{
    return compute(base(kind) + extraBase, m_instances[static_cast<size_t>(kind)].modifiers, info(kind));
}

void Attributes::Set::setBase(Kind kind, double value)
{
    Instance& one = m_instances[static_cast<size_t>(kind)];
    one.baseSet = true;
    one.base = std::clamp(value, info(kind).min, info(kind).max);
}

void Attributes::Set::resetBase(Kind kind)
{
    m_instances[static_cast<size_t>(kind)].baseSet = false;
}

bool Attributes::Set::addModifier(Kind kind, const Modifier& modifier)
{
    Instance& one = m_instances[static_cast<size_t>(kind)];
    for (const Modifier& existing : one.modifiers)
        if (existing.id == modifier.id) return false;
    one.modifiers.push_back(modifier);
    return true;
}

bool Attributes::Set::removeModifier(Kind kind, const std::string& id)
{
    std::vector<Modifier>& list = m_instances[static_cast<size_t>(kind)].modifiers;
    const auto before = list.size();
    list.erase(std::remove_if(list.begin(), list.end(), [&](const Modifier& m) { return m.id == id; }), list.end());
    return list.size() != before;
}

const Attributes::Modifier* Attributes::Set::modifier(Kind kind, const std::string& id) const
{
    for (const Modifier& m : m_instances[static_cast<size_t>(kind)].modifiers)
        if (m.id == id) return &m;
    return nullptr;
}

bool Attributes::Set::customised() const
{
    for (const Instance& one : m_instances)
        if (one.baseSet || !one.modifiers.empty()) return true;
    return false;
}

std::string Attributes::Set::save() const
{
    std::ostringstream out;
    for (int k = 0; k < COUNT; ++k)
    {
        const Instance& one = m_instances[static_cast<size_t>(k)];
        if (one.baseSet) out << "base\t" << INFO[k].id << '\t' << one.base << '\n';
        for (const Modifier& m : one.modifiers)
            out << "modifier\t" << INFO[k].id << '\t' << m.id << '\t' << m.amount << '\t' << static_cast<int>(m.operation) << '\n';
    }
    return out.str();
}

void Attributes::Set::load(const std::string& text)
{
    m_instances = {};
    std::istringstream in(text);
    std::string row;
    while (std::getline(in, row))
    {
        std::vector<std::string> cells;
        std::string cell;
        std::istringstream line(row);
        while (std::getline(line, cell, '\t')) cells.push_back(cell);
        Kind kind;
        if (cells.size() < 3 || !fromId(cells[1], kind)) continue;
        try
        {
            if (cells[0] == "base") setBase(kind, std::stod(cells[2]));
            else if (cells[0] == "modifier" && cells.size() >= 5)
                addModifier(kind, Modifier{ cells[2], std::stod(cells[3]), static_cast<Operation>(std::clamp(std::stoi(cells[4]), 0, 2)) });
        }
        catch (...) {}
    }
}

bool Attributes::command(Set& set, const std::vector<std::string>& args, const std::string& who, std::string& output)
{
    const std::string usage = "/attribute <target> <attribute> get|base|modifier ...";
    if (args.size() < 3)
    {
        output = usage;
        return false;
    }
    Kind kind;
    if (!fromId(args[1], kind))
    {
        output = "Unknown attribute '" + args[1] + "'";
        return false;
    }
    const Info& about = info(kind);
    auto number = [](const std::string& text, double& out) {
        try
        {
            size_t used = 0;
            out = std::stod(text, &used);
            return used == text.size();
        }
        catch (...) { return false; }
    };
    auto shown = [](double value) {
        std::ostringstream text;
        text << value;
        return text.str();
    };
    const std::string& action = args[2];
    if (action == "get")
    {
        double scale = 1.0;
        if (args.size() >= 4 && !number(args[3], scale)) scale = 1.0;
        output = "Value of attribute " + std::string(about.name) + " for " + who + " is " + shown(set.value(kind) * scale);
        return true;
    }
    if (action == "base" && args.size() >= 4)
    {
        if (args[3] == "get")
        {
            double scale = 1.0;
            if (args.size() >= 5 && !number(args[4], scale)) scale = 1.0;
            output = "Base value of attribute " + std::string(about.name) + " for " + who + " is " + shown(set.base(kind) * scale);
            return true;
        }
        if (args[3] == "set" && args.size() >= 5)
        {
            double value = 0.0;
            if (!number(args[4], value))
            {
                output = "Expected a number";
                return false;
            }
            set.setBase(kind, value);
            output = "Base value for attribute " + std::string(about.name) + " for " + who + " set to " + shown(set.base(kind));
            return true;
        }
        if (args[3] == "reset")
        {
            set.resetBase(kind);
            output = "Base value for attribute " + std::string(about.name) + " for " + who + " reset to default " + shown(about.base);
            return true;
        }
    }
    if (action == "modifier" && args.size() >= 5)
    {
        if (args[3] == "add" && args.size() >= 7)
        {
            Modifier added;
            added.id = args[4];
            Operation operation;
            if (!number(args[5], added.amount) || !operationFromId(args[6], operation))
            {
                output = "/attribute <target> <attribute> modifier add <id> <amount> add_value|add_multiplied_base|add_multiplied_total";
                return false;
            }
            added.operation = operation;
            if (!set.addModifier(kind, added))
            {
                output = "Entity " + who + " already has modifier " + added.id + " on attribute " + about.name;
                return false;
            }
            output = "Added modifier " + added.id + " to attribute " + about.name + " for " + who;
            return true;
        }
        if (args[3] == "remove")
        {
            if (!set.removeModifier(kind, args[4]))
            {
                output = "There is no modifier " + args[4] + " on attribute " + about.name + " for " + who;
                return false;
            }
            output = "Removed modifier " + args[4] + " from attribute " + about.name + " for " + who;
            return true;
        }
        if (args[3] == "value" && args.size() >= 6 && args[4] == "get")
        {
            const Modifier* found = set.modifier(kind, args[5]);
            if (!found)
            {
                output = "There is no modifier " + args[5] + " on attribute " + about.name + " for " + who;
                return false;
            }
            output = "Value of modifier " + found->id + " on attribute " + about.name + " for " + who + " is " + shown(found->amount);
            return true;
        }
    }
    output = usage;
    return false;
}
