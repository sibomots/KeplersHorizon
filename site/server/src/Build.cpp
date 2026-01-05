//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <sstream>

#include "build_cancel_command.h"
#include "build_command.h"
#include "build_commit_command.h"
#include "build_list_drafts_command.h"
#include "build_new_command.h"
#include "build_set_attribute_command.h"
#include "build_show_draft_command.h"
#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildCancelCommand::invoke(void)
{
    // Redundancy comment
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    std::string draft_code = get_current_draft(game_id, active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to cancel");
        Telemetry::getInstance().write("Error: No current draft to cancel");
        return false;
    }

    delete_draft(game_id, active_player, draft_code);
    set_current_draft(game_id, active_player, "");
    Logger::instance().info("Canceled draft: " + draft_code);
    Telemetry::getInstance().write("Canceled draft: " + draft_code);

    return true;
}

bool BuildCommand::invoke(void)
{
    // Delegate to StateMachine to handle build commit logic.
    // The Command's job is simply to set the draft code and trigger transition.
    StateMachine& sm = StateMachine::getInstance();
    sm.set_pending_build_draft(m_draft_code);
    return sm.transition();
}

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
        Telemetry::getInstance().write("Error: No current draft to commit");
        return false;
    }

    // Validate draft exists
    if (!draft_exists(game_id, active_player, draft_code))
    {
        Logger::instance().error("Draft not found: " + draft_code);
        Telemetry::getInstance().write("Error: Draft not found: " + draft_code);
        return false;
    }

    // Load draft
    DraftRow d = load_draft(game_id, active_player, draft_code);

    // Validate draft attributes
    if (d.attr.type == 'S' && d.attr.SR != 0)
    {
        Logger::instance().error("SystemShips cannot have SR");
        Telemetry::getInstance().write("Error: SystemShips cannot have SR");
        return false;
    }
    if (d.attr.M % 3 != 0)
    {
        Logger::instance().error("Missiles must be a multiple of 3");
        Telemetry::getInstance().write(
            "Error: Missiles must be a multiple of 3");
        return false;
    }
    if (d.attr.PD < 0 || d.attr.B < 0 || d.attr.S < 0 || d.attr.T < 0 ||
        d.attr.M < 0 || d.attr.SR < 0)
    {
        Logger::instance().error("Negative attribute");
        Telemetry::getInstance().write(
            "Error: Negative attribute values not allowed");
        return false;
    }

    // Calculate cost
    int cost = d.attr.PD + d.attr.B + d.attr.S + d.attr.T + d.attr.SR;
    cost += (d.attr.M + 2) / 3;
    if (d.attr.type == 'W')
        cost += 5; // Warp generator

    // Check BP availability
    int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;
    if (cost > bp)
    {
        Logger::instance().error("Insufficient CR. Need " +
                                 std::to_string(cost) + ", have " +
                                 std::to_string(bp));
        Telemetry::getInstance().write("Error: Insufficient CR. Need " +
                                       std::to_string(cost) + ", have " +
                                       std::to_string(bp));
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

    std::ostringstream bcmsg;
    bcmsg
        << "Committed: "
        << sh.name << " - " << sh.code << " (Tech Level "
        << std::to_string(sh.attr.tech) << ")\n"
        << "Cost: "
        << std::to_string(cost) << " BP, Remaining: " << std::to_string(bp)
        << " BP";

    Telemetry::getInstance().write(bcmsg.str());
    return true;
}

bool BuildListDraftsCommand::invoke(void)
{
    // begin: redundancy -- There is something redundant in all of this.
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    // end of redundancy

    std::vector<DraftRow> drafts = load_drafts(game_id, active_player);
    std::string current_draft = get_current_draft(game_id, active_player);

    if (drafts.empty())
    {
        Logger::instance().info("No drafts found");
        Telemetry::getInstance().write("No drafts found");
    }
    else
    {
        std::ostringstream msg;
        msg << "Draft ships (" << drafts.size() << "):\n";
        for (const auto& d : drafts)
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
        Telemetry::getInstance().write(msg.str());
    }

    return true;
}

bool BuildNewCommand::invoke(void)
{
    // Check if this command is allowed given current game state
    BuildNewParams_t params;
    params.ship_type = m_ship_code.empty() ? 'W' : m_ship_code[0];
    params.ship_name = m_ship_name;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::BUILD_NEW,
                                                    &params, inhibit_error))
    {
        Telemetry::getInstance().write("Error: " + inhibit_error);
        return false;
    }

    GameState s = StateMachine::getInstance().get_game_state();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Check BP availability
    int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;
    if (bp <= 0)
    {
        Logger::instance().error("No Build Points available");
        Telemetry::getInstance().write(
            "SHIPYARD: Insufficient Build Points. Construction halted.");
        return false;
    }

    // Validate ship code format
    if (m_ship_code.empty() || m_ship_code.length() > 10)
    {
        Logger::instance().error("Invalid ship code format");
        Telemetry::getInstance().write(
            "SHIPYARD: Invalid hull designation. Review ship code.");
        return false;
    }

    // Auto-assign ship number if not provided
    // Numbers must be unique across ALL players, not just the active player
    std::string ship_code = m_ship_code;
    if (ship_code.find_first_of("0123456789") == std::string::npos)
    {
        int next_num = 1;
        std::string candidate;
        do
        {
            candidate = ship_code + std::to_string(next_num);
            next_num++;
            // Check both players to ensure global uniqueness
        } while (ship_exists(s.game_id, 'A', candidate) ||
                 ship_exists(s.game_id, 'B', candidate) ||
                 draft_exists(s.game_id, 'A', candidate) ||
                 draft_exists(s.game_id, 'B', candidate));
        ship_code = candidate;
    }

    // Check for duplicates
    if (draft_exists(s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Draft already exists: " + ship_code);
        Telemetry::getInstance().write("SHIPYARD: Hull " + ship_code +
                                       " already on drafting board.");
        return false;
    }

    if (ship_exists(s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Ship already exists: " + ship_code);
        Telemetry::getInstance().write("SHIPYARD: Vessel " + ship_code +
                                       " already commissioned in fleet.");
        return false;
    }

    // Create draft
    DraftRow draft;
    draft.code = ship_code;
    draft.name = m_ship_name;
    draft.attr.type = 'W'; // Default to warship

    insert_draft(s.game_id, active_player, draft);
    set_current_draft(s.game_id, active_player, ship_code);

    Logger::instance().info("Draft created: " + m_ship_name + " - " +
                            ship_code);

    std::ostringstream bmes;
    bmes << "SHIPYARD: Hull "
         << ship_code
         << " laid down. Designation: "
         << m_ship_name;

    Telemetry::getInstance().write(bmes.str());
    return true;
}

bool BuildSetAttributeCommand::invoke(void)
{
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::string draft_code = get_current_draft(game_id, active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to modify");
        Telemetry::getInstance().write("Error: No current draft to modify");
        return false;
    }

    DraftRow d = load_draft(game_id, active_player, draft_code);

    // Apply attributes using C++17 structured bindings
    for (const auto& [attr_id, value] : m_attributes)
    {
        switch (attr_id)
        {
        case AttributeID::POWER_DRIVE:
            d.attr.PD = value;
            break;
        case AttributeID::BEAM:
            d.attr.B = value;
            break;
        case AttributeID::SCREEN:
            d.attr.S = value;
            break;
        case AttributeID::TUBE:
            d.attr.T = value;
            break;
        case AttributeID::MISSILE:
            d.attr.M = value;
            break;
        case AttributeID::SYSTEM_RACK:
            d.attr.SR = value;
            break;
        }
    }

    update_draft_attrs(game_id, active_player, draft_code, d);

    std::ostringstream msg;
    msg << "Draft updated: " << draft_code << " [PD=" << d.attr.PD
        << ", B=" << d.attr.B << ", S=" << d.attr.S << ", T=" << d.attr.T
        << ", M=" << d.attr.M << ", SR=" << d.attr.SR << "]";
    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    return true;
}

bool BuildShowDraftCommand::invoke(void)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    GameState s = StateMachine::getInstance().get_game_state();
    int m_game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    if (!draft_exists(m_game_id, active_player, m_draft_code))
    {
        Logger::instance().error("Draft not found: " + m_draft_code);
        Telemetry::getInstance().write("Error: Draft not found: " +
                                       m_draft_code);
        return false;
    }

    DraftRow d = load_draft(m_game_id, active_player, m_draft_code);
    std::ostringstream msg;
    msg << "Draft: " << d.name << " - " << d.code << "\n"
        << "  Type: " << d.attr.type << "\n"
        << "  PD=" << d.attr.PD << ", B=" << d.attr.B << ", S=" << d.attr.S
        << ", T=" << d.attr.T << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
    Logger::instance().info(msg.str());
    Telemetry::getInstance().write(msg.str());

    set_current_draft(m_game_id, active_player, m_draft_code);

    return true;
}
