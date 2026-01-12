//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "quit_command.h"

#include <ctime>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

bool QuitCommand::invoke(void)
{
    int game_id = StateMachine::getInstance().get_game_id();
    int user_id = StateMachine::getInstance().get_current_user_id();
    DatabaseManager& db = DatabaseManager::getInstance();

    // Generate auto-save name with timestamp: ASF-DD-MM-YYYY
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "ASF-%d-%m-%Y", t);
    std::string save_name(buf);

    // Check for existing auto-save from quit, update or insert
    auto existing =
        db.query("SELECT id FROM saved_games WHERE user_id=" +
                 std::to_string(user_id) + " AND save_name LIKE 'ASF-%'");

    if (!existing.empty())
    {
        db.exec("UPDATE saved_games SET game_id=" + std::to_string(game_id) +
                ", save_name='" + db.esc(save_name) +
                "', saved_at=NOW() WHERE id=" + existing[0][0]);
    }
    else
    {
        db.exec("INSERT INTO saved_games(user_id, save_name, game_id) VALUES(" +
                std::to_string(user_id) + ",'" + db.esc(save_name) + "'," +
                std::to_string(game_id) + ")");
    }

    Logger::instance().info("QUIT: Auto-saved game as '" + save_name + "'");

    std::ostringstream out;
    out << "QUIT: Game auto-saved as '" << save_name << "'.\n";
    out << "Returning to lobby.\n";
    out << "\033[LOBBY]"; // Signal client to return to lobby

    Telemetry::getInstance().write(out.str());

    // Reset state machine to lobby state
    StateMachine::getInstance().clear_game_session();

    return true;
}
