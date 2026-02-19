///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "db.h"
#include "delete_command.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

#include <sstream>

bool DeleteCommand::invoke(void)
{
    int user_id = StateMachine::instance().get_current_user_id();
    DatabaseManager& db = DatabaseManager::instance();

    if (m_save_name.empty())
    {
        Telemetry::instance().write(LC_SAVE_CMD_NO_NAME);
        return false;
    }

    // Find save by name for this user
    std::string q =
        "SELECT id, save_name FROM saved_games WHERE user_id=? AND save_name=?";

    auto rows = db.Query(q, {user_id, m_save_name});

    if (rows.empty())
    {
        Telemetry::instance().write(
            std::format(LC_SAVE_CMD_NO_SAVED_GAME, m_save_name));
        return false;
    }

    std::string save_id = rows[0][0];
    std::string actual_name = rows[0][1];

    // Delete the save
    std::string dq = "DELETE FROM saved_games WHERE id=?";
    db.Exec(dq, {save_id});
    Telemetry::instance().write(
        std::format(LC_SAVE_CMD_DELETED_GAME, actual_name));
    return true;
}
