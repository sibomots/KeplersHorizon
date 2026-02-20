///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "represagent.h"

#include <format>
#include <sstream>

#include "db.h"
#include "facilities.h"
#include "localization.h"
#include "star_system_constraints.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool RepresAgent::apply(RepresAgentParam& param)
{
    return std::visit(
        [this](auto&& arg) -> bool
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, RepairParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, ResupplyParam>)
            {
                return this->apply(arg);
            }
            return false;
        },
        param);
}

bool RepresAgent::apply(RepairParam& param)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    RepairMode mode = param.get_mode();

    if (mode == RepairMode::REPAIR_LIST)
    {
        auto rows = db.Query(
            "SELECT s.ship_code, s.ship_name, s.at_hex, s.pd, s.phasic, "
            "s.shield, "
            "s.launcher, s.torpedoes, s.pd_max, s.phasic_max, s.shield_max, "
            "s.launcher_max, s.torpedoes_max FROM ships s "
            "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = "
            "s.game_id "
            "WHERE s.game_id=? AND s.owner=? AND bs.owner=? "
            "AND s.destroyed_at IS NULL AND ("
            "s.pd < s.pd_max OR s.phasic < s.phasic_max OR "
            "s.shield < s.shield_max OR s.launcher < s.launcher_max)",
            {s.game_id, owner, owner});

        if (rows.empty())
        {
            Telemetry::instance().write(LC_REPRES_NO_DAMAGED_SHIPS_AT_BASES);
        }
        else
        {
            std::ostringstream out;
            out << LC_REPRES_LIST_BANNER << ":\n";
            for (const auto& r : rows)
            {
                out << "  " << r[0] << " (" << r[1] << ") at " << r[2]
                    << "\n";
            }
            out << LC_REPRES_HINT << "\n" << LC_REPRES_EXAMPLE << "\n";
            Telemetry::instance().write(out.str());
        }
        bres = true;
    }
    else
    {
        std::string ship_code = param.get_ship_code();
        AttributeID attr = param.get_attribute();
        int amount = param.get_amount();

        auto shipRow = db.Query(
            "SELECT s.at_hex, s.pd, s.phasic, s.shield, s.launcher, "
            "s.torpedoes, "
            "s.pd_max, s.phasic_max, s.shield_max, s.launcher_max, "
            "s.torpedoes_max, "
            "ss.name "
            "FROM ships s "
            "LEFT JOIN star_systems ss ON ss.hex_id = s.at_hex "
            "AND ss.module_id = 1 "
            "WHERE s.game_id=? AND s.owner=? AND s.ship_code=? "
            "AND s.destroyed_at IS NULL",
            {s.game_id, owner, ship_code});

        if (shipRow.empty())
        {
            Telemetry::instance().write(
                std::format(LC_REPRES_TARGET_SHIP_NOT_FOUND, ship_code));
            bres = false;
        }
        else
        {
            std::string system_name = shipRow[0][11];
            std::string at_hex = shipRow[0][0];

            auto base_check = db.Query(
                "SELECT 1 FROM base_stars WHERE game_id=? AND hex_id=? "
                "AND owner=?",
                {s.game_id, at_hex, owner});

            bool can_repair = !base_check.empty();

            if (!can_repair && !system_name.empty())
            {
                can_repair =
                    FacilityEngine::can_repair_at(s.game_id, system_name,
                                                  owner);
            }

            if (!can_repair)
            {
                Telemetry::instance().write(LC_REPRES_SHIP_MUST_BE_AT_FACILITY);
                bres = false;
            }
            else if (amount <= 0)
            {
                Telemetry::instance().write(
                    std::format(LC_REPRES_TARGET_SPECIFY, ship_code));
                bres = false;
            }
            else
            {
                std::string col;
                std::string origCol;
                int current = 0;
                int orig = 0;

                switch (attr)
                {
                case AttributeID::POWER_DRIVE:
                {
                    col = "pd";
                    origCol = "pd_max";
                    current = std::atoi(shipRow[0][1].c_str());
                    orig = std::atoi(shipRow[0][6].c_str());
                    break;
                }
                case AttributeID::PHASIC:
                {
                    col = "phasic";
                    origCol = "phasic_max";
                    current = std::atoi(shipRow[0][2].c_str());
                    orig = std::atoi(shipRow[0][7].c_str());
                    break;
                }
                case AttributeID::SHIELD:
                {
                    col = "shield";
                    origCol = "shield_max";
                    current = std::atoi(shipRow[0][3].c_str());
                    orig = std::atoi(shipRow[0][8].c_str());
                    break;
                }
                case AttributeID::LAUNCHER:
                {
                    col = "launcher";
                    origCol = "launcher_max";
                    current = std::atoi(shipRow[0][4].c_str());
                    orig = std::atoi(shipRow[0][9].c_str());
                    break;
                }
                default:
                {
                    bres = false;
                    break;
                }
                }

                if (!col.empty())
                {
                    int maxRepair = orig - current;
                    if (maxRepair <= 0)
                    {
                        Telemetry::instance().write(LC_REPRES_AT_MAX);
                        bres = false;
                    }
                    else
                    {
                        int repairAmt = std::min(amount, maxRepair);

                        int repairMod =
                            StarSystemConstraints::getRepairModifier(s.game_id,
                                                                     at_hex);
                        repairAmt = std::max(1, repairAmt + repairMod);
                        repairAmt = std::min(repairAmt, maxRepair);

                        int cost = repairAmt * 20;

                        int availBP =
                            (KH_EQU(owner, 'A')) ? s.creditsA : s.creditsB;

                        if (cost > availBP)
                        {
                            Telemetry::instance().write(
                                std::format(LC_REPRES_COST_LIMIT, cost,
                                            availBP));
                            bres = false;
                        }
                        else
                        {
                            int newVal = current + repairAmt;
                            db.Exec(
                                std::format("UPDATE ships SET {}=? WHERE "
                                            "game_id=? AND ship_code=?",
                                            col),
                                {newVal, s.game_id, ship_code});

                            if (KH_EQU(owner, 'A'))
                            {
                                s.creditsA -= cost;
                            }
                            else
                            {
                                s.creditsB -= cost;
                            }
                            StateMachine::instance().save_game(s);

                            Telemetry::instance().write(
                                std::format(LC_REPRES_TARGET_SUCCEEDED,
                                            ship_code, col, repairAmt, cost));
                            bres = true;
                        }
                    }
                }
            }
        }
    }

    return bres;
}

bool RepresAgent::apply(ResupplyParam& param)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    ResupplyMode mode = param.get_mode();

    if (mode == ResupplyMode::RESUPPLY_LIST)
    {
        auto rows = db.Query(
            "SELECT s.ship_code, s.ship_name, s.at_hex, s.torpedoes, "
            "s.torpedoes_max "
            "FROM ships s "
            "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = "
            "s.game_id "
            "WHERE s.game_id=? AND s.owner=? AND bs.owner=? "
            "AND s.torpedoes < s.torpedoes_max AND s.destroyed_at IS NULL",
            {s.game_id, owner, owner});

        if (rows.empty())
        {
            Telemetry::instance().write(LC_REPRES_NO_TORPEDO_NEEDED);
        }
        else
        {
            std::ostringstream out;
            out << LC_REPRES_RESUP_LIST_BANNER << ":\n";
            for (const auto& r : rows)
            {
                int cur = std::atoi(r[3].c_str());
                int max = std::atoi(r[4].c_str());
                out << "  " << r[0] << " (" << r[1] << ") - Torpedoes: "
                    << cur << "/" << max << "\n";
            }
            out << LC_REPRES_RESUPPLY_HINT << "\n"
                << LC_REPRES_RESUPPLY_EXAMPLE << "\n";
            Telemetry::instance().write(out.str());
        }
        bres = true;
    }
    else
    {
        std::string ship_code = param.get_ship_code();
        int qty = param.get_torpedoes();

        auto shipRow = db.Query(
            "SELECT s.torpedoes, s.torpedoes_max, s.at_hex "
            "FROM ships s "
            "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = "
            "s.game_id "
            "WHERE s.game_id=? AND s.owner=? AND s.ship_code=? AND bs.owner=? "
            "AND s.destroyed_at IS NULL",
            {s.game_id, owner, ship_code, owner});

        if (shipRow.empty())
        {
            Telemetry::instance().write(LC_REPRES_RESUPPLY_SHIP_NOT_AT_BASE);
            bres = false;
        }
        else
        {
            int curTorpedoes = std::atoi(shipRow[0][0].c_str());
            int maxTorpedoes = std::atoi(shipRow[0][1].c_str());
            std::string at_hex = shipRow[0][2];
            int canAdd = maxTorpedoes - curTorpedoes;

            if (canAdd <= 0)
            {
                Telemetry::instance().write(LC_REPRES_REPAIR_TORPEDO_AT_MAX);
                bres = false;
            }
            else if (qty <= 0)
            {
                Telemetry::instance().write(std::format(
                    LC_REPRES_RESUPPLY_TARGET_SPECIFY_TORPEDO, ship_code));
                bres = false;
            }
            else
            {
                int addAmt = std::min(qty, canAdd);

                int resupplyMod =
                    StarSystemConstraints::getResupplyModifier(s.game_id,
                                                               at_hex);
                addAmt = std::max(1, addAmt + resupplyMod);
                addAmt = std::min(addAmt, canAdd);

                int cost = ((addAmt + 2) / 3) * 20;

                int availBP =
                    (KH_EQU(owner, 'A')) ? s.creditsA : s.creditsB;

                if (cost > availBP)
                {
                    Telemetry::instance().write(
                        std::format(LC_REPRES_TARGET_RESUPPLY_COST_LIMIT, cost,
                                    availBP));
                    bres = false;
                }
                else
                {
                    int newTorpedoes = curTorpedoes + addAmt;
                    db.Exec(
                        "UPDATE ships SET torpedoes=? WHERE game_id=? "
                        "AND ship_code=?",
                        {newTorpedoes, s.game_id, ship_code});

                    if (KH_EQU(owner, 'A'))
                    {
                        s.creditsA -= cost;
                    }
                    else
                    {
                        s.creditsB -= cost;
                    }
                    StateMachine::instance().save_game(s);

                    Telemetry::instance().write(
                        std::format(LC_REPRES_TARGET_RESUPPLY_SUCCEEDED,
                                    ship_code, addAmt, cost));
                    bres = true;
                }
            }
        }
    }

    return bres;
}
