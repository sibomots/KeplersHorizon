//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "delete_command.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool DeleteCommand::invoke(void)
{
    int user_id = StateMachine::getInstance().get_current_user_id();
    DatabaseManager& db = DatabaseManager::getInstance();

    if (m_save_name.empty())
    {
        Telemetry::getInstance().write("DELETE: No save name specified.\n"
                                       "Usage: delete <save_name>");
        return false;
    }

    // Find save by name for this user
    auto rows = db.query("SELECT id, save_name FROM saved_games WHERE user_id=" +
                         std::to_string(user_id) + " AND save_name='" +
                         db.esc(m_save_name) + "'");

    if (rows.empty())
    {
        Telemetry::getInstance().write("DELETE: No saved game '" + m_save_name +
                                       "' found.");
        return false;
    }

    std::string save_id = rows[0][0];
    std::string actual_name = rows[0][1];

    // Delete the save
    db.exec("DELETE FROM saved_games WHERE id=" + save_id);

    Telemetry::getInstance().write("DELETE: game '" + actual_name + "' deleted.");

    return true;
}
