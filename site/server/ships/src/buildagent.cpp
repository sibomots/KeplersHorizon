//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include <sstream>

#include "buildagent.h"
#include "db.h"
#include "logger.h"
#include "ships.h"
#include "statemachine.h"
#include "telemetry.h"

// Main dispatch apply - routes to specific apply methods
bool BuildAgent::apply(BuildAgentParam& param)
{
    return std::visit(
        [this](auto&& arg) -> bool
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, BuildNewParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildSetParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildCommitParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildDraftsParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildCancelParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildShowDraftParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, BuildParam>)
            {
                // Base param - shouldn't happen
                return false;
            }
            return false;
        },
        param);
}

// BuildNew - Create a new ship draft
bool BuildAgent::apply(BuildNewParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string ship_code = param.get_ship_code();
    std::string ship_name = param.get_ship_name();
    char ship_type = param.get_ship_type();

    GameState s = StateMachine::getInstance().get_game_state();

    // Check BP availability
    int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;
    if (bp <= 0)
    {
        Telemetry::getInstance().write(
            "SHIPYARD: Insufficient Build Points. Construction halted.");
        return false;
    }

    // Validate ship code format
    if (ship_code.empty() || ship_name.empty())
    {
        Telemetry::getInstance().write(
            "SHIPYARD: Invalid hull designation. Review ship code.");
        return false;
    }

    // Auto-assign ship number if not provided
    // Numbers must be unique across ALL players
    int did = 0;
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
            A_has_draft = get_draft_by_spec(did, game_id, 'A', candidate_code);
            B_has_draft = get_draft_by_spec(did, game_id, 'B', candidate_code);
            A_has_ship = ship_exists(game_id, 'A', candidate_code);
            B_has_ship = ship_exists(game_id, 'B', candidate_code);
        } while (A_has_draft || B_has_draft || A_has_ship || B_has_ship);

        ship_code = candidate_code;
    }

    // Check for duplicates
    if (get_draft_by_spec(did, game_id, owner, ship_code))
    {
        Telemetry::getInstance().write("SHIPYARD: Hull " + ship_code +
                                       " already on drafting board.");
        return false;
    }

    if (ship_exists(game_id, owner, ship_code))
    {
        Telemetry::getInstance().write("SHIPYARD: Vessel " + ship_code +
                                       " already commissioned in fleet.");
        return false;
    }

    // Create draft
    DraftRow draft;
    draft.set_code(ship_code);
    draft.set_name(ship_name);
    draft.set_type(ship_type);
    draft.update_cost();

    insert_draft(game_id, owner, draft);

    std::ostringstream msg;
    msg << "SHIPYARD: Hull " << ship_code
        << " laid down. Designation: " << ship_name;
    Telemetry::getInstance().write(msg.str());

    return true;
}

// BuildSet - Set attributes on a draft
bool BuildAgent::apply(BuildSetParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();
    AttributeMap attributes = param.get_attributes();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::getInstance().write("SHIPYARD: Ship " + target +
                                           " not in space dock.");
        }
        return false;
    }

    DraftRow drow;
    bool found_ship_draft =
        load_ship_draft_by_spec(drow, did, game_id, owner, target);

    if (!found_ship_draft)
    {
        Telemetry::getInstance().write("SHIPYARD: Ship " + target +
                                       " is LOST from space dock.");
        return false;
    }

    // Apply attributes using AttributeMap
    for (const auto& [attr_id, value] : attributes.data)
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
        case AttributeID::UNKNOWN:
            break;
        }
    }

    update_draft_attrs(did, game_id, owner, target, drow);
    drow.update_cost();

    std::ostringstream os;
    append_draft_header(os);
    append_draft_row(os, drow);
    Telemetry::getInstance().write(os.str());

    return true;
}

// BuildCommit - Commit a draft to become a ship
bool BuildAgent::apply(BuildCommitParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();

    GameState s = StateMachine::getInstance().get_game_state();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Ship with hull designation " + target +
                " not in space dock.");
        }
        return false;
    }

    // Load draft
    DraftRow drow;
    bool found_draft_row =
        load_ship_draft_by_spec(drow, did, game_id, owner, target);

    if (!found_draft_row)
    {
        Telemetry::getInstance().write("SHIPYARD: Ship " + target +
                                       " is LOST from space dock.");
        return false;
    }

    // Validate draft
    std::vector<std::string> report;
    bool is_valid_ship_candidate = test_ship_draft_candidate(drow, report);

    if (!is_valid_ship_candidate)
    {
        // Report validation errors
        for (const auto& msg : report)
        {
            Telemetry::getInstance().write(msg);
        }
        return false;
    }

    // Update cost and check BP availability
    drow.update_cost();
    int candidate_cost = drow.get_cost();

    int& bp = (s.active_player == "A") ? s.creditsA : s.creditsB;

    if (candidate_cost > bp)
    {
        Telemetry::getInstance().write("SHIPYARD: Insufficient CR. Need " +
                                       std::to_string(candidate_cost) +
                                       ", have " + std::to_string(bp));
        return false;
    }

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
    ShipRow ship = ShipRow::from_draft(drow, s.round, s.active_player);

    // Commit to DB
    insert_ship(game_id, owner, ship);
    delete_draft(did, game_id, owner, drow.code);

    // Deduct BP and save
    bp -= candidate_cost;
    StateMachine::getInstance().save_game(s);

    std::ostringstream msg;
    msg << "Committed: " << ship << "\n"
        << "Remaining BP: " << std::to_string(bp) << " BP";
    Telemetry::getInstance().write(msg.str());

    return true;
}

// BuildDrafts - List all drafts for player
bool BuildAgent::apply(BuildDraftsParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();

    std::vector<DraftRow> drafts = load_drafts_by_owner(game_id, owner);

    if (drafts.empty())
    {
        Telemetry::getInstance().write("SHIPYARD: No ships in production");
        return false;
    }

    std::ostringstream oss;
    build_drafts_report(oss, drafts);
    Telemetry::getInstance().write(oss.str());

    return true;
}

bool BuildAgent::apply(BuildShowDraftParam& param)
{
    bool result = false;
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();

    GameState s = StateMachine::getInstance().get_game_state();
    DatabaseManager& db = DatabaseManager::getInstance();
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::getInstance().write(
                "SHIPYARD: Ship with hull designation " + target +
                " not in space dock.");
        }
        return false;
    }
    else
    {
        // Load draft
        DraftRow drow;
        bool found_draft = load_ship_draft_by_spec(drow, did, game_id,
                                                   active_player, target);
        if (found_draft)
        {
            std::ostringstream os;
            drow.update_cost();
            append_draft_header(os);
            append_draft_row(os, drow);
            Telemetry::getInstance().write(os.str());
            result = true;
        }
        else
        {
            Telemetry::getInstance().write("SHIPYARD: Ship " + target +
                                           " is LOST from space dock.");
            result = false;
        }
    }
    return result;
}

// BuildCancel - Cancel a draft
bool BuildAgent::apply(BuildCancelParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        Telemetry::getInstance().write(
            "SHIPYARD: No ship with hull designation " + target +
            " in the shipyard.");
        return false;
    }

    // Delete the draft
    delete_draft(did, game_id, owner, target);

    Telemetry::getInstance().write("SHIPYARD: Canceled: " + target);

    return true;
}
