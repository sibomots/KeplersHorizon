///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_AIAGENT_H__
#define __KH_AIAGENT_H__

#include <string>

#include "state.h"
#include "taskrunner.h"

/**
 * @brief AI Agent - Orchestrates AI player behavior via Task queue
 *
 * The AIAgent manages when the AI should act and creates Tasks containing
 * commands to be executed by the TaskRunner. It does NOT cache any game state -
 * StateMachine is the single source of truth for game_id, active_player, phase.
 *
 * AIAgent only tracks whether it's currently executing a turn (IDLE vs ACTIVE).
 */
class AIAgent
{
  public:
    static AIAgent& instance();

    // Called from advance_next() hooks
    void on_turn_start(int game_id, char ai_player);
    void on_phase_advance(int game_id, char ai_player);
    void on_turn_end(int game_id,
                     char ai_player); // NEW: Reset state when losing control
    void on_combat_detected(int game_id, char ai_player);

    // Called from TaskRunner polling callback
    bool requires_task();
    bool next_task(Task** ppTask);

  private:
    AIAgent();
    ~AIAgent() = default;

    // Prevent copying
    AIAgent(const AIAgent&) = delete;
    AIAgent& operator=(const AIAgent&) = delete;

    enum class TurnState
    {
        IDLE,          // Not executing AI turn
        ACTIVE,        // Currently executing AI turn
        COMBAT_PENDING // AI needs to draft combat orders (Phase 3+)
    };

    TurnState m_state;
    int m_task_sequence; // Just for numbering tasks

    // NO cached game state - StateMachine is source of truth

    // Internal helpers
    void push_next_command(int game_id, char ai_player);
    bool decide_next_command(const GameState& s, char ai_player,
                             std::string& command_out);
    Task* create_ai_task(int game_id, char ai_player, const std::string& cmd);
};

#endif // __AIAGENT_H__
