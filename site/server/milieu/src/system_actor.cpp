///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "system_actor.h"

#include <algorithm>
#include <format>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "telemetry.h"

bool SystemActor::invoke(void)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();

    // Validate system exists and get hex
    std::string system_name = get_system_name();
    std::string q =
        "SELECT name, hex_id FROM star_systems WHERE UPPER(name)=?";
    auto check = db.Query(q, {system_name});

    if (check.empty())
    {
        Telemetry::instance().write(
            std::format(LC_MILIEU_UNKNOWN_SYSTEM, system_name));
        return false;
    }

    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    SystemMode mode = get_mode();

    if (!(mode > FIRST_SYSTEM_STRATEGY && mode < LAST_SYSTEM_STRATEGY))
    {
        Telemetry::instance().write(LC_MILIEU_UNKNOWN_SYSTEM_SUBCOMMAND);
        return false;
    }

    system_name = check[0][0];
    std::string hexId = check[0][1];

    // System investigation costs 1 PD — find a player ship at this hex
    // Ships with LRS investigate for free
    auto scanShips = db.Query(
        "SELECT ship_code, pd, pd_spent, lrs FROM ships "
        " WHERE game_id=? AND owner=? AND at_hex=? AND destroyed_at IS NULL "
        " ORDER BY lrs DESC, (pd - pd_spent) DESC LIMIT 1",
        {s.game_id, owner, hexId});

    if (scanShips.empty())
    {
        Telemetry::instance().write(LC_MILIEU_SYSTEM_NO_SHIPS);
        return false;
    }

    std::string scanShipCode = scanShips[0][0];
    int pdTotal = std::atoi(scanShips[0][1].c_str());
    int pdSpent = std::atoi(scanShips[0][2].c_str());
    int lrs = std::atoi(scanShips[0][3].c_str());
    bool freeScan = (lrs > 0);

    if (!freeScan && (pdTotal - pdSpent) < 1)
    {
        Telemetry::instance().write(
          std::format(LC_MILIEU_SYSTEM_NO_POWER, 1));
        return false;
    }

    SystemParam sp = SystemParam::Builder()
                         .set_game_id(s.game_id)
                         .set_player(owner)
                         .set_mode(mode)
                         .set_system_name(system_name)
                         .build();
    MilieuAgentParam mp(sp);
    bres = MilieuAgent::instance().apply(mp);

    // Deduct 1 PD on success (LRS-equipped ships investigate for free)
    if (bres && !freeScan)
    {
        db.Exec("UPDATE ships SET pd_spent=pd_spent+1 "
                "WHERE game_id=? AND owner=? AND ship_code=?",
                {s.game_id, owner, scanShipCode});
    }

    return bres;
}
