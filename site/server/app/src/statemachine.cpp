//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>

#include "aiagent.h"
#include "ce.h"
#include "db.h"
#include "logger.h"
#include "moduleutil.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"
#include "turn_end.h"

bool StateMachine::is_ai_player(const std::string& player) const
{
    if (!data.is_singleplayer_mode)
        return false;
    if (player.empty())
        return false;
    return (player[0] == data.ai_player_side);
}

void StateMachine::set_game_mode(bool singleplayer, char ai_player)
{
    data.is_singleplayer_mode = singleplayer;
    data.ai_player_side = singleplayer ? ai_player : '\0';

    if (singleplayer)
    {
        Logger::instance().info(
            "[SM] Single-player mode enabled, AI is Player " +
            std::string(1, ai_player));
    }
    else
    {
        Logger::instance().info("[SM] Two-player mode enabled");
    }
}

// Methods doing work do NOT move state.
bool StateMachine::preinitialize()
{
    return true;
}

bool StateMachine::initialize()
{
    if (data.state == ServerState::INVALID ||
        data.state == ServerState::PREINITIALIZE)
    {
        data.state = ServerState::READY_GAME_START;
    }
    return true;
}

bool StateMachine::start_game_for_random_player()
{
    // TBD logic
    return true;
}

GameState StateMachine::load_game(int game_id)
{
    DatabaseManager& db = DatabaseManager::instance();
    auto rows = db.query("SELECT module_id,state_json FROM games WHERE id=" +
                         std::to_string(game_id) + " LIMIT 1");
    if (rows.empty())
    {
        throw std::runtime_error("game not found");
    }
    int module_id = std::atoi(rows[0][0].c_str());
    std::string state_json = rows[0][1];

    // load state from state_json
    GameState s = GameState::from_json_min(state_json);
    s.game_id = game_id;

    // module_id used by queries for universe data
    (void)module_id;

    // Quick re-sync by reading a few known fields with string search (simple).
    auto find_int = [&](const std::string& key, int fallback) -> int
    {
        std::string pat = "\"" + key + "\":";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
        {
            return fallback;
        }
        p += pat.size();
        while (p < state_json.size() &&
               std::isspace((unsigned char)state_json[p]))
            p++;
        int sign = 1;
        if (p < state_json.size() && state_json[p] == '-')
        {
            sign = -1;
            p++;
        }
        long v = 0;
        while (p < state_json.size() &&
               std::isdigit((unsigned char)state_json[p]))
        {
            v = v * 10 + (state_json[p] - '0');
            p++;
        }
        return (int)(v * sign);
    };
    auto find_str = [&](const std::string& key,
                        const std::string& fallback) -> std::string
    {
        std::string pat = "\"" + key + "\":\"";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
        {
            return fallback;
        }
        else
        {
            p += pat.size();
            size_t e = state_json.find("\"", p);
            if (e == std::string::npos)
            {
                return fallback;
            }
            return state_json.substr(p, e - p);
        }
    };

    s.round = std::max(1, find_int("round", s.round));
    s.active_player = find_str("activePlayer", s.active_player);
    s.phase_index = find_int("phaseIndex", s.phase_index);

    // Parse bp/vp objects
    auto find_obj_int = [&](const std::string& objKey,
                            const std::string& fieldKey, int fallback) -> int
    {
        std::string op = "\"" + objKey + "\":{";
        size_t p = state_json.find(op);
        if (p == std::string::npos)
        {
            return fallback;
        }
        size_t end = state_json.find("}", p + op.size());
        if (end == std::string::npos)
        {
            return fallback;
        }
        std::string sub = state_json.substr(p, end - p + 1);
        // re-use global find_int (ok for now; small JSON)
        return find_int(fieldKey, fallback);
    };

    // We'll do simple direct searches for "vp":{"A":X,"B":Y} etc.
    size_t vpPos = state_json.find("\"vp\":{");
    if (vpPos != std::string::npos)
    {
        size_t end = state_json.find("}", vpPos);
        std::string vpSub = state_json.substr(vpPos, end - vpPos + 1);
        auto getv = [&](const std::string& k) -> int
        {
            std::string pat = "\"" + k + "\":";
            size_t p = vpSub.find(pat);
            if (p == std::string::npos)
            {
                return 0;
            }
            p += pat.size();
            while (p < vpSub.size() && std::isspace((unsigned char)vpSub[p]))
                p++;
            long v = 0;
            while (p < vpSub.size() && std::isdigit((unsigned char)vpSub[p]))
            {
                v = v * 10 + (vpSub[p] - '0');
                p++;
            }
            return (int)v;
        };
        s.vpA = getv("A");
        s.vpB = getv("B");
    }
    size_t bpPos = state_json.find("\"bp\":{");
    if (bpPos != std::string::npos)
    {
        size_t end = state_json.find("}", bpPos);
        std::string bpSub = state_json.substr(bpPos, end - bpPos + 1);
        auto getv = [&](const std::string& k) -> int
        {
            std::string pat = "\"" + k + "\":";
            size_t p = bpSub.find(pat);
            if (p == std::string::npos)
            {
                return 0;
            }
            p += pat.size();
            while (p < bpSub.size() && std::isspace((unsigned char)bpSub[p]))
                p++;
            long v = 0;
            while (p < bpSub.size() && std::isdigit((unsigned char)bpSub[p]))
            {
                v = v * 10 + (bpSub[p] - '0');
                p++;
            }
            return (int)v;
        };
        s.creditsA = getv("A");
        s.creditsB = getv("B");
    }

    // Load Combat Summary
    CombatEngine ce(game_id);
    auto combats = ce.get_active_combats();
    if (!combats.empty())
    {
        std::ostringstream c;
        c << "{"
             "\"active_hexes\":[";
        for (size_t i = 0; i < combats.size(); ++i)
        {
            if (i > 0)
            {
                c << ",";
            }
            c << "\"" << combats[i].hex_id << "\"";
        }
        c << "],"
             "\"combats\":[";
        for (size_t i = 0; i < combats.size(); ++i)
        {
            if (i > 0)
            {
                c << ",";
            }
            c << "{\"hex\":\"" << combats[i].hex_id << "\",";
            c << "\"log\":\"" << json_escape(combats[i].last_log) << "\",";
            c << "\"stage\":" << combats[i].stage; // useful for UI
            c << "}";
        }
        c << "],"
             "\"count\":"
          << combats.size();
        c << "}";
        s.combat_summary_json = c.str();
    }
    return s;
}

GameState StateMachine::new_game_state()
{
    GameState s;
    s.round = 1;
    s.active_player = "A";
    s.phase_index = PH_BUILD_SHIPS;
    s.vpA = 0;
    s.vpB = 0;

    s.creditsA = 40;
    s.creditsB = 40;

    return s;
}

void StateMachine::apply_start_of_turn(GameState& s)
{
    DatabaseManager& db = DatabaseManager::instance();
    // Called when a player begins their player-turn (phase 0 = Build Ships).
    // 1) Count victory points automatically.
    // 2) Award BP (+10) at start of each player-turn after the first.

    if (s.game_over)
    {
        // BUGBUG not sure what happens.
        return;
    }

    // VP: +1 for each enemy base system occupied at start of your turn.
    char me = s.active_player.empty() ? 'A' : s.active_player[0];
    char enemy = (me == 'A') ? 'B' : 'A';

    int vp_gain = 0;
    {
        // star_systems uses module_id from game, ships use game_id
        int mod = get_module_id_for_game(s.game_id);
        std::string q =
            "SELECT COUNT(DISTINCT ss.name) "
            "FROM ships sh JOIN star_systems ss ON sh.at_system = ss.name AND "
            "ss.module_id=" +
            std::to_string(mod) +
            " WHERE sh.game_id=" + std::to_string(s.game_id) +
            " AND sh.owner='" + std::string(1, me) +
            "' AND sh.racked_in IS NULL AND sh.destroyed_at IS NULL "
            " AND ss.is_base=1 AND ss.base_owner='" +
            std::string(1, enemy) + "'";
        auto r = db.query(q);
        if (!r.empty() && !r[0].empty())
        {
            vp_gain = std::atoi(r[0][0].c_str());
        }
    }

    if (vp_gain > 0)
    {
        if (me == 'A')
        {
            s.vpA += vp_gain;
        }
        else
        {
            s.vpB += vp_gain;
        }
    }

    // VP needed to win (advanced mode: 3 VP)
    int need = 3;

    int my_vp = (me == 'A') ? s.vpA : s.vpB;
    if (my_vp >= need)
    {
        s.game_over = true;
        s.winner = std::string(1, me);
        return;
    }

    // Advanced mode BP cadence: +10 BP at start of each player-turn after first
    bool is_first_player_first_turn = (s.round == 1 && me == 'A');
    if (!is_first_player_first_turn)
    {
        if (me == 'A')
        {
            s.creditsA += 200; // 10 × 20 (inflated)
        }
        else
        {
            s.creditsB += 200; // 10 × 20 (inflated)
        }
    }

    // Reset movement points for active player's ships
    db.exec("UPDATE ships SET pd_spent=0 WHERE game_id=" +
            std::to_string(s.game_id) + " AND owner='" + std::string(1, me) +
            "'");
}

void StateMachine::advance_next(GameState& s)
{
    if (s.game_over)
    {
        return;
    }

    if (s.phase_index < PH_END_TURN)
    {
        // --- Prevent skipping active combat ---
        if (s.phase_index == PH_RESOLVE_COMBAT)
        {
            CombatEngine ce(s.game_id);
            if (!ce.get_active_combats().empty())
            {
                // Cannot advance until all combats resolved
                Logger::instance().info("[SM] Combat is still active.");
                return;
            }
        }

        s.phase_index++;

        // --- Combat Trigger Logic ---
        if (s.phase_index == PH_RESOLVE_COMBAT)
        {
            CombatEngine ce(s.game_id);
            ce.check_for_combat_triggers();
            auto combats = ce.get_active_combats();

            Logger::instance().info(
                "[SM][advance_next] Combat phase for game " +
                std::to_string(s.game_id) +
                ", combats found: " + std::to_string(combats.size()));

            if (combats.empty())
            {
                // No combat? Auto-skip to next phase
                Logger::instance().info("[SM][advance_next] No combat, "
                                        "skipping to Pick/Drop phase");
                s.phase_index = PH_SYSTEM_PICKDROP;
            }
            else
            {
                Logger::instance().info("[SM][advance_next] Combat detected! "
                                        "Pausing at Combat phase");

                // Notify BOTH players about combat detection with detailed
                // listing
                DatabaseManager& db = DatabaseManager::instance();
                char meOwner =
                    (s.active_player.empty() ? 'A' : s.active_player[0]);
                char oppOwner = (meOwner == 'A') ? 'B' : 'A';

                for (const auto& combat : combats)
                {
                    // Look up system name from hex
                    std::string sysName = combat.hex_id;
                    int mod = get_module_id_for_game(s.game_id);
                    auto sysRow =
                        db.query("SELECT name FROM star_systems "
                                 "WHERE module_id=" +
                                 std::to_string(mod) + " AND hex_id='" +
                                 combat.hex_id + "' LIMIT 1");
                    if (!sysRow.empty())
                    {
                        sysName = sysRow[0][0];
                    }

                    // Query ships in this hex
                    auto shipRows = db.query(
                        "SELECT ship_code, ship_name, ship_type, owner, pd, "
                        "beam, screen, tube, missiles "
                        "FROM ships WHERE game_id=" +
                        std::to_string(s.game_id) + " AND at_hex='" +
                        combat.hex_id +
                        "' AND destroyed_at IS NULL ORDER BY owner, ship_code");

                    // Generate message for each player (A and B) with their
                    // perspective
                    for (char viewer : {'A', 'B'})
                    {
                        char enemyOwner = (viewer == 'A') ? 'B' : 'A';

                        std::ostringstream combatMsg;
                        combatMsg << "   CONFLICT IN STAR SYSTEM: " << sysName
                                  << " [" << combat.hex_id << "]\n";
                        combatMsg << "     SHIPS IN SYSTEM " << sysName << "\n";

                        // Blue-Force (viewer's ships) - show stats
                        combatMsg << "         Blue-Force\n";
                        int blueNum = 1;
                        for (const auto& ship : shipRows)
                        {
                            if (ship[3][0] == viewer)
                            {
                                std::string shipClass = (ship[2] == "W")
                                                            ? "WarpShip"
                                                            : "SystemShip";
                                combatMsg
                                    << "             " << blueNum++ << ". "
                                    << shipClass << " class " << ship[0] << " "
                                    << ship[1] << " (PD:" << ship[4]
                                    << " B:" << ship[5] << " S:" << ship[6]
                                    << " T:" << ship[7] << " M:" << ship[8]
                                    << ")\n";
                            }
                        }

                        // Red-Force (enemy ships) - NO stats (private info)
                        combatMsg << "         Red-Force\n";
                        int redNum = 1;
                        for (const auto& ship : shipRows)
                        {
                            if (ship[3][0] == enemyOwner)
                            {
                                std::string shipClass = (ship[2] == "W")
                                                            ? "WarpShip"
                                                            : "SystemShip";
                                combatMsg << "             " << redNum++ << ". "
                                          << shipClass << " class " << ship[0]
                                          << " " << ship[1] << "\n";
                            }
                        }

                        combatMsg << "     >> Draft orders:   'combat order'\n";
                        combatMsg << "     >> Execute orders: 'combat commit'";

                        Telemetry::instance().add_tell(s.game_id, viewer,
                                                       combatMsg.str());
                    }
                }
            }
        }
        return;
    }

    if (s.active_player == "A")
    {
        s.active_player = "B";
    }
    else
    {
        s.active_player = "A";
        s.round++;

        // Trigger round-end processing (market, facilities, resources, income)
        TurnEndProcessor::on_round_complete(s.game_id, s.round - 1);
    }
    s.phase_index = PH_BUILD_SHIPS;
    apply_start_of_turn(s);

    save_game(s);

    // NEW: Trigger AI if it has initiative after turn/phase change
    if (is_ai_player(s.active_player))
    {
        Logger::instance().info(
            "[SM] AI Player " + s.active_player +
            " has initiative, turn=" + std::to_string(s.round));

        // Phase 2: Uncomment below to trigger AI turn
        AIAgent::instance().take_turn(s.game_id, s.active_player[0]);
    }
}

void StateMachine::save_game(const GameState& s)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "UPDATE games SET state_json='" + db.esc(s.to_json()) +
                    "' WHERE id=" + std::to_string(s.game_id);
    db.exec(q);
}

int StateMachine::next_event_seq(int game_id)
{
    DatabaseManager& db = DatabaseManager::instance();
    auto r = db.query(
        "SELECT COALESCE(MAX(seq),0)+1 FROM game_events WHERE game_id=" +
        std::to_string(game_id));
    if (r.empty())
        return 1;
    return std::atoi(r[0][0].c_str());
}

std::string StateMachine::get_player_name(int game_id, const std::string& seat)
{
    DatabaseManager& db = DatabaseManager::instance();
    auto rows = db.query("SELECT u.username FROM game_seats gs "
                         "JOIN users u ON gs.user_id = u.id "
                         "WHERE gs.game_id=" +
                         std::to_string(game_id) + " AND gs.seat='" +
                         db.esc(seat) + "'");
    return rows.empty() ? seat : rows[0][0];
}

//------------------------------------------------------------------------------
// Command Inhibit System - centralized validation for all commands
// Check priority: 1) Initiative, 2) Phase, 3) Intra-phase state
//------------------------------------------------------------------------------

bool StateMachine::check_inhibits(CommandID cmd, std::string& error_msg)
{
    GameState s = get_game_state();
    char requesting_player = data.current_player;
    bool has_initiative = (s.active_player[0] == requesting_player);

    switch (cmd)
    {
    //--------------------------------------------------------------------------
    // BUILD commands - only during Build Ships phase, only with initiative
    //--------------------------------------------------------------------------
    case CommandID::BUILD_NEW:
    {
        // 1. Check initiative first
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        // 2. Check phase
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg = "Building only allowed during Build Ships phase";
            return false;
        }
        return true;
    }
    break;

    case CommandID::BUILD_SET:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg =
                "Build modifications only allowed during Build Ships phase";
            return false;
        }
        return true;
    }
    break;

    case CommandID::BUILD_COMMIT:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg =
                "Committing ships only allowed during Build Ships phase";
            return false;
        }
        return true;
    }
    break;

    //--------------------------------------------------------------------------
    // DEPLOY command - only during Build Ships phase, only with initiative
    //--------------------------------------------------------------------------
    case CommandID::DEPLOY:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg = "Deployment only allowed during Build Ships phase";
            return false;
        }
        return true;
    }
    break;

    //--------------------------------------------------------------------------
    // MOVE command - only during Movement phase, only with initiative
    //--------------------------------------------------------------------------
    case CommandID::MOVE:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        if (s.phase_index != PH_MOVEMENT)
        {
            error_msg = "Movement only allowed during Movement phase";
            return false;
        }
        return true;
    }
    break;

    //--------------------------------------------------------------------------
    // NEXT/DONE - phase advancement, only with initiative
    //--------------------------------------------------------------------------
    case CommandID::NEXT:
    case CommandID::DONE:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (waiting for " +
                        get_player_name(s.game_id, s.active_player) + ")";
            return false;
        }
        return true;
    }
    break;

    //--------------------------------------------------------------------------
    // STATUS/HELP - always allowed, no restrictions
    //--------------------------------------------------------------------------
    case CommandID::STATUS:
    case CommandID::HELP:
    {
        return true;
    }
    break;

    //--------------------------------------------------------------------------
    // COMBAT commands - during Combat phase
    // Note: Combat has special rules - BOTH players issue orders simultaneously
    //--------------------------------------------------------------------------
    case CommandID::COMBAT_ORDER:
    {
        // Both players can issue combat orders during combat phase
        // (no initiative check - simultaneous orders)
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg = "Combat orders only allowed during Combat phase";
            return false;
        }
        return true;
    }
    break;

    case CommandID::COMBAT_APPLY:
    {
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg = "Combat apply damage only allowed during Combat phase "
                        "after damage has been assessed. TBD";
            return false;
        }
        return true;
    }
    break;

    case CommandID::COMBAT_DRAFTS:
    {
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg =
                "Combat drafts is allowed when making combat orders only.";
            return false;
        }
        return true;
    }
    break;

    case CommandID::COMBAT_CANCEL:
    {
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg =
                "Combat cancel is allowed when making combat orders and "
                "there are pending combat orders to cancel.";
            return false;
        }
        return true;
    }
    break;

    case CommandID::COMBAT_COMMIT:
    {
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg =
                "Combat commit is allowed when making combat orders and "
                "there are pending combat orders to commit.";
            return false;
        }
        return true;
    }
    break;

    case CommandID::COMBAT_FIRE:
    {
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg = "Combat fire only allowed during Combat phase";
            return false;
        }
        // Note: Order commitment enforced by
        // CombatEngine::all_orders_committed()
        return true;
    }
    break;

    } // end switch

    // Default: allow (for any commands not explicitly handled)
    return true;
}
