///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "gamedev_command.h"

#include <format>

#include "db.h"
#include "gamedev_state.h"
#include "statemachine.h"
#include "telemetry.h"

bool GameDevCommand::invoke(void)
{
    GameDevState& gds = GameDevState::instance();
    std::string msg;
    bool result = true;

    switch (m_subcmd)
    {
    case GD_STATUS:
        gds.get_status(msg);
        Telemetry::instance().write(msg);
        break;

    case GD_RESET:
        gds.reset();
        Telemetry::instance().write(
            "GAMEDEV: All parameters reset to defaults.");
        break;

    case GD_ENVIRONMENT:
        gds.set_environment_chance(m_value);
        msg = "GAMEDEV: Environment effect chance set to " +
              std::to_string(m_value) + "%";
        Telemetry::instance().write(msg);
        break;

    case GD_COMBAT:
        gds.set_combat_modifier(m_value);
        msg = "GAMEDEV: Combat modifier override set to ";
        if (m_value >= 0)
        {
            msg += "+";
        }
        msg += std::to_string(m_value);
        Telemetry::instance().write(msg);
        break;

    case GD_MOVEMENT:
        gds.set_movement_modifier(m_value);
        msg = "GAMEDEV: Movement modifier override set to ";
        if (m_value >= 0)
        {
            msg += "+";
        }
        msg += std::to_string(m_value);
        Telemetry::instance().write(msg);
        break;

    case GD_CRT:
        gds.set_force_crt(m_value);
        if (m_value > 0)
        {
            msg = "GAMEDEV: CRT roll forced to " + std::to_string(m_value);
        }
        else
        {
            msg = "GAMEDEV: CRT roll set to natural";
        }
        Telemetry::instance().write(msg);
        break;

    case GD_VP:
    {
        StateMachine& sm = StateMachine::instance();
        GameState s = sm.get_game_state();
        int game_id = sm.get_game_id();
        char me = sm.get_current_player();

        // Update the DB vp column for current player
        std::string vp_col = (KH_EQU(me, 'A')) ? "vp_A" : "vp_B";
        DatabaseManager& db = DatabaseManager::instance();
        db.Exec(std::format("UPDATE games SET {}=? WHERE id=?", vp_col),
                {m_value, game_id});

        // Update in-memory state_json as well
        if (KH_EQU(me, 'A'))
        {
            s.vpA = m_value;
        }
        else
        {
            s.vpB = m_value;
        }
        db.Exec("UPDATE games SET state_json=? WHERE id=?",
                {s.to_json(), game_id});

        msg = std::format("GAMEDEV: VP for player {} set to {}", me, m_value);
        Telemetry::instance().write(msg);
        break;
    }

    default:
        Telemetry::instance().write("GAMEDEV: Unknown subcommand.");
        result = false;
        break;
    }

    return result;
}
