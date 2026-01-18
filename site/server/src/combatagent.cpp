#include "combatagent.h"
#include "db.h"

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
    query_stats_attacker.append("'");
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
    return false;
}
bool CombatAgent::apply(CombatRetreatParam& param)
{
    return false;
}
bool CombatAgent::apply(CombatCommitParam& param)
{
    return false;
}
bool CombatAgent::apply(CombatCancelParam& param)
{
    return false;
}
bool CombatAgent::apply(CombatDraftsParam& param)
{
    return false;
}
bool CombatAgent::apply(CombatStatusParam& param)
{
    return false;
}
