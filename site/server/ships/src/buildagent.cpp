///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "buildagent.h"

#include <sstream>

#include "db.h"
#include "logger.h"
#include "shipmgr.h"
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
            else if constexpr (std::is_same_v<T, BuildFleetListParam>)
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
    ShipManager& shipmgr = ShipManager::instance();
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string ship_code = param.get_ship_code();
    std::string ship_name = param.get_ship_name();
    char ship_type = param.get_ship_type();

    GameState s = StateMachine::instance().get_game_state();

    // Check BP availability
    int& bp = (KH_EQU(s.active_player, "A")) ? s.creditsA : s.creditsB;
    if (bp <= 0)
    {
        Telemetry::instance().write(
            "SHIPYARD: Insufficient Build Points. Construction halted.");
        return false;
    }

    // Validate ship code format
    if (ship_code.empty() || ship_name.empty())
    {
        Telemetry::instance().write(
            "SHIPYARD: Invalid hull designation. Review ship code.");
        return false;
    }

    // Auto-assign ship number if not provided
    // Numbers must be unique across ALL players
    int did = 0;
    if (KH_EQU(ship_code.find_first_of("0123456789"), std::string::npos))
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
            A_has_ship = shipmgr.ship_code_taken(game_id, 'A', candidate_code);
            B_has_ship = shipmgr.ship_code_taken(game_id, 'B', candidate_code);
        } while (A_has_draft || B_has_draft || A_has_ship || B_has_ship);

        ship_code = candidate_code;
    }

    // Check for duplicates
    if (get_draft_by_spec(did, game_id, owner, ship_code))
    {
        Telemetry::instance().write("SHIPYARD: Hull " + ship_code +
                                    " already on drafting board.");
        return false;
    }

    if (shipmgr.ship_code_taken(game_id, owner, ship_code))
    {
        Telemetry::instance().write("SHIPYARD: Vessel " + ship_code +
                                    " already commissioned in fleet.");
        return false;
    }

    // Create draft
    DraftRow draft;
    draft.set_code(ship_code);
    draft.set_name(ship_name);
    draft.set_type(ship_type);
    draft.update_cost();

    shipmgr.insert_draft(game_id, owner, draft);

    std::ostringstream msg;
    msg << "SHIPYARD: Hull " << ship_code
        << " laid down. Designation: " << ship_name;
    Telemetry::instance().write(msg.str());

    return true;
}

// BuildSet - Set attributes on a draft
bool BuildAgent::apply(BuildSetParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();
    AttributeMap attributes = param.get_attributes();
    ShipManager& shipmgr = ShipManager::instance();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::instance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::instance().write("SHIPYARD: Ship " + target +
                                        " does not exist");
        }
        return false;
    }

    DraftRow drow;
    bool found_ship_draft =
        shipmgr.load_ship_draft_by_spec(drow, did, game_id, owner, target);

    if (!found_ship_draft)
    {
        Telemetry::instance().write("SHIPYARD: Ship " + target +
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

    shipmgr.update_draft_attrs(did, game_id, owner, target, drow);
    drow.update_cost();

    std::ostringstream os;
    shipmgr.append_draft_header(os);
    shipmgr.append_draft_row(os, drow);
    Telemetry::instance().write(os.str());

    return true;
}

// BuildCommit - Commit a draft to become a ship
bool BuildAgent::apply(BuildCommitParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();
    ShipManager& shipmgr = ShipManager::instance();

    GameState s = StateMachine::instance().get_game_state();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::instance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::instance().write(
                "SHIPYARD: Ship with hull designation " + target +
                " not in space dock.");
        }
        return false;
    }

    // Load draft
    DraftRow drow;
    bool found_draft_row =
        shipmgr.load_ship_draft_by_spec(drow, did, game_id, owner, target);

    if (!found_draft_row)
    {
        Telemetry::instance().write("SHIPYARD: Ship " + target +
                                    " is LOST from space dock.");
        return false;
    }

    // Validate draft
    std::vector<std::string> report;
    bool is_valid_ship_candidate = shipmgr.is_ship_draft_valid(drow, report);

    if (!is_valid_ship_candidate)
    {
        // Report validation errors
        for (const auto& msg : report)
        {
            Telemetry::instance().write(msg);
        }
        return false;
    }

    // Update cost and check BP availability
    drow.update_cost();
    int candidate_cost = drow.get_cost();

    int& bp = (KH_EQU(s.active_player, "A")) ? s.creditsA : s.creditsB;

    if (candidate_cost > bp)
    {
        Telemetry::instance().write("SHIPYARD: Insufficient CR. Need " +
                                    std::to_string(candidate_cost) + ", have " +
                                    std::to_string(bp));
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
    bool inserted = shipmgr.insert_ship(game_id, owner, ship);
    if (!inserted)
    {
        Telemetry::instance().write(
            "SHIPYARD: Failed to commission " + ship.code +
            ". Hull code conflict.");
        return false;
    }
    shipmgr.delete_draft(did, game_id, owner, drow.code);

    // Deduct BP and save
    bp -= candidate_cost;
    StateMachine::instance().save_game(s);

    std::ostringstream msg;
    msg << "Committed: " << ship << "\n"
        << "Remaining BP: " << std::to_string(bp) << " BP";
    Telemetry::instance().write(msg.str());

    return true;
}

// BuildDrafts - List all drafts for player
bool BuildAgent::apply(BuildDraftsParam& param)
{
    int game_id = param.get_game_id();
    char owner = param.get_player();
    ShipManager& shipmgr = ShipManager::instance();

    std::vector<DraftRow> drafts;
    bool has_drafts = shipmgr.load_drafts_by_owner(drafts, game_id, owner);
    if (!has_drafts)
    {
        Telemetry::instance().write("SHIPYARD: No ships in production");
        return false;
    }

    std::ostringstream oss;
    shipmgr.build_drafts_report(oss, drafts);
    Telemetry::instance().write(oss.str());

    return true;
}

bool BuildAgent::apply(BuildShowDraftParam& param)
{
    bool result = false;
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target = param.get_target();

    GameState s = StateMachine::instance().get_game_state();
    DatabaseManager& db = DatabaseManager::instance();
    ShipManager& shipmgr = ShipManager::instance();

    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        if (target.empty())
        {
            Telemetry::instance().write(
                "SHIPYARD: Need ship hull designator or name to find it");
        }
        else
        {
            Telemetry::instance().write(
                "SHIPYARD: Ship with hull designation " + target +
                " not in space dock.");
        }
        return false;
    }
    else
    {
        // Load draft
        DraftRow drow;
        bool found_draft = shipmgr.load_ship_draft_by_spec(
            drow, did, game_id, active_player, target);
        if (found_draft)
        {
            std::ostringstream os;
            drow.update_cost();
            shipmgr.append_draft_header(os);
            shipmgr.append_draft_row(os, drow);
            Telemetry::instance().write(os.str());
            result = true;
        }
        else
        {
            Telemetry::instance().write("SHIPYARD: Ship " + target +
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
    ShipManager& shipmgr = ShipManager::instance();

    int did = 0;
    bool has_draft = get_draft_by_spec(did, game_id, owner, target);

    if (!has_draft)
    {
        Telemetry::instance().write("SHIPYARD: No ship with hull designation " +
                                    target + " in the shipyard.");
        return false;
    }

    // Delete the draft
    shipmgr.delete_draft(did, game_id, owner, target);

    Telemetry::instance().write("SHIPYARD: Canceled: " + target);

    return true;
}

// private methods

bool BuildAgent::get_draft_by_spec(int& did, int gid, char owner,
                                   std::string target)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::instance();
    std::string q =
        "SELECT DISTINCT "
        "d.id, d.game_id, d.owner, d.ship_code, d.ship_name, d.ship_type "
        " FROM drafts d "
        " WHERE d.owner =? AND d.game_id=? AND ( d.ship_code=? OR "
        "d.ship_name=? ) ";

    auto rows = db.Query(q, {owner, gid, target, target});

    size_t sz = rows.size();
    if (KH_EQU(sz, 0))
    {
        // BIGBUG
        // we cannot find the draft.
    }
    else
    {
        // we found the draft(s)
        // BUGBUG how many??  Do we check?
        std::vector<std::string> r = rows[0];
        did = std::atoi(r[0].c_str());
        result = true;
    }
    return result;
}

bool BuildAgent::apply(BuildFleetListParam& param)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = param.get_player();
    int gid = param.get_game_id();
    DatabaseManager& db = DatabaseManager::instance();

    // Join with star_systems table to get star names for at_hex
    std::string q =
        "SELECT s.ship_code, s.ship_name, s.at_hex, s.racked_in, s.pd, s.beam, "
        " s.screen, s.tube, s.missiles, s.tech_level, s.lrs, s.tb, s.dr, "
        " ss.name "
        " FROM ships s "
        " LEFT JOIN star_systems ss ON s.at_hex = ss.hex_id AND ss.module_id = "
        " 1 "
        " WHERE s.game_id=? AND s.owner=? AND s.destroyed_at IS NULL ORDER BY "
        "s.ship_code";

    auto rows = db.Query(q, {gid, owner});

    if (rows.empty())
    {
        Telemetry::instance().write("FLEET OPS: No ships under your command.");
    }
    else
    {
        std::ostringstream out;
        out << "FLEET REGISTRY [" << rows.size() << " vessels operational]\n";
        out << "HULL  DESIGNATION      SECTOR                PD   B  S  T  M  "
               "LRS TB DR  TECH\n";
        out << "────  ──────────────  ───────────────────    ──  ── ── ── ──  "
               "─── ── ──  ────\n";
        for (const auto& r : rows)
        {
            // Uppercase the hull designator (ship_code)
            std::string hull = r[0];
            std::transform(hull.begin(), hull.end(), hull.begin(), ::toupper);

            // Format location: show star name with hex ID if available
            std::string loc;
            if (!r[3].empty())
            {
                // Racked in another ship
                loc = "in " + r[3];
            }
            else if (!r[13].empty())
            {
                // Have system name from join (index shifted due to lrs/tb/dr)
                loc = r[13] + " (" + r[2] + ")";
            }
            else
            {
                // Just hex ID
                loc = r[2];
            }

            // Use iomanip for proper formatting with right-justified numbers
            out << std::left << std::setw(6) << hull;
            out << std::left << std::setw(16) << r[1].substr(0, 14);
            out << std::left << std::setw(23) << loc.substr(0, 21);
            out << std::right << std::setw(2) << r[4];  // pd
            out << std::right << std::setw(4) << r[5];  // beam
            out << std::right << std::setw(3) << r[6];  // screen
            out << std::right << std::setw(3) << r[7];  // tube
            out << std::right << std::setw(3) << r[8];  // missiles
            out << std::right << std::setw(4) << r[10]; // lrs
            out << std::right << std::setw(3) << r[11]; // tb
            out << std::right << std::setw(3) << r[12]; // dr
            out << std::right << std::setw(6) << r[9];  // tech_level
            out << "\n";
        }

        Telemetry::instance().write(out.str());
    }
    return true;
}
