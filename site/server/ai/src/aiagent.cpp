///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "aiagent.h"

#include "aigamestate.h"
#include "ailogic.h"
#include "logger.h"
#include "statemachine.h"
#include "taskrunner.h"

AIAgent& AIAgent::instance()
{
    static AIAgent singleton;
    return singleton;
}

AIAgent::AIAgent() : m_state(TurnState::IDLE), m_task_sequence(0)
{
}

void AIAgent::on_turn_start(int game_id, char ai_player)
{
    bool should_start = true;

    if (KH_EQU(m_state, TurnState::ACTIVE))
    {
        Logger::instance().ai("Already active, ignoring turn start");
        should_start = false;
    }

    if (should_start)
    {
        m_state = TurnState::ACTIVE;

        Logger::instance().ai("Turn started for " +
                                std::string(1, ai_player) +
                                ", game_id=" + std::to_string(game_id));

        push_next_command(game_id, ai_player);
    }
}

void AIAgent::on_phase_advance(int game_id, char ai_player)
{
    bool should_continue = true;

    if (m_state != TurnState::ACTIVE)
    {
        should_continue = false;
    }

    if (should_continue)
    {
        GameState s = StateMachine::instance().get_game_state();

        if (s.active_player.empty() || s.active_player[0] != ai_player)
        {
            Logger::instance().ai(
                "Turn ended (control transferred during phase)");
            m_state = TurnState::IDLE;
            should_continue = false;
        }
    }

    if (should_continue)
    {
        push_next_command(game_id, ai_player);
    }
}

void AIAgent::on_turn_end(int game_id, char ai_player)
{
    Logger::instance().ai("Turn ended for " +
                            std::string(1, ai_player) + ", resetting to IDLE");
    m_state = TurnState::IDLE;
}

void AIAgent::on_combat_detected(int game_id, char ai_player)
{
    Logger::instance().ai("Combat detected, but not handling yet");
}

bool AIAgent::requires_task()
{
    bool needs_task = false;

    try
    {
        int sm_game_id = StateMachine::instance().get_game_id();

        if (sm_game_id != 0)
        {
            GameState s = StateMachine::instance().get_game_state();

            if (StateMachine::instance().is_ai_player(s.active_player)
                && KH_EQU(m_state, TurnState::IDLE))
            {
                needs_task = true;
            }
        }
    }
    catch (const std::runtime_error&)
    {
        needs_task = false;
    }

    return needs_task;
}

bool AIAgent::next_task(Task** ppTask)
{
    bool created_task = false;

    if (requires_task())
    {
        GameState s = StateMachine::instance().get_game_state();

        if (StateMachine::instance().is_ai_player(s.active_player)
            && KH_EQU(m_state, TurnState::IDLE))
        {
            m_state = TurnState::ACTIVE;

            std::string cmd;
            if (decide_next_command(s, s.active_player[0], cmd))
            {
                *ppTask = create_ai_task(s.game_id, s.active_player[0], cmd);
                Logger::instance().ai(std::format(
                  "Poll created task: {} ", cmd));
                created_task = true;
            }
        }
    }

    return created_task;
}

void AIAgent::push_next_command(int game_id, char ai_player)
{
    bool should_push = true;
    GameState s = StateMachine::instance().get_game_state();

    if (s.active_player.empty() || s.active_player[0] != ai_player)
    {
        Logger::instance().ai("Not our turn, stopping");
        m_state = TurnState::IDLE;
        should_push = false;
    }

    if (should_push)
    {
        std::string cmd;
        if (decide_next_command(s, ai_player, cmd))
        {
            Task* task = create_ai_task(game_id, ai_player, cmd);
            Logger::instance().ai(std::format("Pushing task: {}", cmd));
            TaskRunner::instance().push(task);
        }
    }
}

bool AIAgent::decide_next_command(const GameState& s, char ai_player,
                                  std::string& command_out)
{
    bool success = false;
    AIGameState state(s.game_id, ai_player);

    switch (s.phase_index)
    {
    case PH_BUILD_SHIPS:
    {
        AILogic::BuildDecision decision;
        success = AILogic::instance().decide_build_action(state, decision,
                                                          command_out);
        break;
    }

    case PH_MOVEMENT:
    {
        AILogic::MovementDecision decision;
        success = AILogic::instance().decide_movement_action(state, decision,
                                                             command_out);
        break;
    }

    case PH_RESOLVE_COMBAT:
    {
        AILogic::CombatDecision decision;
        success = AILogic::instance().decide_combat_action(state, decision,
                                                           command_out);
        break;
    }

    case PH_SYSTEM_PICKDROP:
    {
        AILogic::PickDropDecision decision;
        success = AILogic::instance().decide_pickup_drop_action(state, decision,
                                                                command_out);
        break;
    }

    case PH_END_TURN:
        command_out = "DONE";
        success = true;
        break;

    default:
        command_out = "NEXT";
        success = true;
        break;
    }

    return success;
}

Task* AIAgent::create_ai_task(int game_id, char ai_player,
                              const std::string& cmd)
{
    return new Task(game_id, ai_player, cmd, ++m_task_sequence);
}
