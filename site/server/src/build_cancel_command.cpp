//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_cancel_command.h"

#include "game.h"
#include "logger.h"
#include "telemetry.h"

bool BuildCancelCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::string draft_code =
        get_current_draft(m_sm.get_db(), m_sm.get_game_id(), active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to cancel");
        Telemetry::write("Error: No current draft to cancel");
        return false;
    }

    delete_draft(m_sm.get_db(), m_sm.get_game_id(), active_player, draft_code);
    set_current_draft(m_sm.get_db(), m_sm.get_game_id(), active_player, "");
    Logger::instance().info("Canceled draft: " + draft_code);
    Telemetry::write("Canceled draft: " + draft_code);

    return true;
}
