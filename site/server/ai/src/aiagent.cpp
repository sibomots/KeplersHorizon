//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "aiagent.h"
#include "logger.h"
#include "statemachine.h"
#include "taskrunner.h"

AIAgent& AIAgent::instance()
{
    static AIAgent singleton;
    return singleton;
}

AIAgent::AIAgent()
    : m_state(TurnState::IDLE)
    , m_task_sequence(0)
{
}

void AIAgent::on_turn_start(int game_id, char ai_player)
{
    if (m_state == TurnState::ACTIVE)
    {
        Logger::instance().info("[AI] Already active, ignoring turn start");
        return;
    }

    m_state = TurnState::ACTIVE;

    Logger::instance().info("[AI] Turn started for Player " +
                            std::string(1, ai_player) +
                            ", game_id=" + std::to_string(game_id));

    // Push first task to runner
    push_next_command(game_id, ai_player);
}

void AIAgent::on_phase_advance(int game_id, char ai_player)
{
    if (m_state != TurnState::ACTIVE)
    {
        // Not executing a turn, ignore
        return;
    }

    // Verify we still have control by checking StateMachine
    GameState s = StateMachine::instance().get_game_state();

    if (s.active_player.empty() || s.active_player[0] != ai_player)
    {
        Logger::instance().info("[AI] Turn ended (control transferred during phase)");
        m_state = TurnState::IDLE;
        return;
    }

    // Still our turn - push next command
    push_next_command(game_id, ai_player);
}

void AIAgent::on_turn_end(int game_id, char ai_player)
{
    Logger::instance().info("[AI] Turn ended for Player " + 
                            std::string(1, ai_player) + 
                            ", resetting to IDLE");
    m_state = TurnState::IDLE;
}

void AIAgent::on_combat_detected(int game_id, char ai_player)
{
    // Phase 2: Not handling combat yet
    Logger::instance().info("[AI] Combat detected, but not handling yet (Phase 2)");

    // Phase 3+: Check if AI has ships in combat, draft orders
    // if (has_ships_in_combat(game_id, ai_player)) {
    //     m_state = TurnState::COMBAT_PENDING;
    //     push_combat_orders(game_id, ai_player);
    // }
}

bool AIAgent::requires_task()
{
    // Guard against no game loaded
    try
    {
        int sm_game_id = StateMachine::instance().get_game_id();
        if (sm_game_id == 0)
        {
            return false;  // No game loaded yet
        }

        GameState s = StateMachine::instance().get_game_state();

        // Case 1: AI is active player but we haven't started turn yet
        if (StateMachine::instance().is_ai_player(s.active_player) &&
            m_state == TurnState::IDLE)
        {
            return true;
        }

        // Case 2: Combat with AI ships (Phase 3+)
        // if (s.phase_index == PH_RESOLVE_COMBAT && has_ships_in_combat(s)) {
        //     return true;
        // }
    }
    catch (const std::runtime_error&)
    {
        // Game not found - server starting up or no game created yet
        return false;
    }

    return false;
}

bool AIAgent::next_task(Task** ppTask)
{
    if (!requires_task())
    {
        return false;
    }

    GameState s = StateMachine::instance().get_game_state();

    // Start AI's turn
    if (StateMachine::instance().is_ai_player(s.active_player) &&
        m_state == TurnState::IDLE)
    {
        m_state = TurnState::ACTIVE;

        std::string cmd = decide_next_command(s);
        *ppTask = create_ai_task(s.game_id, s.active_player[0], cmd);

        Logger::instance().info("[AI] Poll created task: " + cmd);
        return true;
    }

    // Phase 3+: Handle combat orders
    // if (s.phase_index == PH_RESOLVE_COMBAT) {
    //     std::string combat_cmd = decide_combat_order(s);
    //     *ppTask = create_ai_task(s.game_id, ai_player, combat_cmd);
    //     return true;
    // }

    return false;
}

void AIAgent::push_next_command(int game_id, char ai_player)
{
    // Query fresh state from StateMachine
    GameState s = StateMachine::instance().get_game_state();

    // Safety check - still our turn?
    if (s.active_player.empty() || s.active_player[0] != ai_player)
    {
        Logger::instance().info("[AI] Not our turn, stopping");
        m_state = TurnState::IDLE;
        return;
    }

    std::string cmd = decide_next_command(s);
    Task* task = create_ai_task(game_id, ai_player, cmd);

    Logger::instance().info("[AI] Pushing task: " + cmd);

    // Push to TaskRunner
    TaskRunner::instance().push(task);
}

std::string AIAgent::decide_next_command(const GameState& s)
{
    // Phase 2: Simple logic - just advance through phases
    if (s.phase_index == PH_END_TURN)
    {
        return "DONE";
    }
    else
    {
        return "NEXT";
    }

    // Phase 3+: Intelligent decisions per phase
    // switch (s.phase_index)
    // {
    //     case PH_BUILD_SHIPS:
    //         return AILogic::instance().decide_build_action(s);
    //     case PH_MOVEMENT:
    //         return AILogic::instance().decide_movement_action(s);
    //     case PH_RESOLVE_COMBAT:
    //         return AILogic::instance().decide_combat_tactic(s, ship);
    //     case PH_SYSTEM_PICKDROP:
    //         return check_pickup_drop(s);
    //     case PH_END_TURN:
    //         return "DONE";
    // }
}

Task* AIAgent::create_ai_task(int game_id, char ai_player, const std::string& cmd)
{
    return new Task(game_id, ai_player, cmd, ++m_task_sequence);
}
