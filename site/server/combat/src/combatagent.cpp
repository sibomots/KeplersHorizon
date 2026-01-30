#include "combatagent.h"
#include "ce.h"
#include "db.h"
#include "telemetry.h"
#include "logger.h"
#include <sstream>

CombatSessionState
CombatAgent::get_combat_state_at_hex(const int gid, const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::instance();

    // BUGBUG need to be more precise about the combat_state record that is
    // found. module id and players involved?
    std::string q =
        "SELECT round, stage, attacker_remains, stalemate_counter, last_log "
        "FROM combat_state WHERE game_id=? AND hex_id=?";
    auto rows = db.Query(q, {gid, hex_id});
    if (rows.empty())
    {
        return CombatSessionState();
    }

    return CombatSessionState(gid, hex_id,
                              std::atoi(rows[0][0].c_str()),
                              std::atoi(rows[0][1].c_str()),
                              (rows[0][2] == "1"),
                              std::atoi(rows[0][3].c_str()),
                              rows[0][4]);
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
    else {
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
    if (player == 'B')
    {
        combatting_players = FightingPlayers{'B', 'A'};
    }

    std::string attacker(param.get_attacker());
    std::string attackee(param.get_attackee());
    AttributeMap combat_attr = param.get_attr();
    char tactic = param.get_tactic();

    // BUGBUG MISSILE SET needs to conform to the database schema.
    MissileSet missiles_fired = param.get_missiles();

    std::string packed_missile_data =
        join_vector(missiles_fired, std::string(","));

    /////////////////////////////////////////////////////////
    // 1.
    // Validate Ship Ownership of the Attacker
    // Validate Ship Ownership of the Attackee
    /////////////////////////////////////////////////////////

    std::string pick_attacker =
        "SELECT S.owner FROM ships S WHERE S.game_id=? "
        "AND S.destroyed_at IS NULL AND S.owner=? "
        "AND (S.ship_code=? OR S.ship_name=?)";
    auto attacker_owner = db.Query(pick_attacker,
        {gid, combatting_players.first, attacker, attacker});

    if (attacker_owner.empty())
    {
        // BUGBUG
        // ship is not found
        return false;
    }
    std::string pick_attackee =
        "SELECT S.owner FROM ships S WHERE S.game_id=? "
        "AND S.destroyed_at IS NULL AND S.owner=? "
        "AND (S.ship_code=? OR S.ship_name=?)";
    auto attackee_owner = db.Query(pick_attackee,
        {gid, combatting_players.second, attackee, attackee});

    if (attackee_owner.empty())
    {
        // BUGBUG
        // ship is not found
        return false;
    }

    /////////////////////////////////////////////////////////
    // 2.
    // Validate combat state and stats
    ////////////////////////////////////////////////////////
    std::string query_stats_attacker =
        "SELECT at_hex, pd, beam, screen, tube, missiles, sr "
        "FROM ships WHERE game_id=? AND destroyed_at IS NULL "
        "AND (ship_code=? OR ship_name=?)";
    auto attacker_stats = db.Query(query_stats_attacker, {gid, attacker, attacker});

    if (attacker_stats.empty() || attacker_stats[0][0].empty())
    {
        // BUGBUG
        // didn't find the ship?   How can this be?
        // return "Ship not in space";
        return false;
    }

    std::string combat_hex_id = attacker_stats[0][0];
    AttributeMap max_attack_attr;
    max_attack_attr[AttributeID::POWER_DRIVE] =
        std::atoi(attacker_stats[0][1].c_str());
    max_attack_attr[AttributeID::BEAM] =
        std::atoi(attacker_stats[0][2].c_str());
    max_attack_attr[AttributeID::SCREEN] =
        std::atoi(attacker_stats[0][3].c_str());
    max_attack_attr[AttributeID::TUBE] =
        std::atoi(attacker_stats[0][4].c_str());
    max_attack_attr[AttributeID::MISSILE] =
        std::atoi(attacker_stats[0][5].c_str());
    max_attack_attr[AttributeID::SYSTEM_RACK] =
        std::atoi(attacker_stats[0][6].c_str());

    ////////////////////////////////////////////
    // 2.1
    // Validate there is even combat at this hex?
    // BUGBUG assumes only two players, still need to filter on the same
    // game_id, module_id, players fighting.. etc.. BUGBUG

    auto combat_state = get_combat_state_at_hex(gid, combat_hex_id);

    if (combat_state.game_id == 0)
    {
        // BUGBUG no combnat at this hex
        return false;
    }
    if (combat_state.stage != 0)
    {
        // not accepting orders. Current stage is
        // std::to_string(combat_state.stage)
        return false;
    }

    ////////////////////////////////////////////
    // 2.3
    // Validate Power Limits
    ////////////////////////////////////////////
    //  2.3.1
    //   Check if the ordered BEAM
    //   power is exceeding ship's BEAM power
    ////////////////////////////////////////////
    std::string limit_errors;
    bool cls = true;
    if (combat_attr[AttributeID::BEAM] > max_attack_attr[AttributeID::BEAM])
    {
        limit_errors.append("Beam power orderd (B=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::BEAM]));
        limit_errors.append(") exceeds ship rating (B=");
        limit_errors.append(std::to_string(max_attack_attr[AttributeID::BEAM]));
        limit_errors.append(")\n");
        cls = false;
    }
    if (combat_attr[AttributeID::SCREEN] > max_attack_attr[AttributeID::SCREEN])
    {
        limit_errors.append("Screen power orderd (S=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::SCREEN]));
        limit_errors.append(") exceeds ship rating (S=");
        limit_errors.append(
            std::to_string(max_attack_attr[AttributeID::SCREEN]));
        limit_errors.append(")\n");
        cls = false;
    }
    if (combat_attr[AttributeID::TUBE] > max_attack_attr[AttributeID::TUBE])
    {
        limit_errors.append("Screen power orderd (T=");
        limit_errors.append(std::to_string(combat_attr[AttributeID::TUBE]));
        limit_errors.append(") exceeds ship rating (T=");
        limit_errors.append(std::to_string(max_attack_attr[AttributeID::TUBE]));
        limit_errors.append(")\n");
        cls = false;
    }

    int total_power_ordered = combat_attr[AttributeID::TUBE] +
                              combat_attr[AttributeID::SCREEN] +
                              combat_attr[AttributeID::BEAM];

    if (total_power_ordered > max_attack_attr[AttributeID::POWER_DRIVE])
    {
        limit_errors.append("Total power ordered (B+S+T = ");
        limit_errors.append(std::to_string(total_power_ordered));
        limit_errors.append(") exceeds ship rating (PD=");
        limit_errors.append(
            std::to_string(max_attack_attr[AttributeID::POWER_DRIVE]));
        limit_errors.append(")\n");
        cls = false;
    }

    if (!cls)
    {
        // we have errors in the ordered combat attributes
        // limit_errors has the errors.
        return false;
    }

    // We are ok to proceed with this new combat order.

    // 3. Insert/Update
    std::string combat_order =
        "INSERT INTO combat_orders "
        "(game_id, owner, ship_code, round, tactic, target_id, "
        "power_d, power_b, power_s, power_t, missiles_data, committed) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0) "
        "ON DUPLICATE KEY UPDATE "
        "tactic=?, target_id=?, power_d=?, power_b=?, power_s=?, power_t=?, "
        "missiles_data=?, committed=0";

    db.Exec(combat_order, {
        gid, combatting_players.first, attacker, combat_state.round,
        tactic, attackee,
        combat_attr[AttributeID::POWER_DRIVE], combat_attr[AttributeID::BEAM],
        combat_attr[AttributeID::SCREEN], combat_attr[AttributeID::TUBE],
        packed_missile_data,
        tactic, attackee,
        combat_attr[AttributeID::POWER_DRIVE], combat_attr[AttributeID::BEAM],
        combat_attr[AttributeID::SCREEN], combat_attr[AttributeID::TUBE],
        packed_missile_data
    });

    // Resolution triggered by explicit 'combat commit'
    // BUGBUG
    // return "Order draft saved. Use 'combat commit' when ready.";
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
    std::string result = ce.apply_damage(owner, target_ship , assignments);

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
    auto gameRow = db.Query("SELECT active_combat_hex FROM games WHERE id=?", {gid});

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
        Telemetry::instance().write("TACTICAL: No combat orders queued.");
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
                    "Error: Orders pending for hex " + row[0] +
                    " but active combat is in hex " + activeHex);
                return false;
            }
        }
    }

    // Mark all uncommitted orders as committed
    db.Exec("UPDATE combat_orders SET committed=1 WHERE game_id=? "
            "AND owner=? AND committed=0", {gid, owner});

    Telemetry::instance().write("TACTICAL: Combat orders transmitted.");

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
                       << " " << ord[3] << " [D=" << ord[4] << " B=" << ord[5]
                       << " S=" << ord[6] << " T=" << ord[7] << "]\n";
            }
            reveal << "==============================";
            Telemetry::instance().add_broadcast(gid, reveal.str());

            std::string result = ce.resolve_round(hex_id);
            Telemetry::instance().write(result);
        }
        else
        {
            // Notify opponent that this player has committed
            char opponent = (owner == 'A') ? 'B' : 'A';
            Telemetry::instance().add_tell(
                opponent, "Player " + std::string(1, owner) +
                              " has committed combat orders for hex " + hex_id +
                              ". Use 'combat order' then 'combat commit' for "
                              "your ships.");

            Telemetry::instance().write("TACTICAL: Sector " + hex_id +
                                           " - Awaiting enemy orders.");
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
    auto orderRows = db.Query(
        "SELECT COUNT(*) FROM combat_orders WHERE game_id=? "
        "AND owner=? AND committed=0", {gid, owner});

    if (orderRows.empty() || orderRows[0][0] == "0")
    {
        Telemetry::instance().write(
            "TACTICAL: No combat orders have been received.");
        return true;
    }

    // Delete all uncommitted orders for this player
    db.Exec("DELETE FROM combat_orders WHERE game_id=? AND owner=? "
            "AND committed=0", {gid, owner});

    Telemetry::instance().write(
        "TACTICAL: Orders rescinded. Issue new commands.");
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
        "co.power_d, co.power_b, co.power_s, co.power_t, co.missiles_data "
        "FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND "
        "s.ship_code=co.ship_code "
        "WHERE co.game_id=? AND co.owner=? AND co.committed=0 "
        "AND s.destroyed_at IS NULL ORDER BY s.at_hex, co.ship_code";
    auto rows = db.Query(drafts_q, {gid, owner});

    if (rows.empty())
    {
        Telemetry::instance().write("No pending combat orders.");
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
        std::string tacticName = (tactic == 'A')   ? "Attack"
                                 : (tactic == 'D') ? "Dodge"
                                                   : "Escape";
        out << "    " << r[1] << ": " << tacticName << " -> "
            << (r[3].empty() ? "(none)" : r[3]) << " [D=" << r[4]
            << " B=" << r[5] << " S=" << r[6] << " T=" << r[7];
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
