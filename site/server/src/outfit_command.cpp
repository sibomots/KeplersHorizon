//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "outfit_command.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "db.h"
#include "facilities.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

// Equipment definitions
struct Equipment
{
    const char* name;
    const char* description;
    int price;           // Cost in Credits
    const char* column;  // Database column to update
};

static const Equipment EQUIPMENT[] = {
    {"lrs", "Long Range Scanner", 50, "lrs"},
    {"tb", "Transporter Beam", 75, "tb"},
    {"drones", "Mining/Salvage Drones", 100, "dr"},
    {"dr", "Mining/Salvage Drones", 100, "dr"},  // Alias
};

static const int NUM_EQUIPMENT = sizeof(EQUIPMENT) / sizeof(EQUIPMENT[0]);

bool OutfitCommand::invoke(void)
{
    if (m_ship_code.empty() || m_ship_code == "list")
    {
        show_equipment();
        return true;
    }

    return do_outfit();
}

void OutfitCommand::show_equipment()
{
    std::ostringstream out;
    out << "         EQUIPMENT OUTFITTING\n"
        << "-------------------------------------------\n"
        << "Requires ship at controlled SHIPYARD.\n\n";

    out << "Item     Description             Cost\n";
    out << "-------  --------------------  ------\n";

    // Only show unique items (skip dr alias)
    out << "LRS      Long Range Scanner      50 CR\n";
    out << "TB       Transporter Beam        75 CR\n";
    out << "DRONES   Mining/Salvage Drones  100 CR\n";

    out << "-------------------------------------------\n"
        << "Use: outfit <ship> <equipment>\n"
        << "Example: outfit W1 lrs";

    Telemetry::getInstance().write(out.str());
}

bool OutfitCommand::do_outfit()
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char me = StateMachine::getInstance().get_current_player();

    DatabaseManager& db = DatabaseManager::getInstance();

    // Normalize equipment name to lowercase
    std::string equip_lower = m_equipment;
    for (auto& c : equip_lower)
        c = tolower(c);

    // Find equipment
    const Equipment* item = nullptr;
    for (int i = 0; i < NUM_EQUIPMENT; i++)
    {
        if (equip_lower == EQUIPMENT[i].name)
        {
            item = &EQUIPMENT[i];
            break;
        }
    }

    if (!item)
    {
        Telemetry::getInstance().write("OUTFIT: Unknown equipment: " +
                                       m_equipment + ". Use 'outfit list'.");
        return false;
    }

    // Normalize ship code to uppercase
    std::string ship_upper = m_ship_code;
    for (auto& c : ship_upper)
        c = toupper(c);

    // Find the ship
    auto ships = db.query(
        "SELECT at_hex, at_system FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, me) +
        "' AND ship_code='" + db.esc(ship_upper) +
        "' AND destroyed_at IS NULL");

    if (ships.empty())
    {
        Telemetry::getInstance().write("OUTFIT: Ship " + ship_upper +
                                       " not found.");
        return false;
    }

    std::string at_hex = ships[0][0];
    std::string at_system = ships[0][1];

    if (at_hex.empty())
    {
        Telemetry::getInstance().write(
            "OUTFIT: Ship " + ship_upper + " is not deployed.");
        return false;
    }

    // Check if ship is at a system with a controlled SHIPYARD
    if (at_system.empty())
    {
        // Get system name from hex
        auto sys_rows = db.query(
            "SELECT name FROM star_systems WHERE hex_id='" + db.esc(at_hex) +
            "' AND module_id=1");
        if (!sys_rows.empty())
        {
            at_system = sys_rows[0][0];
        }
    }

    if (at_system.empty())
    {
        Telemetry::getInstance().write(
            "OUTFIT: Ship " + ship_upper + " is not at a star system.");
        return false;
    }

    // Check for SHIPYARD controlled by player
    if (!FacilityEngine::player_controls(game_id, at_system, "SHIPYARD", me))
    {
        Telemetry::getInstance().write(
            "OUTFIT: " + at_system + " does not have a SHIPYARD you control.");
        return false;
    }

    // Check credits
    int credits = (me == 'A') ? s.creditsA : s.creditsB;
    if (credits < item->price)
    {
        Telemetry::getInstance().write(
            "OUTFIT: Insufficient credits. Need " + std::to_string(item->price) +
            " CR, have " + std::to_string(credits) + " CR.");
        return false;
    }

    // Deduct credits
    if (me == 'A')
        s.creditsA -= item->price;
    else
        s.creditsB -= item->price;

    StateMachine::getInstance().save_game(s);

    // Add equipment to ship
    db.exec("UPDATE ships SET " + std::string(item->column) + "=" +
            item->column + "+1 WHERE game_id=" + std::to_string(game_id) +
            " AND owner='" + std::string(1, me) + "' AND ship_code='" +
            db.esc(ship_upper) + "'");

    std::ostringstream msg;
    msg << "OUTFIT: Installed " << item->description << " on " << ship_upper
        << " for " << item->price << " CR.";
    Telemetry::getInstance().write(msg.str());

    Logger::instance().info(msg.str());

    return true;
}
