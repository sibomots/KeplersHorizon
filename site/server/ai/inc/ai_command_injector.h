//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __AI_COMMAND_INJECTOR_H__
#define __AI_COMMAND_INJECTOR_H__

#include <string>
#include <vector>

/**
 * @brief Executes game commands as the AI player
 *
 * AICommandInjector simulates commands from the AI player by:
 * 1. Setting up StateMachine context (game_id, current_player, user_id)
 * 2. Executing commands through the same parser used for human players
 * 3. Logging all AI actions
 *
 * This ensures AI follows the same rules, inhibits, and command processing
 * as human players - no special privileges or shortcuts.
 */

class AICommandInjector
{
  public:
    static AICommandInjector& instance()
    {
        static AICommandInjector _instance;
        return _instance;
    }

    /**
     * @brief Enqueue a command to be executed as the AI player
     *
     * @param game_id The game ID
     * @param ai_player The AI's player side ('A' or 'B')
     * @param cmdline The command to execute (e.g., "bn w destroyer")
     *
     * NOTE: This does NOT execute the command. It enqueues a Task for
     * the TaskRunner to execute. Fire-and-forget semantics.
     *
     * Example:
     *   inject(42, 'B', "bn w attacker");
     *   inject(42, 'B', "bs w1 pd=5 b=3 s=2");
     *   inject(42, 'B', "bc w1");
     */
    static void inject(int game_id, char ai_player, const std::string& cmdline);

    /**
     * @brief Execute a sequence of commands
     *
     * @param game_id The game ID
     * @param ai_player The AI's player side
     * @param commands List of commands to execute
     * @return true if all commands succeeded, false if any failed
     *
     * Stops at first failure and returns false.
     */
    static bool inject_batch(int game_id, char ai_player,
                             const std::vector<std::string>& commands);

  private:
    AICommandInjector() = default;
    ~AICommandInjector() = default;
    AICommandInjector(const AICommandInjector&) = delete;
    AICommandInjector& operator=(const AICommandInjector&) = delete;
    AICommandInjector(AICommandInjector&&) = delete;
    AICommandInjector& operator=(AICommandInjector&&) = delete;

    /**
     * @brief Get AI user ID from database
     * @return User ID for AI_AGENT (typically 3)
     */
    static int get_ai_user_id();
};

#endif // __AI_COMMAND_INJECTOR_H__
