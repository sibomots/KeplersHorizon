#include "combatagent.h"
#include "ce.h"
#include "db.h"
#include "telemetry.h"
#include "logger.h"
#include <sstream>

CombatSessionState
CombatAgent::get_combat_state_at_hex(const int gid, const std::string& hex_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();

    // BUGBUG need to be more precise about the combat_state record that is
    // found. module id and players involved?
    auto rows =
        db.query("SELECT round, stage, attacker_remains, stalemate_counter, "
                 "pending_damage_json, last_log "
                 "FROM combat_state WHERE game_id=" +
                 std::to_string(gid) + " AND hex_id='" + hex_id + "'");
    if (rows.empty())
    {
        // {0, "", 0, 0, false, 0, "", ""};
        return CombatSessionState();
    }

    return CombatSessionState(gid, hex_id,
                              // rows[0][0] == round
                              std::atoi(rows[0][0].c_str()),
                              // rows[0][1] == stage
                              std::atoi(rows[0][1].c_str()),
                              // rows[0][2] == attacker_remains (bool)
                              (rows[0][2] == "1"),
                              // rows[0][3] == stalemate_counter
                              std::atoi(rows[0][3].c_str()),
                              // rows[0][4] == pending_damage_gson
                              rows[0][4],
                              // rows[0][5] == pending_damage
                              rows[0][5]);
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
    DatabaseManager& db = DatabaseManager::getInstance();

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

    std::string pick_attacker;
    pick_attacker.append("SELECT S.owner FROM ships S WHERE S.game_id =");
    pick_attacker.append(std::to_string(gid));
    pick_attacker.append(" AND S.destroyed_at is NULL ");
    pick_attacker.append(" AND S.owner='");
    pick_attacker += combatting_players.first;
    pick_attacker.append("' ");
    pick_attacker.append(" AND ( S.ship_code='");
    pick_attacker.append(db.esc(attacker));
    pick_attacker.append("' OR S.ship_name ='");
    pick_attacker.append(db.esc(attacker));
    pick_attacker.append("')");

    auto attacker_owner = db.query(pick_attacker);

    if (attacker_owner.empty())
    {
        // BUGBUG
        // ship is not found
        return false;
    }
    std::string pick_attackee;
    pick_attackee.append("SELECT S.owner FROM ships S WHERE S.game_id =");
    pick_attackee.append(std::to_string(gid));
    pick_attackee.append(" AND S.destroyed_at is NULL ");
    pick_attackee.append(" AND S.owner='");
    pick_attackee += combatting_players.second;
    pick_attackee.append("' ");
    pick_attackee.append(" AND ( S.ship_code='");
    pick_attackee.append(db.esc(attackee));
    pick_attackee.append("' OR S.ship_name ='");
    pick_attackee.append(db.esc(attackee));
    pick_attackee.append("')");

    auto attackee_owner = db.query(pick_attackee);

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
    std::string query_stats_attacker;
    query_stats_attacker.append(
        "SELECT at_hex, pd, beam, screen, tube, missiles, sr ");
    query_stats_attacker.append(" FROM ships WHERE game_id=");
    query_stats_attacker.append(std::to_string(gid));
    query_stats_attacker.append(" AND destroyed_at is NULL ");
    query_stats_attacker.append(" AND ( ship_code='");
    query_stats_attacker.append(db.esc(attacker));
    query_stats_attacker.append("' OR ship_name='");
    query_stats_attacker.append(db.esc(attacker));
    query_stats_attacker.append("')");
    auto attacker_stats = db.query(query_stats_attacker.c_str());

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
    std::string combat_order;

    combat_order.append("INSERT INTO combat_orders "
                        " (game_id, "
                        " owner, "
                        " ship_code, "
                        " round, "
                        " tactic, "
                        " target_id, "
                        " power_d, "
                        " power_b, "
                        " power_s, "
                        " power_t, "
                        " missiles_data, "
                        " committed) ");
    combat_order.append(" VALUES (");
    combat_order.append(std::to_string(gid));
    combat_order.append(", '");
    combat_order += combatting_players.first;
    combat_order.append("', '");
    combat_order.append(attacker);
    combat_order.append("', ");
    combat_order.append(std::to_string(combat_state.round));
    combat_order.append(", '");
    combat_order += tactic;
    combat_order.append("', '");
    combat_order.append(attackee);
    combat_order.append("', ");
    combat_order.append(std::to_string(combat_attr[AttributeID::POWER_DRIVE]));
    combat_order.append(", ");
    combat_order.append(std::to_string(combat_attr[AttributeID::BEAM]));
    combat_order.append(", ");
    combat_order.append(std::to_string(combat_attr[AttributeID::SCREEN]));
    combat_order.append(", ");
    combat_order.append(std::to_string(combat_attr[AttributeID::TUBE]));
    combat_order.append(", '");
    combat_order.append(packed_missile_data);
    combat_order.append("', 0) ");
    combat_order.append("ON DUPLICATE KEY UPDATE ");
    combat_order.append("tactic='");
    combat_order += tactic;
    combat_order.append("', ");
    combat_order.append("target_id='");
    combat_order.append(attackee);
    combat_order.append("', ");
    combat_order.append("power_d=");
    combat_order.append(std::to_string(combat_attr[AttributeID::POWER_DRIVE]));
    combat_order.append(", ");
    combat_order.append("power_b=");
    combat_order.append(std::to_string(combat_attr[AttributeID::BEAM]));
    combat_order.append(", ");
    combat_order.append("power_s=");
    combat_order.append(std::to_string(combat_attr[AttributeID::SCREEN]));
    combat_order.append(", ");
    combat_order.append("power_t=");
    combat_order.append(std::to_string(combat_attr[AttributeID::TUBE]));
    combat_order.append(", ");
    combat_order.append("missiles_data='");
    combat_order.append(packed_missile_data);
    combat_order.append("', committed=0");

    db.exec(combat_order.c_str());

    // Resolution triggered by explicit 'combat commit'
    // BUGBUG
    // return "Order draft saved. Use 'combat commit' when ready.";
    return true;
}

bool CombatAgent::apply(CombatApplyParam& param)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    
    int gid = param.get_game_id();
    char owner = param.get_player();
    std::string target_ship = param.get_target_ship();
    AttributeMap assignments = param.get_assignments();
    
    // Apply damage via combat engine
    CombatEngine ce(gid);
    std::string result = ce.apply_damage(owner, target_ship , assignments);

    Telemetry::getInstance().write(result);
    return true;
}

bool CombatAgent::apply(CombatRetreatParam& param)
{
    return false;
}

bool CombatAgent::apply(CombatCommitParam& param)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    
    int gid = param.get_game_id();
    char owner = param.get_player();

    // Get the active combat hex
    auto gameRow = db.query("SELECT active_combat_hex FROM games WHERE id=" +
                            std::to_string(gid));

    std::string activeHex;
    if (!gameRow.empty() && !gameRow[0][0].empty())
    {
        activeHex = gameRow[0][0];
    }

    // Get all hexes with uncommitted orders for this player
    auto hexRows = db.query("SELECT DISTINCT s.at_hex FROM combat_orders co "
                            "JOIN ships s ON s.game_id=co.game_id AND "
                            "s.owner=co.owner AND s.ship_code=co.ship_code "
                            "WHERE co.game_id=" +
                            std::to_string(gid) + " AND co.owner='" +
                            std::string(1, owner) +
                            "' AND co.committed=0 AND s.destroyed_at IS NULL");

    if (hexRows.empty())
    {
        Telemetry::getInstance().write("TACTICAL: No combat orders queued.");
        return true;
    }

    // Validate orders are for active hex (if one is set)
    if (!activeHex.empty())
    {
        for (const auto& row : hexRows)
        {
            if (row[0] != activeHex)
            {
                Telemetry::getInstance().write(
                    "Error: Orders pending for hex " + row[0] +
                    " but active combat is in hex " + activeHex);
                return false;
            }
        }
    }

    // Mark all uncommitted orders as committed
    db.exec("UPDATE combat_orders SET committed=1 WHERE game_id=" +
            std::to_string(gid) + " AND owner='" + std::string(1, owner) +
            "' AND committed=0");

    Telemetry::getInstance().write("TACTICAL: Combat orders transmitted.");

    // Check each affected hex for resolution
    CombatEngine ce(gid);
    for (const auto& row : hexRows)
    {
        std::string hex_id = row[0];
        auto cs = ce.get_combat_state(hex_id);

        if (ce.all_orders_committed(hex_id, cs.round))
        {
            // Reveal all orders to both players before resolution (per rules)
            auto orders = db.query(
                "SELECT co.owner, co.ship_code, co.tactic, co.target_id, "
                "co.power_d, co.power_b, co.power_s, co.power_t "
                "FROM combat_orders co "
                "JOIN ships s ON s.game_id=co.game_id AND "
                "s.ship_code=co.ship_code AND s.owner=co.owner "
                "WHERE co.game_id=" +
                std::to_string(gid) + " AND s.at_hex='" + hex_id +
                "' AND co.round=" + std::to_string(cs.round) +
                " AND s.destroyed_at IS NULL ORDER BY co.owner, co.ship_code");

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
            Telemetry::getInstance().add_broadcast(gid, reveal.str());

            std::string result = ce.resolve_round(hex_id);
            Telemetry::getInstance().write(result);
        }
        else
        {
            // Notify opponent that this player has committed
            char opponent = (owner == 'A') ? 'B' : 'A';
            Telemetry::getInstance().add_tell(
                opponent, "Player " + std::string(1, owner) +
                              " has committed combat orders for hex " + hex_id +
                              ". Use 'combat order' then 'combat commit' for "
                              "your ships.");

            Telemetry::getInstance().write("TACTICAL: Sector " + hex_id +
                                           " - Awaiting enemy orders.");
        }
    }
    return true;
}

bool CombatAgent::apply(CombatCancelParam& param)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    
    int gid = param.get_game_id();
    char owner = param.get_player();

    // Check if there are any uncommitted orders to cancel (Bug #1)
    auto orderRows =
        db.query("SELECT COUNT(*) FROM combat_orders WHERE game_id=" +
                 std::to_string(gid) + " AND owner='" +
                 std::string(1, owner) + "' AND committed=0");

    if (orderRows.empty() || orderRows[0][0] == "0")
    {
        Telemetry::getInstance().write(
            "TACTICAL: No combat orders have been received.");
        return true;
    }

    // Delete all uncommitted orders for this player
    db.exec(
        "DELETE FROM combat_orders WHERE game_id=" + std::to_string(gid) +
        " AND owner='" + std::string(1, owner) + "' AND committed=0");

    Telemetry::getInstance().write(
        "TACTICAL: Orders rescinded. Issue new commands.");
    return true;
}

bool CombatAgent::apply(CombatDraftsParam& param)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    
    int gid = param.get_game_id();
    char owner = param.get_player();

    // Query uncommitted orders for this player, grouped by hex
    auto rows = db.query(
        "SELECT s.at_hex, co.ship_code, co.tactic, co.target_id, "
        "co.power_d, co.power_b, co.power_s, co.power_t, co.missiles_data "
        "FROM combat_orders co "
        "JOIN ships s ON s.game_id=co.game_id AND s.owner=co.owner AND "
        "s.ship_code=co.ship_code "
        "WHERE co.game_id=" +
        std::to_string(gid) + " AND co.owner='" + std::string(1, owner) +
        "' AND co.committed=0 AND s.destroyed_at IS NULL "
        "ORDER BY s.at_hex, co.ship_code");

    if (rows.empty())
    {
        Telemetry::getInstance().write("No pending combat orders.");
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
    Telemetry::getInstance().write(out.str());
    return true;
}

bool CombatAgent::apply(CombatStatusParam& param)
{
    return false;
}
