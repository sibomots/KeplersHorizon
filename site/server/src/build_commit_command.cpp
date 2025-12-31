//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "build_commit_command.h"

#include <sstream>

#include "telemetry.h"
#include "ships.h"
#include "logger.h"
#include "typedefs.h"

bool BuildCommitCommand::invoke(void)
{
    // Get active player from game state
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Get current draft
    std::string draft_code = get_current_draft(game_id, active_player);
    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to commit");
        Telemetry::write("Error: No current draft to commit");
        return false;
    }

    // Validate draft exists
    if (!draft_exists(game_id, active_player, draft_code))
    {
        Logger::instance().error("Draft not found: " + draft_code);
        Telemetry::write("Error: Draft not found: " + draft_code);
        return false;
    }

    // Load draft
    DraftRow d = load_draft(game_id, active_player, draft_code);

    // Validate draft attributes
    if (d.attr.type == 'S' && d.attr.SR != 0)
    {
        Logger::instance().error("SystemShips cannot have SR");
        Telemetry::write("Error: SystemShips cannot have SR");
        return false;
    }
    if (d.attr.M % 3 != 0)
    {
        Logger::instance().error("Missiles must be a multiple of 3");
        Telemetry::write("Error: Missiles must be a multiple of 3");
        return false;
    }
    if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 || d.attr.T < 0 ||
        d.attr.M < 0 || d.attr.SR < 0)
    {
        Logger::instance().error("Negative attribute");
        Telemetry::write("Error: Negative attribute values not allowed");
        return false;
    }

    // Calculate cost
    int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
    cost += (d.attr.M + 2) / 3;
    if (d.attr.type == 'W')
        cost += 5; // Warp generator

    // Check BP availability
    int& bp = (s.active_player == "A") ? s.bpA : s.bpB;
    if (cost > bp)
    {
        Logger::instance().error("Insufficient BP. Need " +
                                 std::to_string(cost) + ", have " +
                                 std::to_string(bp));
        Telemetry::write("Error: Insufficient BP. Need " +
                         std::to_string(cost) + ", have " + std::to_string(bp));
        return false;
    }

    // Compute tech level
    int tech = 0;
    if (s.scenario == "advanced" && s.round >= 1)
    {
        tech = (s.round - 1) / 4;
    }

    // Create ship using factory method
    ShipRow sh = ShipRow::from_draft(d, tech, s.round, s.active_player);

    // Commit to DB
    insert_ship(game_id, s.active_player[0], sh);
    delete_draft(game_id, s.active_player[0], d.code);
    set_current_draft(game_id, s.active_player[0], "");

    // Deduct BP and save
    bp -= cost;
    StateMachine::getInstance().save_game(s);

    Logger::instance().info("Committed: " + sh.name + " - " + sh.code + " (L" +
                            std::to_string(sh.attr.tech) +
                            ") cost=" + std::to_string(cost) +
                            " BP. Remaining BP=" + std::to_string(bp));

    Telemetry::write("Committed: " + sh.name + " - " + sh.code + " (L" +
                     std::to_string(sh.attr.tech) + ")");
    Telemetry::write("Cost: " + std::to_string(cost) +
                     " BP, Remaining: " + std::to_string(bp) + " BP");

    return true;
}
