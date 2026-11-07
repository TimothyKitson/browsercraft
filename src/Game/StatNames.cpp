#include "Stats.h"

namespace
{
    struct Info
    {
        const char* id;
        const char* name;
        Stats::Format format;
    };

    const Info INFO[Stats::CUSTOM_COUNT] = {
        { "leave_game", "Games Quit", Stats::Format::Plain },
        { "play_time", "Time Played", Stats::Format::Time },
        { "total_world_time", "Time with World Open", Stats::Format::Time },
        { "time_since_death", "Time Since Last Death", Stats::Format::Time },
        { "time_since_rest", "Time Since Last Rest", Stats::Format::Time },
        { "sneak_time", "Sneak Time", Stats::Format::Time },
        { "walk_one_cm", "Distance Walked", Stats::Format::Distance },
        { "crouch_one_cm", "Distance Crouched", Stats::Format::Distance },
        { "sprint_one_cm", "Distance Sprinted", Stats::Format::Distance },
        { "walk_on_water_one_cm", "Distance Walked on Water", Stats::Format::Distance },
        { "fall_one_cm", "Distance Fallen", Stats::Format::Distance },
        { "climb_one_cm", "Distance Climbed", Stats::Format::Distance },
        { "fly_one_cm", "Distance Flown", Stats::Format::Distance },
        { "walk_under_water_one_cm", "Distance Walked under Water", Stats::Format::Distance },
        { "minecart_one_cm", "Distance by Minecart", Stats::Format::Distance },
        { "boat_one_cm", "Distance by Boat", Stats::Format::Distance },
        { "pig_one_cm", "Distance by Pig", Stats::Format::Distance },
        { "horse_one_cm", "Distance by Horse", Stats::Format::Distance },
        { "aviate_one_cm", "Distance by Elytra", Stats::Format::Distance },
        { "swim_one_cm", "Distance Swum", Stats::Format::Distance },
        { "strider_one_cm", "Distance by Strider", Stats::Format::Distance },
        { "jump", "Jumps", Stats::Format::Plain },
        { "drop", "Items Dropped", Stats::Format::Plain },
        { "damage_dealt", "Damage Dealt", Stats::Format::Tenths },
        { "damage_dealt_absorbed", "Damage Dealt (Absorbed)", Stats::Format::Tenths },
        { "damage_dealt_resisted", "Damage Dealt (Resisted)", Stats::Format::Tenths },
        { "damage_taken", "Damage Taken", Stats::Format::Tenths },
        { "damage_blocked_by_shield", "Damage Blocked by Shield", Stats::Format::Tenths },
        { "damage_absorbed", "Damage Absorbed", Stats::Format::Tenths },
        { "damage_resisted", "Damage Resisted", Stats::Format::Tenths },
        { "deaths", "Number of Deaths", Stats::Format::Plain },
        { "mob_kills", "Mob Kills", Stats::Format::Plain },
        { "animals_bred", "Animals Bred", Stats::Format::Plain },
        { "player_kills", "Player Kills", Stats::Format::Plain },
        { "fish_caught", "Fish Caught", Stats::Format::Plain },
        { "talked_to_villager", "Talked to Villagers", Stats::Format::Plain },
        { "traded_with_villager", "Traded with Villagers", Stats::Format::Plain },
        { "eat_cake_slice", "Cake Slices Eaten", Stats::Format::Plain },
        { "fill_cauldron", "Cauldrons Filled", Stats::Format::Plain },
        { "use_cauldron", "Water Taken from Cauldron", Stats::Format::Plain },
        { "clean_armor", "Armor Pieces Cleaned", Stats::Format::Plain },
        { "clean_banner", "Banners Cleaned", Stats::Format::Plain },
        { "clean_shulker_box", "Shulker Boxes Cleaned", Stats::Format::Plain },
        { "interact_with_brewingstand", "Interactions with Brewing Stand", Stats::Format::Plain },
        { "interact_with_beacon", "Interactions with Beacon", Stats::Format::Plain },
        { "inspect_dropper", "Droppers Searched", Stats::Format::Plain },
        { "inspect_hopper", "Hoppers Searched", Stats::Format::Plain },
        { "inspect_dispenser", "Dispensers Searched", Stats::Format::Plain },
        { "play_noteblock", "Note Blocks Played", Stats::Format::Plain },
        { "tune_noteblock", "Note Blocks Tuned", Stats::Format::Plain },
        { "pot_flower", "Plants Potted", Stats::Format::Plain },
        { "trigger_trapped_chest", "Trapped Chests Triggered", Stats::Format::Plain },
        { "open_enderchest", "Ender Chests Opened", Stats::Format::Plain },
        { "enchant_item", "Items Enchanted", Stats::Format::Plain },
        { "play_record", "Music Discs Played", Stats::Format::Plain },
        { "interact_with_furnace", "Interactions with Furnace", Stats::Format::Plain },
        { "interact_with_crafting_table", "Interactions with Crafting Table", Stats::Format::Plain },
        { "open_chest", "Chests Opened", Stats::Format::Plain },
        { "sleep_in_bed", "Times Slept in a Bed", Stats::Format::Plain },
        { "open_shulker_box", "Shulker Boxes Opened", Stats::Format::Plain },
        { "open_barrel", "Barrels Opened", Stats::Format::Plain },
        { "interact_with_blast_furnace", "Interactions with Blast Furnace", Stats::Format::Plain },
        { "interact_with_smoker", "Interactions with Smoker", Stats::Format::Plain },
        { "interact_with_lectern", "Interactions with Lectern", Stats::Format::Plain },
        { "interact_with_campfire", "Interactions with Campfire", Stats::Format::Plain },
        { "interact_with_cartography_table", "Interactions with Cartography Table", Stats::Format::Plain },
        { "interact_with_loom", "Interactions with Loom", Stats::Format::Plain },
        { "interact_with_stonecutter", "Interactions with Stonecutter", Stats::Format::Plain },
        { "bell_ring", "Bells Rung", Stats::Format::Plain },
        { "raid_trigger", "Raids Triggered", Stats::Format::Plain },
        { "raid_win", "Raids Won", Stats::Format::Plain },
        { "interact_with_anvil", "Interactions with Anvil", Stats::Format::Plain },
        { "interact_with_grindstone", "Interactions with Grindstone", Stats::Format::Plain },
        { "target_hit", "Targets Hit", Stats::Format::Plain },
        { "interact_with_smithing_table", "Interactions with Smithing Table", Stats::Format::Plain },
    };
}

const char* Stats::id(Custom stat) { return INFO[static_cast<int>(stat)].id; }
const char* Stats::name(Custom stat) { return INFO[static_cast<int>(stat)].name; }
Stats::Format Stats::format(Custom stat) { return INFO[static_cast<int>(stat)].format; }
