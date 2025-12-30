//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "build_list_drafts_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
#include "typs.h"

bool BuildListDraftsCommand::invoke(void)
{
    GameState s = m_sm.get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::vector<DraftRow> drafts =
        load_drafts(m_sm.get_db(), m_sm.get_game_id(), active_player);
    std::string current_draft =
        get_current_draft(m_sm.get_db(), m_sm.get_game_id(), active_player);

    if (drafts.empty())
    {
        Logger::instance().info("No drafts found");
        Telemetry::write("No drafts found");
    }
    else
    {
        std::ostringstream msg;
        msg << "Draft ships (" << drafts.size() << "):\n";
        for (const auto &d : drafts)
        {
            // Calculate cost
            int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
            cost += (d.attr.M + 2) / 3;
            if (d.attr.type == 'W')
                cost += 5;

            msg << "  " << d.code << " '" << d.name << "' cost=" << cost
                << " BP\n";
            msg << "    Type=" << d.attr.type << " PD=" << d.attr.PD
                << " B=" << d.attr.B << " S=" << d.attr.S << " T=" << d.attr.T
                << " M=" << d.attr.M << " SR=" << d.attr.SR;

            if (d.code == current_draft)
                msg << "  [current]";
            msg << "\n";
        }
        Logger::instance().info(msg.str());
        Telemetry::write(msg.str());
    }

    return true;
}
