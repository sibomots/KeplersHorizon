///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "system_strategy.h"

#include <algorithm>
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
    out << "=== SYSTEM: " << system_name << " ===\n";
    out << "Knowledge Level: " << level << "\n\n";

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
            out << "Planets: " << pcount[0][0] << "\n";
        }

        // Belt count
        std::string bc =
            "SELECT COUNT(*) FROM system_asteroid_belts WHERE system_name=?";
        auto bcount = db.Query(bc, {system_name});

        if (!bcount.empty() && bcount[0][0] != "0")
        {
            out << "Asteroid Belts: " << bcount[0][0] << "\n";
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
        out << "\nUse 'system " << system_name
            << " planets' for planetary data.\n";
        out << "Use 'system " << system_name
            << " facilities' for infrastructure.\n";
    }
    if (rank >= 3)
    {
        out << "Use 'system " << system_name << " resources' for deposits.\n";
        out << "Use 'system " << system_name
            << " population' for inhabitants.\n";
    }
    if (rank >= 4)
    {
        out << "Use 'system " << system_name
            << " anomalies' for discoveries.\n";
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
            "SYSTEM: Planetary data requires Charted knowledge level.\n"
            "Current: " +
            level + ". Send a ship to survey.");
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
        out << "No planetary bodies on record.\n";
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
            "SYSTEM: Resource data requires Surveyed knowledge level.\n"
            "Current: " +
            level + ". Use 'survey' command.");
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

bool SystemStrategy::show_populations(const std::string& system_name)
{
    // BUGBUG this ought to be std::pair or something.. better.
    std::string level = get_knowledge_level(system_name);
    int rank = knowledge_rank(level);

    // BUGBUG move to invoke to limit call into this.
    if (rank < 3)
    {
        Telemetry::instance().write(
            "SYSTEM: Population data requires Surveyed knowledge level.\n"
            "Current: " +
            level + ". Use 'survey' command.");
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== " << system_name << " POPULATIONS ===\n\n";

    std::string q =
        "SELECT p.common_name, p.designation, sp.name, pop.pop_class, "
        " pop.population_millions, pop.tech_level, pop.disposition "
        " FROM system_populations pop "
        " JOIN system_planets p ON pop.location_type='Planet' AND "
        " pop.location_id=p.id "
        " JOIN system_species sp ON pop.species_id=sp.id "
        " WHERE p.system_name=? ORDER BY pop.population_millions DESC";
    auto rows = db.Query(q, {system_name});

    if (rows.empty())
    {
        out << "No significant population centers on record.\n";
    }
    else
    {
        out << "World            Species         Class       Pop(M)  Tech  "
               "Disposition\n";
        out << "---------------  --------------  ----------  ------  ----  "
               "-----------\n";
        for (const auto& r : rows)
        {
            std::string loc = r[0].empty() ? r[1] : r[0];
            if (loc.size() > 15)
            {
                loc = loc.substr(0, 13) + "..";
            }
            out << loc << std::string(17 - loc.size(), ' ');

            std::string species = r[2];
            if (species.size() > 14)
            {
                species = species.substr(0, 12) + "..";
            }
            out << species << std::string(16 - species.size(), ' ');

            out << r[3] << std::string(12 - r[3].size(), ' ');
            out << r[4] << std::string(8 - r[4].size(), ' ');
            out << r[5] << std::string(6 - r[5].size(), ' ');
            out << r[6] << "\n";
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
            "SYSTEM: Facility data requires Charted knowledge level.\n"
            "Current: " +
            level + ". Send a ship to survey.");
        return false;
    }

    DatabaseManager& db = DatabaseManager::instance();
    std::ostringstream out;
    out << "=== " << system_name << " FACILITIES ===\n\n";

    std::string q =
        "SELECT p.common_name, p.designation, f.facility_type, f.name, "
        " f.capacity, f.owner, f.operational "
        " FROM system_facilities f "
        " JOIN system_planets p ON f.location_type='Planet' AND "
        " f.location_id=p.id "
        " WHERE p.system_name=? ORDER BY f.facility_type";
    auto rows = db.Query(q, {system_name});

    if (rows.empty())
    {
        out << "No significant infrastructure on record.\n";
    }
    else
    {
        out << "Location         Type           Name                  Cap  "
               "Owner   Op\n";
        out << "---------------  -------------  --------------------  ---  "
               "------  --\n";
        for (const auto& r : rows)
        {
            std::string loc = r[0].empty() ? r[1] : r[0];
            if (loc.size() > 15)
            {
                loc = loc.substr(0, 13) + "..";
            }
            out << loc << std::string(17 - loc.size(), ' ');

            out << r[2] << std::string(15 - r[2].size(), ' ');

            std::string fname = r[3];
            if (fname.size() > 20) 
            {
                fname = fname.substr(0, 18) + "..";
            }
            out << fname << std::string(22 - fname.size(), ' ');

            out << r[4] << std::string(5 - r[4].size(), ' ');
            out << r[5] << std::string(8 - r[5].size(), ' ');
            out << ((KH_EQU(r[6], "1")) ? "Y" : "N") << "\n";
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
            "SYSTEM: Anomaly data requires Intimate knowledge level.\n"
            "Current: " +
            level + ". Extended presence required.");
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
