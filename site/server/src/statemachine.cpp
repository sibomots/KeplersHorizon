#include "statemachine.h"
#include "logger.h"
#include "game.h"
#include "db.h"
#include <iostream>

// Methods doing work do NOT move state.
bool StateMachine::preinitialize() {
    Logger::instance().info("StateMachine::preinitialize");
    // Work only
    return true;
}

bool StateMachine::initialize() {
    Logger::instance().info("StateMachine::initialize");
    // Work only.
    // Assuming startup logic sets initial state via a different mechanism or 
    // we explicitly set "Ready" if initialization is confirmed successful by the caller.
    
    // For now, to satisfy the flow, we will assume we are ready after init.
    // But per user instruction, we don't do transition logic here if it's "handling a state".
    // We'll set the initial valid state for the game loop.
    if (data.state == PlayerState::INVALID || data.state == PlayerState::PREINITIALIZE) {
         data.state = PlayerState::READY_GAME_START;
    }
    return true;
}

bool StateMachine::start_game_for_random_player() {
    // TBD logic
    return true;
}

// Core State Machine Transition Logic
// The "Big Case Statement" that manages lifecycle and executes work based on properties set by Agents/Commands.
bool StateMachine::transition() {
    Logger::instance().info("StateMachine::transition current_state=" + std::to_string(data.state));

    switch (data.state) {
        
        case READY_GAME_START:
            // Check if we have a pending scenario from StartCommand
            // The State Slate has been updated by the Agent (StartCommand) with the intent.
            if (data.scenario != ScenarioType::UNDEFINED) {
                // Determine Scenario String
                std::string sc_str = "";
                switch(data.scenario) {
                    case ScenarioType::LEARNING: sc_str = "learning"; break;
                    case ScenarioType::BASIC:    sc_str = "basic"; break;
                    case ScenarioType::ADVANCED: sc_str = "advanced"; break;
                    default: break;
                }

                Logger::instance().info("Transition: initializing game scenario: " + sc_str);

                // Logic to Establish the Game (DB operations)
                // At this point, we rely on the DB connection being valid (invariant).
                GameState s = new_game_state_for_scenario(sc_str);
                s.game_id = data.game_id;
                
                m_db->exec("DELETE FROM drafts WHERE game_id=" + std::to_string(data.game_id));
                m_db->exec("DELETE FROM ships  WHERE game_id=" + std::to_string(data.game_id));
                set_current_draft(m_db, data.game_id, 'A', "");
                set_current_draft(m_db, data.game_id, 'B', "");
                save_game(m_db, s);
                
                Logger::instance().info("Transition: Game Initialized. Moving to GAME_START.");
                data.state = GAME_START;
                
                // Clear intent? Or keep it as part of state?
                // User said "We set it. We test it. What's possibly going to change it?"
                // So maybe we don't clear it. It IS the scenario of the game.
                
                // Auto-advance
                Logger::instance().info("Auto-Transition: GAME_START -> BUILD_PHASE");
                data.state = BUILD_PHASE;
                return true;
            }
            break;

        case BUILD_PHASE:
            // Logic for build phase transitions
            break;

        default:
            break;
    }
    return false;
}

bool StateMachine::active_player_execute(ICmd* pICmd) {
    if (!pICmd) return false;
    // Command sets properties on the State Slate (invoke), then we turn the crank.
    bool result = pICmd->invoke(); 
    return result;
}

bool StateMachine::nonactive_player_execute(ICmd* pICmd) {
    if (!pICmd) return false;
    return pICmd->invoke();
}

bool StateMachine::nonactive_player_execute(std::shared_ptr<ICmd> pICmd) {
    // Non-active player might only be allowed certain info commands.
    return pICmd->invoke();
}
