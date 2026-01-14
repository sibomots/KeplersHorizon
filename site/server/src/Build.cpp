//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "Build.h"

#include <cctype>
#include <sstream>

#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

bool BuildCancelCommand::invoke(void)
{
    bool result = false;
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = false;

    if (m_btargeted)
    {
        Logger::instance().info("[BUILD][Cancel] Locating ship " + m_target);
        has_draft = get_draft_by_spec(did, game_id, active_player, m_target);
    }
    else
    {
        // not targetted, so we could infer there is one only
        // or cancel the most recent?
        result = false;
    }

    if (!has_draft)
    {
        if (m_target.empty())
        {
           Telemetry::getInstance().write(
            "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
           Telemetry::getInstance().write("SHIPYARD: No ship with hull designation "
               + m_target + " in the shipyard.");
        }
        result = false;
    }
    else
    {
        delete_draft(did, game_id, active_player, m_target);
        // get ship code and name
        Telemetry::getInstance().write("SHIPYARD: Canceled: " + m_target);
        result = true;
    }
    return result;
}

bool BuildCommand::invoke(void)
{
    // jdw StateMachine& sm = StateMachine::getInstance();
    // jdw sm.set_pending_build_draft(m_draft_code);
    // jdw return true;

    // jdw What we want is the list of all  drafts??
    return true;
}

bool BuildCommitCommand::invoke(void)
{
    bool result = false;
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = false;

    if (m_btargeted)
    {
        Logger::instance().info("[BUILD][Commit] Locating ship " + m_target);
        has_draft = get_draft_by_spec(did, game_id, active_player, m_target);
    }

    if (!has_draft)
    {
        if (m_target.empty())
        {
           Telemetry::getInstance().write(
            "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
           Telemetry::getInstance().write(
               "SHIPYARD: Ship with hull designation " +
                 m_target + " not in space dock.");
        }
        result = false;
    }

    if (has_draft) 
    {
        // this ship exists in space dock, it is a draft.
        // Load draft
        DraftRow drow;

        bool found_draft_row = load_ship_draft_by_spec(drow, did, game_id,
                                                       active_player, m_target);

        if (found_draft_row)
        {
            bool is_valid_ship_candidate = false;
            std::vector<std::string> report;
            is_valid_ship_candidate = test_ship_draft_candidate(drow, report);

            int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;
            int candidate_cost = 0;

            bool complete = false;

            if (is_valid_ship_candidate)
            {
                drow.update_cost();
                candidate_cost = drow.get_cost();

                // Check BP availability
                int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;

                if (candidate_cost > bp)
                {
                    Telemetry::getInstance().write(
                        "SHIPYARD: Insufficient CR. Need " +
                        std::to_string(candidate_cost) + ", have " +
                        std::to_string(bp));
                }
                else
                {
                    complete = true;
                }
            }

            if (complete)
            {
                // Compute tech level
                if (s.round >= 1)
                {
                    drow.set_tech((s.round - 1) / 4);
                }
                else
                {
                    drow.set_tech(0);
                }

                // Create ship using factory method
                ShipRow ship =
                    ShipRow::from_draft(drow, s.round, s.active_player);

                // Commit to DB
                insert_ship(game_id, s.active_player[0], ship);
                delete_draft(did, game_id, s.active_player[0], drow.code);

                // Deduct BP and save
                // using a reference to the correct player's account
                bp -= candidate_cost;
                StateMachine::getInstance().save_game(s);

                std::ostringstream bcmsg;
                bcmsg << "Committed: " << ship << "\n"
                      << "Remaining BP: " << std::to_string(bp) << " BP";

                Telemetry::getInstance().write(bcmsg.str());
                result = true;
            }
        }
    }
    return result;
}

bool BuildListDraftsCommand::invoke(void)
{
    bool result = false;
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    std::vector<DraftRow> drafts = load_drafts_by_owner(game_id, active_player);

    if (drafts.empty())
    {
        Telemetry::getInstance().write("SHIPYARD: No ships in production");
    }
    else
    {
        std::ostringstream oss;
        build_draft_report(oss, drafts);
        Telemetry::getInstance().write(oss.str());
        result = true;
    }
    return result;
}

bool BuildNewCommand::invoke(void)
{
    bool result = false;
    // Check if this command is allowed given current game state
    BuildNewParams_t params;

    char code = 0;
    int did = 0;

    code = (char)m_ship_code[0];
    switch (code)
    {
    case 'W':
    case 'w':
        params.ship_type = 'W';
        break;
    case 'S':
    case 's':
        params.ship_type = 'S';
        break;
    default:
        // BUGBUG specify in rules the default
        params.ship_type = 'W';
    }

    params.ship_name = m_ship_name;

    std::string inhibit_error;
    if (!StateMachine::getInstance().check_inhibits(CommandID::BUILD_NEW,
                                                    &params, inhibit_error))
    {
        Telemetry::getInstance().write(inhibit_error);
        result = false;
    }
    else
    {
        GameState s = StateMachine::getInstance().get_game_state();
        //jdw fprintf(stderr, "Active Player: >%s<\n", s.active_player.c_str());

        char active_player =
            (s.active_player.empty() ? 'A' : s.active_player[0]);

        // Check BP availability
        int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;
        if (bp <= 0)
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Insufficient Build Points. Construction halted.");
            result = false;
        }
        // Validate ship code format
        else if (m_ship_code.empty() || m_ship_name.empty())
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Invalid hull designation. Review ship code.");
            result = false;
        }
        else
        {
            // Auto-assign ship number if not provided
            // Numbers must be unique across ALL players, not just the active
            // player
            std::string ship_code = m_ship_code;
            if (ship_code.find_first_of("0123456789") == std::string::npos)
            {
                int next_num = 1;
                std::string candidate_code;
                bool A_has_draft = false;
                bool B_has_draft = false;
                bool A_has_ship = false;
                bool B_has_ship = false;
                do
                {
                   candidate_code = ship_code + std::to_string(next_num);
                   next_num++;
                   // Check both players to ensure global uniqueness
                   A_has_draft = get_draft_by_spec(did,s.game_id,'A',candidate_code);
                   B_has_draft = get_draft_by_spec(did,s.game_id,'B',candidate_code);
                   A_has_ship = ship_exists(s.game_id, 'A', candidate_code);
                   B_has_ship = ship_exists(s.game_id, 'B', candidate_code);
                } while (A_has_draft || B_has_draft || A_has_ship || B_has_ship);

                ship_code = candidate_code;
            }

            // Check for duplicates
            if (get_draft_by_spec(did, s.game_id, active_player, ship_code))
            {
                Telemetry::getInstance().write("SHIPYARD: Hull " + ship_code +
                                               " already on drafting board.");
                result = false;
            }
            else if (ship_exists(s.game_id, active_player, ship_code))
            {
                Telemetry::getInstance().write(
                    "SHIPYARD: Vessel " + ship_code +
                    " already commissioned in fleet.");
                result = false;
            }
            else
            {

                // Create draft
                DraftRow draft;
                draft.set_code(ship_code);
                draft.set_name(m_ship_name);
                draft.set_type(params.ship_type);
                draft.update_cost();

                insert_draft(s.game_id, active_player, draft);
                std::ostringstream bmes;
                bmes << "SHIPYARD: Hull " << ship_code
                     << " laid down. Designation: " << m_ship_name;
                Telemetry::getInstance().write(bmes.str());
                result = true;
            }
        }
    }
    return result;
}

bool BuildSetAttributeCommand::invoke(void)
{
    bool result = false;
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = false;
    if (m_btargeted)
    {
        Logger::instance().info("[BUILD][SetAttr] Locating ship " + m_target);
        has_draft = get_draft_by_spec(did, game_id, active_player, m_target);
    }
    else
    {
        // not targetted, so we could infer there is one only
        // or cancel the most recent?
        result = false;
    }

    if (!has_draft)
    {
        if (m_target.empty())
        {
           Telemetry::getInstance().write(
               "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::getInstance().write("SHIPYARD: Ship " + m_target +
                                       " not in space dock.");
        }
        result = false;
    }
    else
    {
        DraftRow drow;

        bool found_ship_draft = load_ship_draft_by_spec(
            drow, did, game_id, active_player, m_target);

        if (found_ship_draft)
        {
            // Apply attributes using C++17 structured bindings
            for (const auto& [attr_id, value] : m_attributes)
            {
                switch (attr_id)
                {
                case AttributeID::POWER_DRIVE:
                    drow.set_PD(value);
                    break;
                case AttributeID::BEAM:
                    drow.set_B(value);
                    break;
                case AttributeID::SCREEN:
                    drow.set_S(value);
                    break;
                case AttributeID::TUBE:
                    drow.set_T(value);
                    break;
                case AttributeID::MISSILE:
                    drow.set_M(value);
                    break;
                case AttributeID::SYSTEM_RACK:
                    drow.set_SR(value);
                    break;
                }
            }

            update_draft_attrs(did, game_id, active_player, m_target, drow);
            drow.update_cost();
            std::ostringstream os;
            append_draft_header(os, 0);
            append_draft_row(os, drow);

            Telemetry::getInstance().write(os.str());
            result = true;
        }
    }
    return result;
}

bool BuildShowDraftCommand::invoke(void)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::getInstance();
    GameState s = StateMachine::getInstance().get_game_state();
    int game_id = StateMachine::getInstance().get_game_id();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = false;
    if (m_btargeted)
    {
        Logger::instance().info("[BUILD][ShowDraft] Locating ship " + m_target);
        has_draft = get_draft_by_spec(did, game_id, active_player, m_target);
    }
    else
    {
        // not targetted, so we could infer there is one only
        // or cancel the most recent?
        Telemetry::getInstance().write(
            "SHIPYARD: Need ship hull designator or name to find it");
        result = false;
    }

    // if (!draft_exists(game_id, active_player, m_target))
    if (!has_draft)
    {
        Telemetry::getInstance().write("SHIPYARD: Ship " + m_target +
                                       " is not in space dock.");
        result = false;
    }
    else
    {
        DraftRow drow;
        bool found_draft = load_ship_draft_by_spec(drow, did, game_id, 
              active_player, m_target);
        if (found_draft) { 
           std::ostringstream os;
           drow.update_cost();
           append_draft_header(os, 0);
           append_draft_row(os, drow);
           Telemetry::getInstance().write(os.str());
           // set_current_draft(game_id, active_player, m_target);
           result = true;
        }
        else
        {
           Telemetry::getInstance().write("SHIPYARD: Ship " + m_target +
                                          " is LOST from space dock.");
           result = false;
        }
    }
    return result;
}
