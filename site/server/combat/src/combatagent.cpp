///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "combatagent.h"

#include <sstream>

#include "ce.h"
#include "db.h"
#include "logger.h"
#include "telemetry.h"
#include "statemachine.h"

CombatSessionState
CombatAgent::get_combat_state_at_hex(const int gid, const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::instance();

    // BIGBUG need to be more precise about the combat_state record that is
    // found. module id and players involved?
    std::string q =
        "SELECT round, stage, attacker_remains, stalemate_counter, last_log "
        "FROM combat_state WHERE game_id=? AND hex_id=?";
    auto rows = db.Query(q, {gid, hex_id});
    if (rows.empty())
    {
        return CombatSessionState();
    }

    return CombatSessionState(gid, hex_id, std::atoi(rows[0][0].c_str()),
                              std::atoi(rows[0][1].c_str()),
                              (KH_EQU(rows[0][2], "1")),
                              std::atoi(rows[0][3].c_str()), rows[0][4]);
}

bool CombatAgent::is_param_valid(const CombatAgentPayload& param) const
{
    bool bresult = false;
    std::visit(
        [&](const auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, CombatParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatOrderParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatApplyParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatRetreatParam>)
            {
                bresult = true;
            }
        },
        param);

    return bresult;
}

bool CombatAgent::apply(CombatAgentParam& caparam)
{
    bool bresult = false;

    if (caparam.isCombatOrderParam())
    {
        CombatOrderParam& param = caparam.asCombatOrderParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatApplyParam())
    {
        CombatApplyParam& param = caparam.asCombatApplyParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatRetreatParam())
    {
        CombatRetreatParam& param = caparam.asCombatRetreatParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatCommitParam())
    {
        CombatCommitParam& param = caparam.asCombatCommitParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatCancelParam())
    {
        CombatCancelParam& param = caparam.asCombatCancelParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatDraftsParam())
    {
        CombatDraftsParam& param = caparam.asCombatDraftsParam();
        bresult = apply(param);
    }
    else if (caparam.isCombatStatusParam())
    {
        CombatStatusParam& param = caparam.asCombatStatusParam();
        bresult = apply(param);
    }
    else
    {
        // we don't know what this is.
        bresult = false;
    }

    return bresult;
}

////////////////////////////////////////////////////////////////////////////

// We don't have a use for the bare command: "combat" yet..
bool CombatAgent::apply(CombatParam& param)
{
    return false;
}

bool CombatAgent::apply(CombatOrderParam& param)
{
    DatabaseManager& db = DatabaseManager::instance();

    bool bresult = false;
    if (!is_param_valid(param))
    {
        bresult = false;
        return bresult;
    }

    int gid = param.get_game_id();
    int mid = param.get_module_id();
    char player = param.get_player();

    FightingPlayers combatting_players{'A', 'B'};
    if (KH_EQU(player, 'B'))
    {
        combatting_players = FightingPlayers{'B', 'A'};
    }

    std::string attacker(param.get_attacker());
    std::string attackee(param.get_attackee());
    AttributeMap combat_attr = param.get_attr();
    char tactic = param.get_tactic();

    // BIGBUG TORPEDO SET needs to conform to the database schema.
    TorpedoSet torpedoes_fired = param.get_torpedoes();

    std::string packed_torpedo_data =
        join_vector(torpedoes_fired, std::string(","));

    /////////////////////////////////////////////////////////
    // 1.
    // Validate Ship Ownership of the Attacker
    // Validate Ship Ownership of the Attackee
    /////////////////////////////////////////////////////////

    // Lookup attacker: does the ship exist, who owns it, is it alive?
    std::string ship_lookup =
        "SELECT S.owner, S.destroyed_at IS NOT NULL AS is_destroyed "
        "FROM ships S WHERE S.game_id=? "
        "AND (S.ship_code=? OR S.ship_name=?)";

    auto attacker_rows = db.Query(ship_lookup, {gid, attacker, attacker});
    if (attacker_rows.empty())
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_NOT_EXIST, attacker));
        return false;
    }
    char attacker_actual_owner = attacker_rows[0][0][0];
    bool attacker_destroyed = (KH_EQU(attacker_rows[0][1], "1"));
    if (attacker_destroyed)
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_DESTROYED, attacker));
        return false;
    }
    if (attacker_actual_owner != combatting_players.first)
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_NOT_YOURS, attacker));
        return false;
    }

    // Lookup attackee (target): same checks against the opponent
    auto attackee_rows = db.Query(ship_lookup, {gid, attackee, attackee});
    if (attackee_rows.empty())
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_NOT_EXIST, attackee));
        return false;
    }
    char attackee_actual_owner = attackee_rows[0][0][0];
    bool attackee_destroyed = (KH_EQU(attackee_rows[0][1], "1"));
    if (attackee_destroyed)
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_DESTROYED, attackee));
        return false;
    }
    if (attackee_actual_owner != combatting_players.second)
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_NOT_HOSTILE, attackee));
        return false;
    }

    /////////////////////////////////////////////////////////
    // 2.
    // Validate combat state and stats
    ////////////////////////////////////////////////////////
    std::string query_stats_attacker =
        "SELECT at_hex, pd, phasic, shield, launcher, torpedoes, hangar "
        "FROM ships WHERE game_id=? AND destroyed_at IS NULL "
        "AND (ship_code=? OR ship_name=?)";
    auto attacker_stats =
        db.Query(query_stats_attacker, {gid, attacker, attacker});

    if (attacker_stats.empty() || attacker_stats[0][0].empty())
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_SHIP_NOT_DEPLOYED, attacker));
        return false;
    }

    std::string combat_hex_id = attacker_stats[0][0];
    AttributeMap max_attack_attr;
    max_attack_attr[AttributeID::POWER_DRIVE] =
        std::atoi(attacker_stats[0][1].c_str());
    max_attack_attr[AttributeID::PHASIC] =
        std::atoi(attacker_stats[0][2].c_str());
    max_attack_attr[AttributeID::SHIELD] =
        std::atoi(attacker_stats[0][3].c_str());
    max_attack_attr[AttributeID::LAUNCHER] =
        std::atoi(attacker_stats[0][4].c_str());
    max_attack_attr[AttributeID::TORPEDO] =
        std::atoi(attacker_stats[0][5].c_str());
    max_attack_attr[AttributeID::HANGAR] =
        std::atoi(attacker_stats[0][6].c_str());

    ////////////////////////////////////////////
    // 2.1
    // Validate there is even combat at this hex?

    auto combat_state = get_combat_state_at_hex(gid, combat_hex_id);

    if (KH_EQU(combat_state.game_id, 0))
    {
        // No combat record - check if this is a voluntary trigger by the initiative player
        GameState ngs = StateMachine::instance().get_game_state();
        char chActive = ngs.active_player.empty() ? 'A' : ngs.active_player[0];

        if (player != chActive)
        {
            Telemetry::instance().write(LC_COMBAT_INITIATIVE_PLAYER_ONLY);
            return false;
        }

        // Verify opposing forces are present in this hex
        auto oppose_check = db.Query(
            "SELECT COUNT(*) FROM ships WHERE game_id=? AND at_hex=? "
            "AND owner=? AND destroyed_at IS NULL "
            "AND (racked_in IS NULL OR racked_in = '')",
            {gid, combat_hex_id, combatting_players.second});

        int nOppose = std::atoi(oppose_check[0][0].c_str());
        if (nOppose == 0)
        {
            Telemetry::instance().write(LC_COMBAT_NO_RED_FORCES);
            return false;
        }

        // Initiative player voluntarily triggers combat
        CombatEngine ce(gid);
        ce.create_combat(combat_hex_id);
        combat_state = get_combat_state_at_hex(gid, combat_hex_id);
    }

    if (combat_state.stage != 0)
    {
        Telemetry::instance().write(
            std::format(LC_COMBAT_NO_ORDERS, combat_state.stage));
        return false;
    }

    ////////////////////////////////////////////
    // 2.3
    // Validate Power Limits
    ////////////////////////////////////////////
    //  2.3.1
    //   Check if the ordered PHASIC
    //   power is exceeding ship's PHASIC power
    ////////////////////////////////////////////
    std::string limit_errors;
    bool cls = true;
    if (combat_attr[AttributeID::PHASIC] > max_attack_attr[AttributeID::PHASIC])
    {
        limit_errors.append("Phasic power orderd (P=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::PHASIC]));
        limit_errors.append(") exceeds ship rating (P=");
        limit_errors.append(std::to_string(max_attack_attr[AttributeID::PHASIC]));
        limit_errors.append(")\n");
        cls = false;
    }
    if (combat_attr[AttributeID::SHIELD] > max_attack_attr[AttributeID::SHIELD])
    {
        limit_errors.append("Shield power orderd (S=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::SHIELD]));
        limit_errors.append(") exceeds ship rating (S=");
        limit_errors.append(
            std::to_string(max_attack_attr[AttributeID::SHIELD]));
        limit_errors.append(")\n");
        cls = false;
    }
    if (combat_attr[AttributeID::LAUNCHER] > max_attack_attr[AttributeID::LAUNCHER])
    {
        limit_errors.append("Shield power orderd (L=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::LAUNCHER]));
        limit_errors.append(") exceeds ship rating (L=");
        limit_errors.append(std::to_string(max_attack_attr[AttributeID::LAUNCHER]));
        limit_errors.append(")\n");
        cls = false;
    }

    int total_power_ordered = combat_attr[AttributeID::LAUNCHER] +
                              combat_attr[AttributeID::SHIELD] +
                              combat_attr[AttributeID::PHASIC];

    if (total_power_ordered > max_attack_attr[AttributeID::POWER_DRIVE])
    {
        limit_errors.append("Total power ordered (P+S+L = ");
        limit_errors.append(std::to_string(total_power_ordered));
        limit_errors.append(") exceeds ship rating (PD=");
        limit_errors.append(
            std::to_string(max_attack_attr[AttributeID::POWER_DRIVE]));
        limit_errors.append(")\n");
        cls = false;
    }

    if (!cls)
    {
        Logger::instance().debug(limit_errors);
        // we have errors in the ordered combat attributes
        // limit_errors has the errors.
        return false;
    }

    // We are ok to proceed with this new combat order.

    // 3. Insert/Update
    std::string combat_order =
        "INSERT INTO combat_orders "
        "(game_id, owner, ship_code, round, tactic, target_id, "
        "power_d, power_b, power_s, power_t, torpedoes_data, committed) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0) "
        "ON DUPLICATE KEY UPDATE "
        "tactic=?, target_id=?, power_d=?, power_b=?, power_s=?, power_t=?, "
        "torpedoes_data=?, committed=0";

    db.Exec(combat_order,
            {gid, combatting_players.first, attacker, combat_state.round,
             tactic, attackee, combat_attr[AttributeID::POWER_DRIVE],
             combat_attr[AttributeID::PHASIC], combat_attr[AttributeID::SHIELD],
             combat_attr[AttributeID::LAUNCHER], packed_torpedo_data, tactic,
             attackee, combat_attr[AttributeID::POWER_DRIVE],
             combat_attr[AttributeID::PHASIC], combat_attr[AttributeID::SHIELD],
             combat_attr[AttributeID::LAUNCHER], packed_torpedo_data});

    // Resolution triggered by explicit 'combat commit'
    return true;
}

bool CombatAgent::apply(CombatApplyParam& param)
{
    DatabaseManager& db = DatabaseManager::instance();

    int gid = param.get_game_id();
    char owner = param.get_player();
    std::string target_ship = param.get_target_ship();
    AttributeMap assignments = param.get_assignments();

    // Apply damage via combat engine
    CombatEngine ce(gid);
    std::string result = ce.apply_damage(owner, target_ship, assignments);
    Telemetry::instance().write(result);
    return true;
}

bool CombatAgent::apply(CombatRetreatParam& param)
{
    return false;
}

bool CombatAgent::apply(CombatCommitParam& param)
{
    DatabaseManager& db = DatabaseManager::instance();

    int gid = param.get_game_id();
    char owner = param.get_player();

    // Get the active combat hex
    auto gameRow =
        db.Query("SELECT active_combat_hex FROM games WHERE id=?", {gid});

    std::string activeHex;
    if (!gameRow.empty() && !gameRow[0][0].empty())
    {
        activeHex = gameRow[0][0];
    }

    // Get all hexes with uncommitted orders for this player
    std::string hex_q =
        "SELECT DISTINCT s.at_hex FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner "
        "AND s.ship_code=co.ship_code "
        "WHERE co.game_id=? AND co.owner=? AND co.committed=0 "
        "AND s.destroyed_at IS NULL";
    auto hexRows = db.Query(hex_q, {gid, owner});

    if (hexRows.empty())
    {
        Telemetry::instance().write(LC_COMBAT_NO_QUEUED_ORDERS);
        return true;
    }

    // Validate orders are for active hex (if one is set)
    if (!activeHex.empty())
    {
        for (const auto& row : hexRows)
        {
            if (row[0] != activeHex)
            {
                Telemetry::instance().write(
                    std::format(LC_COMBAT_PENDING_ORDER_ERR, row[0], activeHex));
                return false;
            }
        }
    }

    // BIGBUG: commits ALL uncommitted orders for this player across all hexes.
    // Should scope to the active combat hex to avoid cross-hex contamination
    // when multiple hexes have concurrent combats.
    db.Exec("UPDATE combat_orders SET committed=1 WHERE game_id=? "
            "AND owner=? AND committed=0",
            {gid, owner});

    Telemetry::instance().write(LC_COMBAT_ORDERS_TX);

    // Check each affected hex for resolution
    CombatEngine ce(gid);
    for (const auto& row : hexRows)
    {
        std::string hex_id = row[0];
        auto cs = ce.get_combat_state(hex_id);

        if (ce.all_orders_committed(hex_id, cs.round))
        {
            // Reveal all orders to both players before resolution (per rules)
            std::string orders_q =
                "SELECT co.owner, co.ship_code, co.tactic, co.target_id, "
                "co.power_d, co.power_b, co.power_s, co.power_t "
                "FROM combat_orders co "
                "JOIN ships s ON s.game_id=co.game_id AND "
                "s.ship_code=co.ship_code AND s.owner=co.owner "
                "WHERE co.game_id=? AND s.at_hex=? AND co.round=? "
                "AND s.destroyed_at IS NULL ORDER BY co.owner, co.ship_code";
            auto orders = db.Query(orders_q, {gid, hex_id, cs.round});

            std::ostringstream reveal;
            reveal << "=== COMBAT ORDERS REVEALED ===\n";
            for (const auto& ord : orders)
            {
                char t = ord[2][0];
                std::string tactic;
                switch (t)
                {
                case 'A':
                    tactic = "Attack";
                    break;
                case 'D':
                    tactic = "Attack";
                    break;
                case 'E':
                    tactic = "Attack";
                    break;
                }
                reveal << "  " << ord[0] << ":" << ord[1] << " " << tactic
                       << " " << ord[3] << " [D=" << ord[4] << " P=" << ord[5]
                       << " S=" << ord[6] << " L=" << ord[7] << "]\n";
            }
            reveal << "==============================";
            Telemetry::instance().add_broadcast(gid, reveal.str());

            std::string result = ce.resolve_round(hex_id);
            Telemetry::instance().write(result);
        }
        else
        {
            // Notify opponent that this player has committed
            char opponent = owner ^ 0x03;

            Telemetry::instance().add_tell(opponent,
                std::format(LC_COMBAT_RED_FORCE_HAS_ORDERS, hex_id));

            Telemetry::instance().write(
                std::format(LC_COMBAT_WAIT_RED_FORCE_ORDERS, hex_id));
        }
    }
    return true;
}

bool CombatAgent::apply(CombatCancelParam& param)
{
    DatabaseManager& db = DatabaseManager::instance();

    int gid = param.get_game_id();
    char owner = param.get_player();

    // Check if there are any uncommitted orders to cancel (Bug #1)
    auto orderRows =
        db.Query("SELECT COUNT(*) FROM combat_orders WHERE game_id=? "
                 "AND owner=? AND committed=0",
                 {gid, owner});

    if (orderRows.empty() || KH_EQU(orderRows[0][0], "0"))
    {
        Telemetry::instance().write(LC_COMBAT_ORDERS_NO_RX);
        return true;
    }

    // Delete all uncommitted orders for this player
    db.Exec("DELETE FROM combat_orders WHERE game_id=? AND owner=? "
            "AND committed=0",
            {gid, owner});

    Telemetry::instance().write(LC_COMBAT_ORDERS_RECINDED);
    return true;
}

bool CombatAgent::apply(CombatDraftsParam& param)
{
    DatabaseManager& db = DatabaseManager::instance();

    int gid = param.get_game_id();
    char owner = param.get_player();

    // Query uncommitted orders for this player, grouped by hex
    std::string drafts_q =
        "SELECT s.at_hex, co.ship_code, co.tactic, co.target_id, "
        "co.power_d, co.power_b, co.power_s, co.power_t, co.torpedoes_data "
        "FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND "
        "s.ship_code=co.ship_code "
        "WHERE co.game_id=? AND co.owner=? AND co.committed=0 "
        "AND s.destroyed_at IS NULL ORDER BY s.at_hex, co.ship_code";
    auto rows = db.Query(drafts_q, {gid, owner});

    if (rows.empty())
    {
        Telemetry::instance().write(LC_COMBAT_NO_PENDING_ORDERS);
        return true;
    }

    std::ostringstream out;
    out << "Pending Combat Orders:\n";
    std::string lastHex;
    for (const auto& r : rows)
    {
        if (r[0] != lastHex)
        {
            out << "  Hex " << r[0] << ":\n";
            lastHex = r[0];
        }
        char tactic = r[2].empty() ? 'A' : r[2][0];
        std::string tacticName = (KH_EQU(tactic, 'A'))   ? "Attack"
                                 : (KH_EQU(tactic, 'D')) ? "Dodge"
                                                   : "Escape";
        out << "    " << r[1] << ": " << tacticName << " -> "
            << (r[3].empty() ? "(none)" : r[3]) << " [D=" << r[4]
            << " P=" << r[5] << " S=" << r[6] << " L=" << r[7];
        if (!r[8].empty())
        {
            out << " M=" << r[8];
        }
        out << "]\n";
    }
    Telemetry::instance().write(out.str());
    return true;
}

bool CombatAgent::apply(CombatStatusParam& param)
{
    return false;
}
