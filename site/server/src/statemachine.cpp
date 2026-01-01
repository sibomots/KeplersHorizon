//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "statemachine.h"

#include <iostream>

#include "combat.h"
#include "db.h"
#include "ships.h"
#include "logger.h"
#include "telemetry.h"
#include "util.h"

// Methods doing work do NOT move state.
bool StateMachine::preinitialize()
{
    Logger::instance().info("StateMachine::preinitialize");
    // Work only
    return true;
}

bool StateMachine::initialize()
{
    Logger::instance().info("StateMachine::initialize");
    // Work only.
    // Assuming startup logic sets initial state via a different mechanism or
    // we explicitly set "Ready" if initialization is confirmed successful by
    // the caller.

    // For now, to satisfy the flow, we will assume we are ready after init.
    // But per user instruction, we don't do transition logic here if it's
    // "handling a state". We'll set the initial valid state for the game loop.
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

// Core State Machine Transition Logic
// The "Big Case Statement" that manages lifecycle and executes work based on
// properties set by Agents/Commands.
bool StateMachine::transition()
{
    Logger::instance().info("StateMachine::transition current_state=" +
                            std::to_string(data.state));

    DatabaseManager& db = DatabaseManager::getInstance();

    switch (data.state)
    {

    case ServerState::READY_GAME_START:
        // Check if we have a pending scenario from StartCommand
        // The State Slate has been updated by the Agent (StartCommand) with the
        // intent.
        if (data.scenario != ScenarioType::UNDEFINED)
        {
            // Determine Scenario String
            std::string sc_str = "";
            switch (data.scenario)
            {
            case ScenarioType::LEARNING:
                sc_str = "learning";
                break;
            case ScenarioType::BASIC:
                sc_str = "basic";
                break;
            case ScenarioType::ADVANCED:
                sc_str = "advanced";
                break;
            default:
                break;
            }

            Logger::instance().info("Transition: initializing game scenario: " +
                                    sc_str);

            // Logic to Establish the Game (DB operations)
            // At this point, we rely on the DB connection being valid
            // (invariant).
            GameState s = new_game_state_for_scenario(sc_str);
            // BUGBUG this is wrong.
            // The game ID should be made in 'new_game_state_for...'
            // s.game_id = data.game_id;

            db.exec("DELETE FROM drafts WHERE game_id=" +
                    std::to_string(data.game_id));
            db.exec("DELETE FROM ships  WHERE game_id=" +
                    std::to_string(data.game_id));
            set_current_draft(data.game_id, 'A', "");
            set_current_draft(data.game_id, 'B', "");
            save_game(s);

            Logger::instance().info(
                "Transition: Game Initialized. Moving to IN_GAME.");
            data.state = ServerState::IN_GAME;

            // Clear intent? Or keep it as part of state?
            // User said "We set it. We test it. What's possibly going to change
            // it?" So maybe we don't clear it. It IS the scenario of the game.

            // Game is now active - turn phases tracked via PhaseIndex in GameState
            return true;
        }
        break;

    case ServerState::IN_GAME:
#if 0
            // Legacy BUILD_PHASE logic - replaced by self-contained Commands
            // Commands now execute their own logic directly
            // This code is kept for reference but should not execute
            
            // Build/Repair/Resupply logic
            
            // Handle build list drafts
            if (data.pending_build_list_drafts) {
                GameState s = load_game(data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::vector<DraftRow> drafts = load_drafts(data.game_id, active_player);
                
                if (drafts.empty()) {
                    Logger::instance().info("No drafts found");
                } else {
                    std::ostringstream msg;
                    msg << "Drafts (" << drafts.size() << "):";
                    for (const auto& d : drafts) {
                        msg << "\n  " << d.code << " - " << d.name;
                    }
                    Logger::instance().info(msg.str());
                }
                data.pending_build_list_drafts = false;
                return true;
            }
            
            // Handle build show draft
            if (!data.pending_build_show_draft.empty()) {
                GameState s = load_game(data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string ship_code = data.pending_build_show_draft;
                
                if (!draft_exists(data.game_id, active_player, ship_code)) {
                    Logger::instance().error("Draft not found: " + ship_code);
                } else {
                    DraftRow d = load_draft(data.game_id, active_player, ship_code);
                    std::ostringstream msg;
                    msg << "Draft: " << d.name << " - " << d.code << "\n"
                        << "  Type: " << d.attr.type << "\n"
                        << "  PD=" << d.attr.PD << ", B=" << d.attr.B << ", S=" << d.attr.S
                        << ", T=" << d.attr.T << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
                    Logger::instance().info(msg.str());
                    
                    // Set as current draft
                    set_current_draft(data.game_id, active_player, ship_code);
                }
                data.pending_build_show_draft.clear();
                return true;
            }
            
            // Handle build cancel
            if (data.pending_build_cancel) {
                GameState s = load_game(data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to cancel");
                } else {
                    delete_draft(data.game_id, active_player, draft_code);
                    set_current_draft(data.game_id, active_player, "");
                    Logger::instance().info("Canceled draft: " + draft_code);
                }
                data.pending_build_cancel = false;
                return true;
            }
            
            // Handle build commit
            if (data.pending_build_commit) {
                GameState s = load_game(data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to commit");
                    data.pending_build_commit = false;
                    return false;
                }
                
                // Validate and commit (existing logic from before)
                if (!draft_exists(data.game_id, active_player, draft_code)) {
                    Logger::instance().error("Draft not found: " + draft_code);
                    data.pending_build_commit = false;
                    return false;
                }
                
                DraftRow d = load_draft(data.game_id, active_player, draft_code);
                
                // Validate draft
                if (d.attr.type == 'S' && d.attr.SR != 0) {
                    Logger::instance().error("SystemShips cannot have SR");
                    data.pending_build_commit = false;
                    return false;
                }
                if (d.attr.M % 3 != 0) {
                    Logger::instance().error("Missiles must be a multiple of 3");
                    data.pending_build_commit = false;
                    return false;
                }
                if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 ||
                    d.attr.T < 0 || d.attr.M < 0 || d.attr.SR < 0) {
                    Logger::instance().error("Negative attribute");
                    data.pending_build_commit = false;
                    return false;
                }
                
                // Calculate cost
                int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
                cost += (d.attr.M + 2) / 3;
                if (d.attr.type == 'W') cost += 5; // Warp generator
                
                int& bp = (s.active_player == "A") ? s.bpA : s.bpB;
                
                if (cost > bp) {
                    Logger::instance().error("Insufficient BP. Need " + std::to_string(cost) + 
                                           ", have " + std::to_string(bp));
                    data.pending_build_commit = false;
                    return false;
                }
                
                // Compute tech level
                int tech = 0;
                if (s.scenario == "advanced" && s.round >= 1) {
                    tech = (s.round - 1) / 4;
                }
                
                // Create ship
                ShipRow sh;
                sh.code = d.code;
                sh.name = d.name;
                sh.attr.type = d.attr.type;
                sh.attr.PD = d.attr.PD;
                sh.attr.B = d.attr.B;
                sh.attr.S = d.attr.S;
                sh.attr.T = d.attr.T;
                sh.attr.M = d.attr.M;
                sh.attr.SR = d.attr.SR;
                sh.attr.tech = tech;
                sh.built_turn = "R" + std::to_string(s.round) + s.active_player;
                
                // Commit to DB
                insert_ship(data.game_id, s.active_player[0], sh);
                delete_draft(data.game_id, s.active_player[0], d.code);
                set_current_draft(data.game_id, s.active_player[0], "");
                
                // Deduct BP and save
                bp -= cost;
                save_game(s);
                
                Logger::instance().info("Committed: " + sh.name + " - " + sh.code + 
                                      " (L" + std::to_string(sh.attr.tech) + ") cost=" + 
                                      std::to_string(cost) + " BP. Remaining BP=" + std::to_string(bp));
                
                data.pending_build_commit = false;
                return true;
            }
            
            // Handle build set attributes
            if (!data.pending_build_attributes.empty()) {
                GameState s = load_game(data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to modify");
                    data.pending_build_attributes.clear();
                    return false;
                }
                
                // Load draft
                DraftRow d = load_draft(data.game_id, active_player, draft_code);
                
                // Apply attribute updates using C++17 structured bindings
                for (const auto& [attr_id, value] : data.pending_build_attributes) {
                    switch (attr_id) {
                        case AttributeID::POWER_DRIVE:
                            d.attr.PD = value;
                            break;
                        case AttributeID::BEAM:
                            d.attr.B = value;
                            break;
                        case AttributeID::SCREEN:
                            d.attr.S = value;
                            break;
                        case AttributeID::TUBE:
                            d.attr.T = value;
                            break;
                        case AttributeID::MISSILE:
                            d.attr.M = value;
                            break;
                        case AttributeID::SYSTEM_RACK:
                            d.attr.SR = value;
                            break;
                    }
                }
                
                // Update in database
                update_draft_attrs(data.game_id, active_player, draft_code, d);
                
                // Log success
                std::ostringstream msg;
                msg << "Draft updated: " << draft_code << " [PD=" << d.attr.PD 
                    << ", B=" << d.attr.B << ", S=" << d.attr.S << ", T=" << d.attr.T 
                    << ", M=" << d.attr.M << ", SR=" << d.attr.SR << "]";
                Logger::instance().info(msg.str());
                
                data.pending_build_attributes.clear();
                return true;
            }
            
            // Old BuildCommand support (with draft_code parameter)
            if (!data.pending_build_draft.empty()) {
                // Same logic as pending_build_commit but uses data.pending_build_draft
                // TODO: Can be removed once all uses migrate to BuildCommitCommand
                data.pending_build_draft.clear();
            }
            
            // TODO: Implement repair logic when pending_repair_ship is set
            // TODO: Implement resupply logic when pending_resupply_ship is set
#endif
        break;
        // Build/Repair/Resupply logic

        // Handle build list drafts
        if (data.pending_build_list_drafts)
        {
            GameState s = get_game_state();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::vector<DraftRow> drafts =
                load_drafts(data.game_id, active_player);

            if (drafts.empty())
            {
                Logger::instance().info("No drafts found");
            }
            else
            {
                std::ostringstream msg;
                msg << "Drafts (" << drafts.size() << "):";
                for (const auto& d : drafts)
                {
                    msg << "\n  " << d.code << " - " << d.name;
                }
                Logger::instance().info(msg.str());
            }
            data.pending_build_list_drafts = false;
            return true;
        }

        // Handle build show draft
        if (!data.pending_build_show_draft.empty())
        {
            GameState s = get_game_state();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string ship_code = data.pending_build_show_draft;

            if (!draft_exists(data.game_id, active_player, ship_code))
            {
                Logger::instance().error("Draft not found: " + ship_code);
            }
            else
            {
                DraftRow d = load_draft(data.game_id, active_player, ship_code);
                std::ostringstream msg;
                msg << "Draft: " << d.name << " - " << d.code << "\n"
                    << "  Type: " << d.attr.type << "\n"
                    << "  PD=" << d.attr.PD << ", B=" << d.attr.B
                    << ", S=" << d.attr.S << ", T=" << d.attr.T
                    << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
                Logger::instance().info(msg.str());

                // Set as current draft
                set_current_draft(data.game_id, active_player, ship_code);
            }
            data.pending_build_show_draft.clear();
            return true;
        }

        // Handle build cancel
        if (data.pending_build_cancel)
        {
            GameState s = get_game_state();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to cancel");
            }
            else
            {
                delete_draft(data.game_id, active_player, draft_code);
                set_current_draft(data.game_id, active_player, "");
                Logger::instance().info("Canceled draft: " + draft_code);
            }
            data.pending_build_cancel = false;
            return true;
        }

        // Handle build commit
        if (data.pending_build_commit)
        {
            GameState s = get_game_state();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to commit");
                data.pending_build_commit = false;
                return false;
            }

            // Validate and commit (existing logic from before)
            if (!draft_exists(data.game_id, active_player, draft_code))
            {
                Logger::instance().error("Draft not found: " + draft_code);
                data.pending_build_commit = false;
                return false;
            }

            DraftRow d = load_draft(data.game_id, active_player, draft_code);

            // Validate draft
            if (d.attr.type == 'S' && d.attr.SR != 0)
            {
                Logger::instance().error("SystemShips cannot have SR");
                data.pending_build_commit = false;
                return false;
            }
            if (d.attr.M % 3 != 0)
            {
                Logger::instance().error("Missiles must be a multiple of 3");
                data.pending_build_commit = false;
                return false;
            }
            if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 || d.attr.T < 0 ||
                d.attr.M < 0 || d.attr.SR < 0)
            {
                Logger::instance().error("Negative attribute");
                data.pending_build_commit = false;
                return false;
            }

            // Calculate cost
            int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
            cost += (d.attr.M + 2) / 3;
            if (d.attr.type == 'W')
                cost += 5; // Warp generator

            int& bp = (s.active_player == "A") ? s.bpA : s.bpB;

            if (cost > bp)
            {
                Logger::instance().error("Insufficient BP. Need " +
                                         std::to_string(cost) + ", have " +
                                         std::to_string(bp));
                data.pending_build_commit = false;
                return false;
            }

            // Compute tech level
            int tech = 0;
            if (s.scenario == "advanced" && s.round >= 1)
            {
                tech = (s.round - 1) / 4;
            }

            // Create ship
            ShipRow sh;
            sh.code = d.code;
            sh.name = d.name;
            sh.attr.type = d.attr.type;
            sh.attr.PD = d.attr.PD;
            sh.attr.B = d.attr.B;
            sh.attr.S = d.attr.S;
            sh.attr.T = d.attr.T;
            sh.attr.M = d.attr.M;
            sh.attr.SR = d.attr.SR;
            sh.attr.tech = tech;
            sh.built_turn = "R" + std::to_string(s.round) + s.active_player;

            // Commit to DB
            insert_ship(data.game_id, s.active_player[0], sh);
            delete_draft(data.game_id, s.active_player[0], d.code);
            set_current_draft(data.game_id, s.active_player[0], "");

            // Deduct BP and save
            bp -= cost;
            save_game(s);

            Logger::instance().info("Committed: " + sh.name + " - " + sh.code +
                                    " (L" + std::to_string(sh.attr.tech) +
                                    ") cost=" + std::to_string(cost) +
                                    " BP. Remaining BP=" + std::to_string(bp));

            data.pending_build_commit = false;
            return true;
        }

        // Handle build set attributes
        if (!data.pending_build_attributes.empty())
        {
            GameState s = get_game_state();
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to modify");
                data.pending_build_attributes.clear();
                return false;
            }

            // Load draft
            DraftRow d = load_draft(data.game_id, active_player, draft_code);

            // Apply attribute updates using C++17 structured bindings
            for (const auto& [attr_id, value] : data.pending_build_attributes)
            {
                switch (attr_id)
                {
                case AttributeID::POWER_DRIVE:
                    d.attr.PD = value;
                    break;
                case AttributeID::BEAM:
                    d.attr.B = value;
                    break;
                case AttributeID::SCREEN:
                    d.attr.S = value;
                    break;
                case AttributeID::TUBE:
                    d.attr.T = value;
                    break;
                case AttributeID::MISSILE:
                    d.attr.M = value;
                    break;
                case AttributeID::SYSTEM_RACK:
                    d.attr.SR = value;
                    break;
                }
            }

            // Update in database
            update_draft_attrs(data.game_id, active_player, draft_code, d);

            // Log success
            std::ostringstream msg;
            msg << "Draft updated: " << draft_code << " [PD=" << d.attr.PD
                << ", B=" << d.attr.B << ", S=" << d.attr.S
                << ", T=" << d.attr.T << ", M=" << d.attr.M
                << ", SR=" << d.attr.SR << "]";
            Logger::instance().info(msg.str());

            data.pending_build_attributes.clear();
            return true;
        }

        // Old BuildCommand support (with draft_code parameter)
        if (!data.pending_build_draft.empty())
        {
            // Same logic as pending_build_commit but uses
            // data.pending_build_draft
            // TODO: Can be removed once all uses migrate to BuildCommitCommand
            data.pending_build_draft.clear();
        }

        // TODO: Implement repair logic when pending_repair_ship is set
        // TODO: Implement resupply logic when pending_resupply_ship is set

        break;

    default:
        break;
    }
    return false;
}

bool StateMachine::active_player_execute(ICmd* pICmd)
{
    if (!pICmd)
        return false;
    // Command sets properties on the State Slate (invoke), then we turn the
    // crank.
    bool result = pICmd->invoke();
    return result;
}

bool StateMachine::nonactive_player_execute(ICmd* pICmd)
{
    if (!pICmd)
        return false;
    return pICmd->invoke();
}

GameState StateMachine::load_game(int game_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query("SELECT scenario,state_json FROM games WHERE id=" +
                         std::to_string(game_id) + " LIMIT 1");
    if (rows.empty())
    {
        throw std::runtime_error("game not found");
    }
    std::string scenario = rows[0][0];
    std::string state_json = rows[0][1];

    // state_json already includes scenario; trust it.
    GameState s = GameState::from_json_min(state_json);
    s.game_id = game_id;

    // safer: extract scenario/activePlayer/phaseIndex/round/bp/vp from the
    // authoritative stored JSON is omitted. We'll keep a minimal local parse +
    // a fallback:
    if (s.scenario.empty())
    {
        s.scenario = scenario;
    }

    // Quick re-sync by reading a few known fields with string search (simple).
    auto find_int = [&](const std::string& key, int fallback) -> int {
        std::string pat = "\"" + key + "\":";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
            return fallback;
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
                        const std::string& fallback) -> std::string {
        std::string pat = "\"" + key + "\":\"";
        size_t p = state_json.find(pat);
        if (p == std::string::npos)
            return fallback;
        p += pat.size();
        size_t e = state_json.find("\"", p);
        if (e == std::string::npos)
            return fallback;
        return state_json.substr(p, e - p);
    };

    s.round = std::max(1, find_int("round", s.round));
    s.active_player = find_str("activePlayer", s.active_player);
    s.phase_index = find_int("phaseIndex", s.phase_index);
    s.scenario = find_str("scenario", s.scenario);

    // Parse bp/vp objects
    auto find_obj_int = [&](const std::string& objKey,
                            const std::string& fieldKey, int fallback) -> int {
        std::string op = "\"" + objKey + "\":{";
        size_t p = state_json.find(op);
        if (p == std::string::npos)
            return fallback;
        size_t end = state_json.find("}", p + op.size());
        if (end == std::string::npos)
            return fallback;
        std::string sub = state_json.substr(p, end - p + 1);
        return find_int(
            fieldKey,
            fallback); // re-use global find_int (ok for now; small JSON)
    };

    // We'll do simple direct searches for "vp":{"A":X,"B":Y} etc.
    size_t vpPos = state_json.find("\"vp\":{");
    if (vpPos != std::string::npos)
    {
        size_t end = state_json.find("}", vpPos);
        std::string vpSub = state_json.substr(vpPos, end - vpPos + 1);
        auto getv = [&](const std::string& k) -> int {
            std::string pat = "\"" + k + "\":";
            size_t p = vpSub.find(pat);
            if (p == std::string::npos)
                return 0;
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
        auto getv = [&](const std::string& k) -> int {
            std::string pat = "\"" + k + "\":";
            size_t p = bpSub.find(pat);
            if (p == std::string::npos)
                return 0;
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
        s.bpA = getv("A");
        s.bpB = getv("B");
    }

    // Load Combat Summary
    {
        CombatEngine ce(game_id);
        auto combats = ce.get_active_combats();
        if (!combats.empty())
        {
            std::ostringstream c;
            c << "{";
            c << "\"active_hexes\":[";
            for (size_t i = 0; i < combats.size(); ++i)
            {
                if (i > 0)
                    c << ",";
                c << "\"" << combats[i].hex_id << "\"";
            }
            c << "],";
            c << "\"combats\":[";
            for (size_t i = 0; i < combats.size(); ++i)
            {
                if (i > 0)
                    c << ",";
                c << "{\"hex\":\"" << combats[i].hex_id << "\",";
                c << "\"log\":\"" << escape_json(combats[i].last_log) << "\",";
                c << "\"stage\":" << combats[i].stage; // useful for UI
                c << "}";
            }
            c << "],";
            c << "\"count\":" << combats.size();
            c << "}";
            s.combat_summary_json = c.str();
        }
    }

    return s;
}

GameState StateMachine::new_game_state_for_scenario(const std::string& scenario)
{
    GameState s;
    s.scenario = scenario;
    s.round = 1;
    s.active_player = "A";
    s.phase_index = PH_BUILD_SHIPS;
    s.vpA = 0;
    s.vpB = 0;

    if (scenario == "learning")
    {
        s.bpA = 40;
        s.bpB = 40;
    }
    else if (scenario == "basic")
    {
        s.bpA = 50;
        s.bpB = 50;
    }
    else if (scenario == "advanced")
    {
        s.bpA = 20;
        s.bpB = 20; // start of first turn
    }

    // BUGBUG BUGBUG  s.game_id = ????
    return s;
}

void StateMachine::apply_start_of_turn(GameState& s)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    // Called when a player begins their player-turn (phase 0 = Build Ships).
    // 1) Count victory points automatically.
    // 2) In Advanced scenario, award BP (+10) at start of each player-turn
    // after
    //    the very first player-turn.

    if (s.scenario.empty() || s.game_over)
        return;

    // VP: +1 for each enemy base system occupied at start of your turn.
    char me = s.active_player.empty() ? 'A' : s.active_player[0];
    char enemy = (me == 'A') ? 'B' : 'A';

    int vp_gain = 0;
    {
        // star_systems uses map_id (shared map data), ships use game_id
        std::string q =
            "SELECT COUNT(DISTINCT ss.name) "
            "FROM ships sh JOIN star_systems ss ON sh.at_system = ss.name AND ss.map_id=1 "
            "WHERE sh.game_id=" +
            std::to_string(s.game_id) + " AND sh.owner='" + std::string(1, me) +
            "' AND sh.racked_in IS NULL "
            " AND ss.is_base=1 AND ss.base_owner='" +
            std::string(1, enemy) + "'";
        auto r = db.query(q);
        if (!r.empty() && !r[0].empty())
            vp_gain = std::atoi(r[0][0].c_str());
    }

    if (vp_gain > 0)
    {
        if (me == 'A')
            s.vpA += vp_gain;
        else
            s.vpB += vp_gain;
    }

    int need = 3;
    if (s.scenario == "learning")
        need = 1;
    else if (s.scenario == "basic")
        need = 2;
    else if (s.scenario == "advanced")
        need = 3;

    int my_vp = (me == 'A') ? s.vpA : s.vpB;
    if (my_vp >= need)
    {
        s.game_over = true;
        s.winner = std::string(1, me);
        return;
    }

    // Advanced scenario BP cadence.
    if (s.scenario == "advanced")
    {
        bool is_first_player_first_turn = (s.round == 1 && me == 'A');
        if (!is_first_player_first_turn)
        {
            if (me == 'A')
                s.bpA += 10;
            else
                s.bpB += 10;
        }
    }

    // Reset movement points for active player's ships
    db.exec("UPDATE ships SET pd_spent=0 WHERE game_id=" +
            std::to_string(s.game_id) + " AND owner='" + std::string(1, me) +
            "'");
}

void StateMachine::advance_next(GameState& s)
{
    if (s.scenario.empty() || s.game_over)
        return;

    if (s.phase_index < PH_END_TURN)
    {

        // --- Prevent skipping active combat ---
        if (s.phase_index == PH_RESOLVE_COMBAT)
        {
            CombatEngine ce(s.game_id);
            if (!ce.get_active_combats().empty())
            {
                return; // Cannot advance until all combats resolved
            }
        }

        s.phase_index++;

        // --- Combat Trigger Logic ---
        if (s.phase_index == PH_RESOLVE_COMBAT)
        {
            CombatEngine ce(s.game_id);
            ce.check_for_combat_triggers();
            auto combats = ce.get_active_combats();
            
            Logger::instance().info("[advance_next] Combat phase for game " + 
                                   std::to_string(s.game_id) + ", combats found: " + 
                                   std::to_string(combats.size()));

            if (combats.empty())
            {
                // No combat? Auto-skip to next phase
                Logger::instance().info("[advance_next] No combat, skipping to Pick/Drop phase");
                s.phase_index = PH_SYSTEM_PICKDROP;
            }
            else
            {
                Logger::instance().info("[advance_next] Combat detected! Pausing at Combat phase");
                // Combat exists - don't advance further (will return on next call via line 911-918)
            }
        }
        // ----------------------------

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
    }
    s.phase_index = PH_BUILD_SHIPS;
    apply_start_of_turn(s);
}

void StateMachine::save_game(const GameState& s)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q = "UPDATE games SET scenario=";
    if (s.scenario.empty())
    {
        q += "NULL";
    }
    else
    {
        q += "'" + db.esc(s.scenario) + "'";
    }

    q += ", state_json='" + db.esc(s.to_json()) +
         "' WHERE id=" + std::to_string(s.game_id);

    db.exec(q);
}

int StateMachine::next_event_seq(int game_id)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto r = db.query(
        "SELECT COALESCE(MAX(seq),0)+1 FROM game_events WHERE game_id=" +
        std::to_string(game_id));
    if (r.empty())
        return 1;
    return std::atoi(r[0][0].c_str());
}

//------------------------------------------------------------------------------
// Command Inhibit System - centralized validation for all commands
// Check priority: 1) Initiative, 2) Phase, 3) Intra-phase state
//------------------------------------------------------------------------------

bool StateMachine::check_inhibits(CommandID cmd, void* params, std::string& error_msg)
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
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
            return false;
        }
        // 2. Check phase
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg = "Building only allowed during Build Ships phase";
            return false;
        }
        // 3. Parameter-specific checks could go here using:
        // auto* p = static_cast<BuildNewParams_t*>(params);
        return true;
    }
    break;
    
    case CommandID::BUILD_SET:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg = "Build modifications only allowed during Build Ships phase";
            return false;
        }
        return true;
    }
    break;
    
    case CommandID::BUILD_COMMIT:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
            return false;
        }
        if (s.phase_index != PH_BUILD_SHIPS)
        {
            error_msg = "Committing ships only allowed during Build Ships phase";
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
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
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
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
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
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
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
    // Note: Combat has special rules - both players issue orders, then apply
    //--------------------------------------------------------------------------
    case CommandID::COMBAT_ORDER:
    {
        if (!has_initiative)
        {
            error_msg = "It's not your turn (active player: " + s.active_player + ")";
            return false;
        }
        if (s.phase_index != PH_RESOLVE_COMBAT)
        {
            error_msg = "Combat orders only allowed during Combat phase";
            return false;
        }
        // TODO: Intra-phase check - orders must be issued before apply
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
        // TODO: Intra-phase check - both players must have issued orders first
        return true;
    }
    break;
    
    } // end switch
    
    // Default: allow (for any commands not explicitly handled)
    return true;
}

