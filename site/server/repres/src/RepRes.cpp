///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "db.h"
#include "facilities.h"
#include "repair_command.h"
#include "resupply_command.h"
#include "star_system_constraints.h"
#include "statemachine.h"
#include "telemetry.h"

#include <sstream>

bool RepairCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // If no ship specified, list repairable ships
    if (m_ship_code.empty())
    {
        // Find ships at player's base stars that have damage
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
            "s.shield < s.shield_max OR s.launcher < s.launcher_max OR "
            "s.torpedoes < s.torpedoes_max)",
            {s.game_id, owner, owner});

        if (rows.empty())
        {
            Telemetry::instance().write(LC_REPRES_NO_DAMAGED_SHIPS_AT_BASES);
            return true;
        }

        std::ostringstream out;
        out << LC_REPRES_LIST_BANNER ":\n";
        for (const auto& r : rows)
        {
            out << "  " << r[0] << " (" << r[1] << ") at " << r[2] << "\n";
        }
        out << LC_REPRES_HINT << "\n" << LC_REPRES_EXAMPLE << "\n";
        Telemetry::instance().write(out.str());
        return true;
    }

    // Validate ship exists and is at player's base star OR controlled repair
    // facility
    auto shipRow = db.Query(
        "SELECT s.at_hex, s.pd, s.phasic, s.shield, s.launcher, s.torpedoes, "
        "s.pd_max, s.phasic_max, s.shield_max, s.launcher_max, "
        "s.torpedoes_max, "
        "ss.name "
        "FROM ships s "
        "LEFT JOIN star_systems ss ON ss.hex_id = s.at_hex AND ss.module_id = "
        "1 "
        "WHERE s.game_id=? AND s.owner=? AND s.ship_code=? "
        "AND s.destroyed_at IS NULL",
        {s.game_id, owner, m_ship_code});

    if (shipRow.empty())
    {
        Telemetry::instance().write(
            std::format(LC_REPRES_TARGET_SHIP_NOT_FOUND, m_ship_code));
        return false;
    }

    std::string system_name = shipRow[0][11];
    std::string at_hex = shipRow[0][0];

    // Check if at base star
    auto base_check = db.Query(
        "SELECT 1 FROM base_stars WHERE game_id=? AND hex_id=? AND owner=?",
        {s.game_id, at_hex, owner});

    bool can_repair = !base_check.empty();

    // Also check for controlled repair facility
    if (!can_repair && !system_name.empty())
    {
        can_repair =
            FacilityEngine::can_repair_at(s.game_id, system_name, owner);
    }

    if (!can_repair)
    {
        Telemetry::instance().write(LC_REPRES_SHIP_MUST_BE_AT_FACILITY);
        return false;
    }

    if (m_attribute.empty() || m_amount <= 0)
    {
        Telemetry::instance().write(
            std::format(LC_REPRES_TARGET_SPECIFY, m_ship_code));
        return false;
    }

    // Map attribute to columns

    // BUGBUG WHY ARE we still using strings as keys this way??
    std::string col, origCol;
    if (KH_EQU(m_attribute, "pd") || KH_EQU(m_attribute, "d"))
    {
        col = "pd";
        origCol = "pd_max";
    }
    else if (KH_EQU(m_attribute, "b") || KH_EQU(m_attribute, "phasic"))
    {
        col = "phasic";
        origCol = "phasic_max";
    }
    else if (KH_EQU(m_attribute, "s") || KH_EQU(m_attribute, "shield"))
    {
        col = "shield";
        origCol = "shield_max";
    }
    else if (KH_EQU(m_attribute, "t") || KH_EQU(m_attribute, "launcher"))
    {
        col = "launcher";
        origCol = "launcher_max";
    }
    else
    {
        Telemetry::instance().write(LC_REPRES_INVALID_ATTR);
        return false;
    }

    int current = 0, orig = 0;
    if (KH_EQU(col, "pd"))
    {
        current = std::atoi(shipRow[0][1].c_str());
        orig = std::atoi(shipRow[0][6].c_str());
    }
    else if (KH_EQU(col, "phasic"))
    {
        current = std::atoi(shipRow[0][2].c_str());
        orig = std::atoi(shipRow[0][7].c_str());
    }
    else if (KH_EQU(col, "shield"))
    {
        current = std::atoi(shipRow[0][3].c_str());
        orig = std::atoi(shipRow[0][8].c_str());
    }
    else if (KH_EQU(col, "launcher"))
    {
        current = std::atoi(shipRow[0][4].c_str());
        orig = std::atoi(shipRow[0][9].c_str());
    }

    int maxRepair = orig - current;
    if (maxRepair <= 0)
    {
        Telemetry::instance().write(LC_REPRES_AT_MAX);
        return false;
    }

    int repairAmt = std::min(m_amount, maxRepair);

    // Apply environmental repair modifier
    int repairMod = StarSystemConstraints::getRepairModifier(s.game_id, at_hex);
    repairAmt = std::max(1, repairAmt + repairMod);
    repairAmt = std::min(repairAmt, maxRepair); // Can't exceed max

    int cost = repairAmt * 20; // 20 CR per unit (inflated ×20)

    // Check BP
    int availBP = (KH_EQU(owner, 'A')) ? s.creditsA : s.creditsB;
    if (cost > availBP)
    {
        Telemetry::instance().write(
            std::format(LC_REPRES_COST_LIMIT, cost, availBP));
        return false;
    }

    // Apply repair
    int newVal = current + repairAmt;
    db.Exec("UPDATE ships SET " + col + "=? WHERE game_id=? AND ship_code=?",
            {newVal, s.game_id, m_ship_code});

    // Deduct BP via GameState
    if (KH_EQU(owner, 'A'))
    {
        s.creditsA -= cost;
    }
    else
    {
        s.creditsB -= cost;
    }
    StateMachine::instance().save_game(s);

    Telemetry::instance().write(std::format(LC_REPRES_TARGET_SUCCEEDED,
                                            m_ship_code, col, repairAmt, cost));
    return true;
}

bool ResupplyCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    // If no ship specified, list ships that can be resupplied
    if (m_ship_code.empty())
    {
        // Find ships at player's base stars that can take torpedoes
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
            return true;
        }

        std::ostringstream out;
        out << LC_REPRES_RESUP_LIST_BANNER << ":\n";
        for (const auto& r : rows)
        {
            int cur = std::atoi(r[3].c_str());
            int max = std::atoi(r[4].c_str());
            out << "  " << r[0] << " (" << r[1] << ") - Torpedoes: " << cur
                << "/" << max << "\n";
        }
        out << LC_REPRES_RESUPPLY_HINT << "\n"
            << LC_REPRES_RESUPPLY_EXAMPLE << "\n";
        Telemetry::instance().write(out.str());
        return true;
    }

    // Validate ship exists and is at player's base star
    auto shipRow = db.Query(
        "SELECT s.torpedoes, s.torpedoes_max, s.at_hex "
        "FROM ships s "
        "JOIN base_stars bs ON bs.hex_id = s.at_hex AND bs.game_id = s.game_id "
        "WHERE s.game_id=? AND s.owner=? AND s.ship_code=? AND bs.owner=? "
        "AND s.destroyed_at IS NULL",
        {s.game_id, owner, m_ship_code, owner});

    if (shipRow.empty())
    {
        Telemetry::instance().write(LC_REPRES_RESUPPLY_SHIP_NOT_AT_BASE);
        return false;
    }

    int curTorpedoes = std::atoi(shipRow[0][0].c_str());
    int maxTorpedoes = std::atoi(shipRow[0][1].c_str());
    std::string at_hex = shipRow[0][2];
    int canAdd = maxTorpedoes - curTorpedoes;

    if (canAdd <= 0)
    {
        Telemetry::instance().write(LC_REPRES_REPAIR_TORPEDO_AT_MAX);
        return false;
    }

    if (m_torpedoes <= 0)
    {
        Telemetry::instance().write(std::format(
            LC_REPRES_RESUPPLY_TARGET_SPECIFY_TORPEDO, m_ship_code));
        return false;
    }

    int addAmt = std::min(m_torpedoes, canAdd);

    // Apply environmental resupply modifier
    int resupplyMod =
        StarSystemConstraints::getResupplyModifier(s.game_id, at_hex);
    addAmt = std::max(1, addAmt + resupplyMod);
    addAmt = std::min(addAmt, canAdd); // Can't exceed capacity

    int cost = ((addAmt + 2) / 3) * 20; // 20 CR per 3 torpedoes (inflated ×20)

    // Check BP
    int availBP = (KH_EQU(owner, 'A')) ? s.creditsA : s.creditsB;
    if (cost > availBP)
    {
        Telemetry::instance().write(
            std::format(LC_REPRES_TARGET_RESUPPLY_COST_LIMIT, cost, availBP));
        return false;
    }

    // Apply resupply
    int newTorpedoes = curTorpedoes + addAmt;
    db.Exec("UPDATE ships SET torpedoes=? WHERE game_id=? AND ship_code=?",
            {newTorpedoes, s.game_id, m_ship_code});

    // Deduct BP via GameState
    if (KH_EQU(owner, 'A'))
    {
        s.creditsA -= cost;
    }
    else
    {
        s.creditsB -= cost;
    }
    StateMachine::instance().save_game(s);

    Telemetry::instance().write(std::format(LC_REPRES_TARGET_RESUPPLY_SUCCEEDED,
                                            m_ship_code, addAmt, cost));
    return true;
}
