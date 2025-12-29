///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "statemachine.h"

#include <iostream>

#include "db.h"
#include "game.h"
#include "logger.h"
#include "telemetry.h"

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
    if (data.state == PlayerState::INVALID ||
        data.state == PlayerState::PREINITIALIZE)
    {
        data.state = PlayerState::READY_GAME_START;
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

    switch (data.state)
    {

    case READY_GAME_START:
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
            s.game_id = data.game_id;

            m_db->exec("DELETE FROM drafts WHERE game_id=" +
                       std::to_string(data.game_id));
            m_db->exec("DELETE FROM ships  WHERE game_id=" +
                       std::to_string(data.game_id));
            set_current_draft(m_db, data.game_id, 'A', "");
            set_current_draft(m_db, data.game_id, 'B', "");
            save_game(m_db, s);
            
            // Update status panel
            Telemetry::status(s.game_id, s.scenario, s.round, s.active_player,
                            s.phase_name(), s.vpA, s.vpB, s.bpA, s.bpB,
                            "Game started: " + sc_str);

            Logger::instance().info(
                "Transition: Game Initialized. Moving to GAME_START.");
            data.state = GAME_START;

            // Clear intent? Or keep it as part of state?
            // User said "We set it. We test it. What's possibly going to change
            // it?" So maybe we don't clear it. It IS the scenario of the game.

            // Auto-advance
            Logger::instance().info(
                "Auto-Transition: GAME_START -> BUILD_PHASE");
            data.state = BUILD_PHASE;
            return true;
        }
        break;

    case BUILD_PHASE:
#if 0
            // Legacy BUILD_PHASE logic - replaced by self-contained Commands
            // Commands now execute their own logic directly
            // This code is kept for reference but should not execute
            
            // Build/Repair/Resupply logic
            
            // Handle build list drafts
            if (data.pending_build_list_drafts) {
                GameState s = load_game(m_db, data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::vector<DraftRow> drafts = load_drafts(m_db, data.game_id, active_player);
                
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
                GameState s = load_game(m_db, data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string ship_code = data.pending_build_show_draft;
                
                if (!draft_exists(m_db, data.game_id, active_player, ship_code)) {
                    Logger::instance().error("Draft not found: " + ship_code);
                } else {
                    DraftRow d = load_draft(m_db, data.game_id, active_player, ship_code);
                    std::ostringstream msg;
                    msg << "Draft: " << d.name << " - " << d.code << "\n"
                        << "  Type: " << d.attr.type << "\n"
                        << "  PD=" << d.attr.PD << ", B=" << d.attr.B << ", S=" << d.attr.S
                        << ", T=" << d.attr.T << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
                    Logger::instance().info(msg.str());
                    
                    // Set as current draft
                    set_current_draft(m_db, data.game_id, active_player, ship_code);
                }
                data.pending_build_show_draft.clear();
                return true;
            }
            
            // Handle build cancel
            if (data.pending_build_cancel) {
                GameState s = load_game(m_db, data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(m_db, data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to cancel");
                } else {
                    delete_draft(m_db, data.game_id, active_player, draft_code);
                    set_current_draft(m_db, data.game_id, active_player, "");
                    Logger::instance().info("Canceled draft: " + draft_code);
                }
                data.pending_build_cancel = false;
                return true;
            }
            
            // Handle build commit
            if (data.pending_build_commit) {
                GameState s = load_game(m_db, data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(m_db, data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to commit");
                    data.pending_build_commit = false;
                    return false;
                }
                
                // Validate and commit (existing logic from before)
                if (!draft_exists(m_db, data.game_id, active_player, draft_code)) {
                    Logger::instance().error("Draft not found: " + draft_code);
                    data.pending_build_commit = false;
                    return false;
                }
                
                DraftRow d = load_draft(m_db, data.game_id, active_player, draft_code);
                
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
                insert_ship(m_db, data.game_id, s.active_player[0], sh);
                delete_draft(m_db, data.game_id, s.active_player[0], d.code);
                set_current_draft(m_db, data.game_id, s.active_player[0], "");
                
                // Deduct BP and save
                bp -= cost;
                save_game(m_db, s);
                
                Logger::instance().info("Committed: " + sh.name + " - " + sh.code + 
                                      " (L" + std::to_string(sh.attr.tech) + ") cost=" + 
                                      std::to_string(cost) + " BP. Remaining BP=" + std::to_string(bp));
                
                data.pending_build_commit = false;
                return true;
            }
            
            // Handle build set attributes
            if (!data.pending_build_attributes.empty()) {
                GameState s = load_game(m_db, data.game_id);
                char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
                std::string draft_code = get_current_draft(m_db, data.game_id, active_player);
                
                if (draft_code.empty()) {
                    Logger::instance().error("No current draft to modify");
                    data.pending_build_attributes.clear();
                    return false;
                }
                
                // Load draft
                DraftRow d = load_draft(m_db, data.game_id, active_player, draft_code);
                
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
                update_draft_attrs(m_db, data.game_id, active_player, draft_code, d);
                
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
            GameState s = load_game(m_db, data.game_id);
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::vector<DraftRow> drafts =
                load_drafts(m_db, data.game_id, active_player);

            if (drafts.empty())
            {
                Logger::instance().info("No drafts found");
            }
            else
            {
                std::ostringstream msg;
                msg << "Drafts (" << drafts.size() << "):";
                for (const auto &d : drafts)
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
            GameState s = load_game(m_db, data.game_id);
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string ship_code = data.pending_build_show_draft;

            if (!draft_exists(m_db, data.game_id, active_player, ship_code))
            {
                Logger::instance().error("Draft not found: " + ship_code);
            }
            else
            {
                DraftRow d =
                    load_draft(m_db, data.game_id, active_player, ship_code);
                std::ostringstream msg;
                msg << "Draft: " << d.name << " - " << d.code << "\n"
                    << "  Type: " << d.attr.type << "\n"
                    << "  PD=" << d.attr.PD << ", B=" << d.attr.B
                    << ", S=" << d.attr.S << ", T=" << d.attr.T
                    << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
                Logger::instance().info(msg.str());

                // Set as current draft
                set_current_draft(m_db, data.game_id, active_player, ship_code);
            }
            data.pending_build_show_draft.clear();
            return true;
        }

        // Handle build cancel
        if (data.pending_build_cancel)
        {
            GameState s = load_game(m_db, data.game_id);
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(m_db, data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to cancel");
            }
            else
            {
                delete_draft(m_db, data.game_id, active_player, draft_code);
                set_current_draft(m_db, data.game_id, active_player, "");
                Logger::instance().info("Canceled draft: " + draft_code);
            }
            data.pending_build_cancel = false;
            return true;
        }

        // Handle build commit
        if (data.pending_build_commit)
        {
            GameState s = load_game(m_db, data.game_id);
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(m_db, data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to commit");
                data.pending_build_commit = false;
                return false;
            }

            // Validate and commit (existing logic from before)
            if (!draft_exists(m_db, data.game_id, active_player, draft_code))
            {
                Logger::instance().error("Draft not found: " + draft_code);
                data.pending_build_commit = false;
                return false;
            }

            DraftRow d =
                load_draft(m_db, data.game_id, active_player, draft_code);

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

            int &bp = (s.active_player == "A") ? s.bpA : s.bpB;

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
            insert_ship(m_db, data.game_id, s.active_player[0], sh);
            delete_draft(m_db, data.game_id, s.active_player[0], d.code);
            set_current_draft(m_db, data.game_id, s.active_player[0], "");

            // Deduct BP and save
            bp -= cost;
            save_game(m_db, s);
            
            // Update status panel
            Telemetry::status(s.game_id, s.scenario, s.round, s.active_player,
                            s.phase_name(), s.vpA, s.vpB, s.bpA, s.bpB,
                            "Ship committed: " + sh.name);

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
            GameState s = load_game(m_db, data.game_id);
            char active_player =
                (s.active_player.empty() ? 'A' : s.active_player[0]);
            std::string draft_code =
                get_current_draft(m_db, data.game_id, active_player);

            if (draft_code.empty())
            {
                Logger::instance().error("No current draft to modify");
                data.pending_build_attributes.clear();
                return false;
            }

            // Load draft
            DraftRow d =
                load_draft(m_db, data.game_id, active_player, draft_code);

            // Apply attribute updates using C++17 structured bindings
            for (const auto &[attr_id, value] : data.pending_build_attributes)
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
            update_draft_attrs(m_db, data.game_id, active_player, draft_code,
                               d);

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

bool StateMachine::active_player_execute(ICmd *pICmd)
{
    if (!pICmd)
        return false;
    // Command sets properties on the State Slate (invoke), then we turn the
    // crank.
    bool result = pICmd->invoke();
    return result;
}

bool StateMachine::nonactive_player_execute(ICmd *pICmd)
{
    if (!pICmd)
        return false;
    return pICmd->invoke();
}
