///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "system_strategy.h"

#include <algorithm>
#include <format>
#include <sstream>

#include "db.h"
#include "statemachine.h"
#include "system_modes.h"
#include "telemetry.h"

// Convert knowledge level string to numeric rank for comparison
int SystemStrategy::knowledge_rank(const std::string& level)
{
    // BUGBUG FIX THIS to be more flexible and not based on string keys
    if (KH_EQU(level, "Intimate"))
    {
        return 4;
    }
    else if (KH_EQU(level, "Surveyed"))
    {
        return 3;
    }
    else if (KH_EQU(level, "Charted"))
    {
        return 2;
    }
    else if (KH_EQU(level, "Rumored"))
    {
        return 1;
    }
    else {
        return 0; // Unknown
    }
}

// BUGBUG accept game id and owner as arguments.
// Get player's current knowledge level for this system
std::string SystemStrategy::get_knowledge_level(const std::string& system_name)
{
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    DatabaseManager& db = DatabaseManager::instance();

    std::string q = "SELECT knowledge_level FROM codex_entries "
                    " WHERE game_id=?  AND player=? AND system_name=?";
    auto rows = db.Query(q, {s.game_id, owner, system_name});

    if (rows.empty())
    {
        return "Unknown";
    }
    return rows[0][0];
}

bool SystemStrategy::show_overview(const std::string& system_name)
{
    DatabaseManager& db = DatabaseManager::instance();

    // BUGBUG use pair ??
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    std::ostringstream out;
    out << "=== "
        << std::format(LC_MILIEU_SYSTEM_SHOW_BANNER, system_name)
        << " ===\n"
        << std::format(LC_MILIEU_SYSTEM_SHOW_INFO_LEVEL, level)
        << "\n";

    // Always show star info if at least Rumored
    if (rank >= 1)
    {
        std::string sq =
            "SELECT designation, star_class, luminosity, color, notes "
            " FROM system_stars WHERE system_name=?";
        auto stars = db.Query(sq, {system_name});

        if (!stars.empty())
        {
            out << "STAR";
            if (stars.size() > 1)
            {
                out << "S";
            }
            out << ":\n";
            for (const auto& s : stars)
            {
                out << "  " << s[0] << " - Class " << s[1] << " " << s[2];
                if (!s[3].empty())
                {
                    out << " (" << s[3] << ")";
                }
                out << "\n";
            }
            out << "\n";
        }

        // Planet count
        std::string pq =
            "SELECT COUNT(*) FROM system_planets WHERE system_name=?";
        auto pcount = db.Query(pq, {system_name});

        if (!pcount.empty())
        {
            out << std::format(LC_MILIEU_SYSTEM_SHOW_PLANET, pcount[0][0])
                << "\n";
        }

        // Belt count
        std::string bc =
            "SELECT COUNT(*) FROM system_asteroid_belts WHERE system_name=?";
        auto bcount = db.Query(bc, {system_name});

        if (!bcount.empty() && bcount[0][0] != "0")
        {
            out << std::format(LC_MILIEU_SYSTEM_SHOW_ASTEROID, bcount[0][0])
                << "\n";
        }
    }

    std::string q =
        "SELECT rumor_text FROM system_codex_rumors WHERE system_name=?";
    auto rumors = db.Query(q, {system_name});

    if (!rumors.empty())
    {
        out << "\nRUMORS:\n";
        out << "  \"" << rumors[0][0] << "\"\n";
    }

    // Hints for more info
    if (rank >= 2)
    {
        out << "\n"
            << std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_PLANET_HINT, system_name)
            << "\n"
            << std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_FACILITIES_HINT, system_name)
            << "\n";
    }
    if (rank >= 3)
    {
        out << std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_RESOURCES_HINT, system_name)
            << "\n";
    }
    if (rank >= 4)
    {
        out << std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_ANOMALIES_HINT, system_name)
            << "\n";
    }

    Telemetry::instance().write(out.str());
    return true;
}

bool SystemStrategy::show_planets(const std::string& system_name)
{
    // BUGBUG use std::pair ??
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    // BUGBUG this ought to be moved into invoke, to limit depth
    // of call into the command/actor
    if (rank < 2)
    {
        Telemetry::instance().write(
            std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN, level));
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== " << system_name << " PLANETS ===\n\n";

    std::string q =
        "SELECT orbital_position, designation, common_name, planet_type, "
        " atmosphere, habitability, notes FROM system_planets "
        " WHERE system_name=? ORDER BY orbital_position";
    auto rows = db.Query(q, {system_name});

    if (rows.empty())
    {
        out << LC_MILIEU_SYSTEM_NO_PLANET_RECORDS << "\n";
    }
    else
    {
        out << "#   Designation       Type       Atmo      Habit\n";
        out << "--  ---------------  ---------  --------  -----------\n";
        for (const auto& r : rows)
        {
            std::string name = r[2].empty() ? r[1] : r[2];
            out << r[0] << std::string(4 - r[0].size(), ' ');
            out << name << std::string(17 - name.size(), ' ');
            out << r[3] << std::string(11 - r[3].size(), ' ');
            out << r[4] << std::string(10 - r[4].size(), ' ');
            out << r[5] << "\n";
        }
    }

    Telemetry::instance().write(out.str());
    return true;
}

bool SystemStrategy::show_resources(const std::string& system_name)
{
    // BUGBUG make this std::pair or something else.. This is Ugly.
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    // BUGBUG this ought to be moved into invoke, to limit depth
    // of call into the command/actor
    if (rank < 3)
    {
        Telemetry::instance().write(
            std::format(LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN_SURVEY, level));
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== " << system_name << " RESOURCES ===\n\n";

    // Get resources from planets
    std::string pq =
        "SELECT p.common_name, p.designation, r.resource_type, r.abundance, "
        " r.extraction_difficulty "
        " FROM system_resources r "
        " JOIN system_planets p ON r.location_type='Planet' AND "
        " r.location_id=p.id "
        " WHERE p.system_name=? ORDER BY r.resource_type";

    auto rows = db.Query(pq, {system_name});

    // Get resources from belts
    std::string bq =
        "SELECT b.designation, r.resource_type, r.abundance, "
        " r.extraction_difficulty "
        " FROM system_resources r "
        " JOIN system_asteroid_belts b ON r.location_type='Belt' AND "
        " r.location_id=b.id "
        " WHERE b.system_name=?  ORDER BY r.resource_type";

    auto belt_rows = db.Query(bq, {system_name});

    if (rows.empty() && belt_rows.empty())
    {
        out << "No significant resource deposits on record.\n";
    }
    else
    {
        out << "Location          Resource      Abundance   Difficulty\n";
        out << "----------------  ------------  ----------  ----------\n";

        for (const auto& r : rows)
        {
            std::string loc = r[0].empty() ? r[1] : r[0];
            if (loc.size() > 16)
            {
                loc = loc.substr(0, 14) + "..";
            }
            out << loc << std::string(18 - loc.size(), ' ');
            out << r[2] << std::string(14 - r[2].size(), ' ');
            out << r[3] << std::string(12 - r[3].size(), ' ');
            out << r[4] << "\n";
        }

        for (const auto& r : belt_rows)
        {
            std::string loc = r[0];
            if (loc.size() > 16)
                loc = loc.substr(0, 14) + "..";
            out << loc << std::string(18 - loc.size(), ' ');
            out << r[1] << std::string(14 - r[1].size(), ' ');
            out << r[2] << std::string(12 - r[2].size(), ' ');
            out << r[3] << "\n";
        }
    }

    Telemetry::instance().write(out.str());
    return true;
}

bool SystemStrategy::show_facilities(const std::string& system_name)
{
    // BUGBUG ugly
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    // BUGBUG - move into invoke!
    if (rank < 2)
    {
        Telemetry::instance().write(
         std::format(LC_MILIEU_SYSTEM_FACILITIES_DATA_TARGET_INFO_MIN_CHARTED,
              level));
        return false;
    }

    GameState s = StateMachine::instance().get_game_state();
    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== "
        << std::format(LC_MILIEU_SYSTEM_SHOW_FACILITIES_BANNER, system_name)
        << " ===\n\n";

    // Join static facility data with live controller from facility_control
    std::string q =
        "SELECT p.common_name, p.designation, f.facility_type, f.name, "
        " fc.controller "
        " FROM system_facilities f "
        " JOIN system_planets p ON f.location_type='Planet' AND "
        " f.location_id=p.id "
        " LEFT JOIN facility_control fc ON fc.game_id=? "
        " AND fc.system_name=? AND fc.facility_type=f.facility_type "
        " WHERE p.system_name=? ORDER BY f.facility_type";
    auto rows = db.Query(q, {s.game_id, system_name, system_name});

    if (rows.empty())
    {
        out << LC_MILIEU_SYSTEM_NO_INFRA_RECORDS << "\n";
    }
    else
    {
        char me = StateMachine::instance().get_current_player();

        out << std::format("{:<17}{:<15}{:<22}{}\n",
                           "Location", "Type", "Name", "Controller");
        out << std::format("{:<17}{:<15}{:<22}{}\n",
                           "---------------", "-------------",
                           "--------------------", "----------");
        for (const auto& r : rows)
        {
            std::string loc = r[0].empty() ? r[1] : r[0];
            if (loc.size() > 15)
            {
                loc = loc.substr(0, 13) + "..";
            }

            std::string fname = r[3];
            if (fname.size() > 20)
            {
                fname = fname.substr(0, 18) + "..";
            }

            std::string ctrl;
            if (r[4].empty())
            {
                ctrl = "Neutral";
            }
            else if (KH_EQU(r[4][0], me))
            {
                ctrl = "You";
            }
            else
            {
                ctrl = "Enemy";
            }

            out << std::format("{:<17}{:<15}{:<22}{}\n",
                               loc, r[2], fname, ctrl);
        }
    }

    Telemetry::instance().write(out.str());
    return true;
}

bool SystemStrategy::show_anomalies(const std::string& system_name)
{
    // BUGBUG ugly..
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    // BUGBUG move into invoke
    if (rank < 4)
    {
        Telemetry::instance().write(
         std::format(LC_MILIEU_SYSTEM_ANOMALIES_DATA_TARGET_INFO_MIN_INTIMATE,
              level));
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== " << system_name << " ANOMALIES ===\n\n";
    std::string q = "SELECT name, anomaly_type, effect, discovery_text "
                    " FROM system_anomalies WHERE system_name=?";
    auto rows = db.Query(q, {system_name});

    if (rows.empty())
    {
        out << "No anomalies discovered in this system.\n";
    }
    else
    {
        for (const auto& r : rows)
        {
            out << "** " << r[0] << " ** [" << r[1] << "]\n";
            if (!r[3].empty())
            {
                out << "  " << r[3] << "\n";
            }
            if (!r[2].empty())
            {
                out << "  Effect: " << r[2] << "\n";
            }
            out << "\n";
        }
    }

    Telemetry::instance().write(out.str());
    return true;
}
