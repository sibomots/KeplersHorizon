///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "status_command.h"

#include <iomanip>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool StatusCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    DatabaseManager& db = DatabaseManager::instance();

    std::string this_player_real_name =
            StateMachine::instance().get_player_name(game_id, s.active_player);
    std::string other_player_real_name =
            StateMachine::instance().get_player_name(game_id, 
         (KH_EQU(s.active_player, "A") ? "B" : "A")); 
    
    std::ostringstream out;
    out << "              GAME STATUS\n";
    out << "────────────────────────────────────────────\n";
    out << "Round: " << s.round << "            Active: " << this_player_real_name << "\n";
    out << "Phase: " << s.phase_name() << "\n";
    out << "────────────────────────────────────────────\n";
    out << "              " << this_player_real_name << "      " << other_player_real_name << "\n";
    out << "Credits: " << std::setw(8) << s.creditsA << "    " << std::setw(8)
        << s.creditsB << "\n";
    out << "VP:      " << std::setw(8) << s.vpA << "    " << std::setw(8)
        << s.vpB << "\n";

    // Count ships
    std::string q = "SELECT COUNT(*) FROM ships WHERE game_id=? AND owner=? "
                    "AND destroyed_at IS NULL";
    auto countA = db.Query(q, {game_id, 'A'});
    auto countB = db.Query(q, {game_id, 'B'});

    int shipsA = countA.empty() ? 0 : std::atoi(countA[0][0].c_str());
    int shipsB = countB.empty() ? 0 : std::atoi(countB[0][0].c_str());

    out << "Ships:   " << std::setw(8) << shipsA << "    " << std::setw(8)
        << shipsB << "\n";
    out << "────────────────────────────────────────────\n";


    if (s.game_over)
    {
        std::string winning_player_real_name =
            StateMachine::instance().get_player_name(game_id, s.winner);
        out << std::format(LC_GAME_WINNER, winning_player_real_name)
            << "\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}
