///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "pick_drop_command.h"

#include "db.h"
#include "logger.h"
#include "shipmgr.h"
#include "statemachine.h"
#include "telemetry.h"

bool PickCommand::invoke(void)
{
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::PICK,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Load systemship
    ShipRow sysship;
    if (!shipmgr.load_ship_by_code_or_name(sysship, game_id, active_player,
                                           m_systemship_code))
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_NO_SHIP_IN_FLEET,
                   m_systemship_code));
        return false;
    }

    // Verify it's a systemship (no warp generator)
    if (KH_EQU(sysship.attr.type, 'W'))
    {
        Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_INHIBIT_WARPSHIP,
                       sysship.name));
        return false;
    }

    // Verify it's not already racked
    if (!sysship.racked_in.empty())
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_ALREADY_HANGAR,
                      sysship.name, sysship.racked_in));
        return false;
    }

    // Load warpship
    ShipRow warpship;
    if (!shipmgr.load_ship_by_code_or_name(warpship, game_id, active_player,
                                           m_warpship_code))
    {
        Telemetry::instance().write(
              std::format(LC_PICKDROP_TARGET_NO_WARPSHIP_IN_FLEET,
                     m_warpship_code));
        return false;
    }

    // Verify it's a warpship
    if (warpship.attr.type != 'W')
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_NOT_WARPSHIP,
                       warpship.name));
        return false;
    }

    // Verify warpship has system rack capacity
    if (warpship.attr.Hangar <= 0)
    {
        Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_WARPSHIP_LACK_HANGAR,
                     warpship.name));
        return false;
    }

    // Verify both ships are at the same hex
    if (sysship.at_hex != warpship.at_hex)
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_WARPSHIP_LOCATION,
            sysship.name, sysship.at_hex, warpship.name, warpship.at_hex));
        return false;
    }

    // Count how many systemships already racked in this warpship
    auto racked_count =
        db.Query("SELECT COUNT(*) FROM ships WHERE game_id=? AND owner=? "
                 "AND racked_in=? AND destroyed_at IS NULL",
                 {game_id, active_player, warpship.code});
    int current_racked = 0;
    if (!racked_count.empty() && !racked_count[0].empty())
    {
        current_racked = std::atoi(racked_count[0][0].c_str());
    }

    if (current_racked >= warpship.attr.Hangar)
    {
        Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_WARPSHIP_HANGAR_CAPACITY,
                   warpship.name, current_racked, warpship.attr.Hangar));
        return false;
    }

    // Rack the systemship
    shipmgr.update_ship_location(game_id, active_player, sysship.code, "", "",
                                 warpship.code);

    Telemetry::instance().write(
           std::format(LC_PICKDROP_TARGET_SYSTEMSHIP_HANGAR_SUCCESS,
                    sysship.name, warpship.name));
    return true;
}

bool DropCommand::invoke(void)
{
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::DROP,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = StateMachine::instance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Load systemship
    ShipRow sysship;
    if (!shipmgr.load_ship_by_code_or_name(sysship, game_id, active_player,
                                           m_systemship_code))
    {
        Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_NO_SHIP_IN_FLEET,
                      m_systemship_code));
        return false;
    }

    // Verify it's racked
    if (sysship.racked_in.empty())
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_SYSTEMSHIP_NOT_IN_HANGAR,
                      sysship.name));
        return false;
    }

    // Load warpship
    ShipRow warpship;
    if (!shipmgr.load_ship_by_code_or_name(warpship, game_id, active_player,
                                           m_warpship_code))
    {
        Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_NO_WARP_SHIP_IN_FLEET,
                    m_warpship_code));
        return false;
    }

    // Verify systemship is racked in this warpship
    if (sysship.racked_in != warpship.code)
    {
        Telemetry::instance().write(
               std::format(LC_PICKDROP_TARGET_SYSTEMSHIP_HELD_DIFFERENT,
                       sysship.name, warpship.name, sysship.racked_in));
        return false;
    }

    // Warpship must be at a star hex to drop
    if (warpship.at_system.empty())
    {
        Telemetry::instance().write(
            std::format(LC_PICKDROP_TARGET_INHIBIT_DROP_SPACE,
              warpship.name));
        return false;
    }

    // Drop the systemship at warpship's location
    shipmgr.update_ship_location(game_id, active_player, sysship.code,
                                 warpship.at_system, warpship.at_hex, "");

    Telemetry::instance().write(
             std::format(LC_PICKDROP_TARGET_SYSTEMSHIP_DROP_SUCCESS,
                  sysship.name, warpship.name, warpship.at_system));
    return true;
}
