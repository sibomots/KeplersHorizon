///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "outfit_strategy.h"

#include <cstdlib>
#include <format>
#include <sstream>

#include "db.h"
#include "facilities.h"
#include "moduleutil.h"
#include "outfit_modes.h"
#include "statemachine.h"
#include "telemetry.h"

// Equipment data is DB-driven via equipment_catalog table (seeded in Game.sql).

bool OutfitStrategy::show_equipment(void)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    int game_id = StateMachine::instance().get_game_id();
    int module_id = get_module_id_for_game(game_id);

    auto rows = db.Query("SELECT equipment_type, description, price "
                         "FROM equipment_catalog WHERE module_id=? "
                         "ORDER BY equipment_type",
                         {module_id});

    std::ostringstream out;
    out << "         EQUIPMENT OUTFITTING\n"
        << "-------------------------------------------\n"
        << "Requires ship at controlled SHIPYARD.\n\n"
        << "Item     Description             Cost\n"
        << "-------  --------------------  ------\n";

    for (const auto& row : rows)
    {
        out << std::format("{:<8} {:<20}  {} CR\n", row[0], row[1], row[2]);
    }

    out << "-------------------------------------------\n"
        << "Use: outfit <ship> <equipment>\n"
        << "Example: outfit W1 lrs";

    Telemetry::instance().write(out.str());
    bres = true;

    return bres;
}

bool OutfitStrategy::outfit_lrs(int game_id, char owner,
                                const std::string& ship_code)
{
    return install_equipment(game_id, owner, ship_code, "LRS");
}

bool OutfitStrategy::outfit_drones(int game_id, char owner,
                                   const std::string& ship_code)
{
    return install_equipment(game_id, owner, ship_code, "DRONES");
}

bool OutfitStrategy::install_equipment(int game_id, char owner,
                                       const std::string& ship_code,
                                       const std::string& equipment_type)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int module_id = get_module_id_for_game(game_id);

    auto equip = db.Query("SELECT description, price, ship_column "
                          "FROM equipment_catalog WHERE module_id=? "
                          "AND equipment_type=?",
                          {module_id, equipment_type});

    if (equip.empty())
    {
        Telemetry::instance().write(std::format(
            "OUTFIT: Equipment {} not in catalog.", equipment_type));
    }
    else
    {
        std::string desc = equip[0][0];
        int price = std::atoi(equip[0][1].c_str());
        std::string column = equip[0][2];

        // Normalize ship code to uppercase
        std::string ship_upper = ship_code;
        for (unsigned int idx = 0; idx < ship_upper.size(); idx++)
        {
            ship_upper[idx] = toupper(ship_upper[idx]);
        }

        auto ships =
            db.Query("SELECT at_hex, at_system FROM ships WHERE game_id=? "
                     "AND owner=? AND ship_code=? AND destroyed_at IS NULL",
                     {game_id, owner, ship_upper});

        if (ships.empty())
        {
            Telemetry::instance().write(
                std::format("OUTFIT: Ship {} not found.", ship_upper));
        }
        else
        {
            std::string at_hex = ships[0][0];
            std::string at_system = ships[0][1];

            if (at_hex.empty())
            {
                Telemetry::instance().write(std::format(
                    "OUTFIT: Ship {} is not deployed.", ship_upper));
            }
            else
            {
                if (at_system.empty())
                {
                    auto sys_rows = db.Query("SELECT name FROM star_systems "
                                             "WHERE hex_id=? AND module_id=1",
                                             {at_hex});
                    if (!sys_rows.empty())
                    {
                        at_system = sys_rows[0][0];
                    }
                }

                if (at_system.empty())
                {
                    Telemetry::instance().write(
                        std::format("OUTFIT: Ship {} is not at a star system.",
                                    ship_upper));
                }
                else if (!FacilityEngine::player_controls(game_id, at_system,
                                                          "SHIPYARD", owner))
                {
                    Telemetry::instance().write(std::format(
                        "OUTFIT: {} has no SHIPYARD you control.", at_system));
                }
                else
                {
                    int credits = (KH_EQU(owner, 'A')) ? s.creditsA : s.creditsB;

                    if (credits < price)
                    {
                        Telemetry::instance().write(
                            std::format("OUTFIT: Insufficient credits. "
                                        "Need {} CR, have {} CR.",
                                        price, credits));
                    }
                    else
                    {
                        if (KH_EQU(owner,'A'))
                        {
                            s.creditsA -= price;
                        }
                        else
                        {
                            s.creditsB -= price;
                        }
                        StateMachine::instance().save_game(s);

                        db.Exec(std::format("UPDATE ships SET {}={}+1 "
                                            "WHERE game_id=? AND owner=? "
                                            "AND ship_code=?",
                                            column, column),
                                {game_id, owner, ship_upper});

                        std::string msg =
                            std::format("OUTFIT: Installed {} on {} for {} CR.",
                                        desc, ship_upper, price);
                        Telemetry::instance().write(msg);
                        bres = true;
                    }
                }
            }
        }
    }

    return bres;
}
