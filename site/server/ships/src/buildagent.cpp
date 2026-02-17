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
        case AttributeID::PHASIC:
            drow.set_Phasic(value);
            break;
        case AttributeID::SHIELD:
            drow.set_Shield(value);
            break;
        case AttributeID::LAUNCHER:
            drow.set_Launcher(value);
            break;
        case AttributeID::TORPEDO:
            drow.set_Torpedo(value);
            break;
        case AttributeID::HANGAR:
            drow.set_Hanger(value);
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
    // Select both current and max values for combat attributes
    std::string q =
        "SELECT s.ship_code, s.ship_name, s.at_hex, s.racked_in, "
        " s.pd, s.pd_max, s.phasic, s.phasic_max, "
        " s.shield, s.shield_max, s.launcher, s.launcher_max, "
        " s.torpedoes, s.torpedoes_max, s.tech_level, s.lrs, s.hangar, s.hangar_max, "
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
        // Column indices:
        //  0=ship_code  1=ship_name  2=at_hex  3=racked_in
        //  4=pd  5=pd_max  6=phasic  7=phasic_max
        //  8=shield  9=shield_max  10=launcher  11=launcher_max
        //  12=torpedoes  13=torpedoes_max  14=tech_level  15=lrs
        //  16=hangar  17=hangar_max  18=ss.name
        // Column order: HULL, DESIGNATION, SECTOR, TECH LV, PD, P, S, L, T, H, LRS
        std::ostringstream out;
        out << "FLEET REGISTRY [" << rows.size() << " vessels operational]\n";
        out << std::format("{:<6}{:<16}{:<16} {:>4} {:>5} {:>5} {:>5} "
                           "{:>5} {:>5} {:>5} {:>3}\n",
                           "HULL", "DESIGNATION", "SECTOR", "TECH",
                           "PD", "P", "S", "L",
                           "T", "H", "LRS");
        out << std::format("{:<6}{:<16}{:<16} {:>4} {:>5} {:>5} {:>5} "
                           "{:>5} {:>5} {:>5} {:>3}\n",
                           "----", "--------------", "--------------",
                           "----", "-----", "-----", "-----",
                           "-----", "-----", "-----", "---");
        for (const auto& r : rows)
        {
            std::string hull = r[0];
            std::transform(hull.begin(), hull.end(), hull.begin(), ::toupper);

            std::string loc;
            if (!r[3].empty())
            {
                loc = "in " + r[3];
            }
            else if (!r[18].empty())
            {
                loc = r[18] + " (" + r[2] + ")";
            }
            else
            {
                loc = r[2];
            }

            // Format attribute as "cur/max" when damaged, or just "cur" when full
            auto fmtAttr = [](const std::string& cur, const std::string& max)
            {
                if (cur == max || max == "0")
                {
                    return cur;
                }
                return cur + "/" + max;
            };

            std::string sPD = fmtAttr(r[4], r[5]);
            std::string sP  = fmtAttr(r[6], r[7]);
            std::string sS  = fmtAttr(r[8], r[9]);
            std::string sL  = fmtAttr(r[10], r[11]);
            std::string sT  = fmtAttr(r[12], r[13]);
            std::string sH = fmtAttr(r[16], r[17]);

            out << std::format("{:<6}{:<16}{:<16} {:>4} {:>5} {:>5} {:>5} "
                               "{:>5} {:>5} {:>5} {:>3}\n",
                               hull, r[1].substr(0, 14), loc.substr(0, 18),
                               r[14], sPD, sP, sS, sL,
                               sT, sH, r[15]);
        }

        Telemetry::instance().write(out.str());
    }
    return true;
}
