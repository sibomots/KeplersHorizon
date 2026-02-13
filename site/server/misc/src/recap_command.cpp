///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "recap_command.h"

#include <cstdlib>
#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "db.h"
#include "logger.h"
#include "statemachine.h"
#include "telemetry.h"

static const char* kClosingQuotes[] = {
    "\"The stars remember those who dared.\"",
    "\"Per aspera ad astra.\"",
    "\"Beyond the horizon, the light endures.\"",
    "\"What was charted can never be uncharted.\"",
    "\"The void yields its secrets to the persistent.\"",
    "\"All voyages end. Few are remembered.\"",
};
static const int kNumQuotes =
    sizeof(kClosingQuotes) / sizeof(kClosingQuotes[0]);

static const char* kDivider =
    "============================================================";
static const char* kSectionLine =
    "------------------------------------------------------------";

bool RecapCommand::invoke(void)
{
    GameState s = StateMachine::instance().get_game_state();

    if (!s.game_over)
    {
        Telemetry::instance().write("Game is still in progress.");
        return true;
    }

    int game_id = StateMachine::instance().get_game_id();
    char me = StateMachine::instance().get_current_player();

    std::ostringstream out;
    build_header(out, game_id, me);
    build_fleet_registry(out, game_id, me);
    build_combat_record(out, game_id, me);
    build_economic_summary(out, game_id, me);
    build_exploration(out, game_id, me);
    build_notable_events(out, game_id, me);
    build_footer(out);

    Telemetry::instance().write(out.str());
    return true;
}

bool RecapCommand::emit_recap(int game_id, char player)
{
    std::string text;
    bool ok = build_recap_text(game_id, player, text);
    if (ok)
    {
        Telemetry::instance().add_tell(game_id, player, text);
    }
    return ok;
}

bool RecapCommand::build_recap_text(int game_id, char player,
                                    std::string& result_text)
{
    RecapCommand cmd;
    std::ostringstream out;
    cmd.build_header(out, game_id, player);
    cmd.build_fleet_registry(out, game_id, player);
    cmd.build_combat_record(out, game_id, player);
    cmd.build_economic_summary(out, game_id, player);
    cmd.build_exploration(out, game_id, player);
    cmd.build_notable_events(out, game_id, player);
    cmd.build_footer(out);
    result_text = out.str();
    return true;
}

// ---------------------------------------------------------------
//  HEADER
// ---------------------------------------------------------------
void RecapCommand::build_header(std::ostringstream& out, int game_id,
                                char me)
{
    DatabaseManager& db = DatabaseManager::instance();
    char enemy = me ^ 0x03;  // A or B

    // Get game data
    std::vector<std::vector<std::string>> game_rows = db.Query(
        "SELECT vp_A, vp_B, winner, state_json FROM games WHERE id=?",
        {game_id});

    int vpA = 0;
    int vpB = 0;
    std::string winner;
    int round = 0;

    if (!game_rows.empty())
    {
        vpA = std::atoi(game_rows[0][0].c_str());
        vpB = std::atoi(game_rows[0][1].c_str());
        winner = game_rows[0][2];
    }

    // Get round from GameState (more reliable than parsing state_json)
    GameState s = StateMachine::instance().get_game_state();
    round = s.round;

    int my_vp = (KH_EQU(me, 'A')) ? vpA : vpB;
    int enemy_vp = (KH_EQU(me, 'A')) ? vpB : vpA;
    int my_credits = (KH_EQU(me, 'A')) ? s.creditsA : s.creditsB;

    // Get my username
    std::string my_name = "Unknown";
    std::vector<std::vector<std::string>> name_rows = db.Query(
        "SELECT u.username FROM users u "
        "JOIN game_seats gs ON gs.user_id = u.id "
        "WHERE gs.game_id=? AND gs.seat=?",
        {game_id, me});
    if (!name_rows.empty())
    {
        my_name = name_rows[0][0];
    }

    // Determine outcome text
    std::string outcome;
    if (!winner.empty() && KH_EQU(winner[0], me))
    {
        outcome = std::format("VICTORY - {}", my_name);
    }
    else
    {
        outcome = std::format("DEFEAT - {}", my_name);
    }

    // Get max tech level (across all ships ever built, alive or dead)
    std::vector<std::vector<std::string>> tech_rows = db.Query(
        "SELECT COALESCE(MAX(tech_level), 0) FROM ships "
        "WHERE game_id=? AND owner=?",
        {game_id, me});
    int max_tech =
        tech_rows.empty() ? 0 : std::atoi(tech_rows[0][0].c_str());

    out << "\n" << kDivider << "\n";
    out << "         K E P L E R ' S   H O R I Z O N\n";
    out << "              MISSION DEBRIEF REPORT\n";
    out << kDivider << "\n";
    out << std::format("OUTCOME: {}\n", outcome);
    out << std::format("FINAL VP: You {}, Enemy {}       ROUNDS PLAYED: {}\n",
                       my_vp, enemy_vp, round);
    out << std::format("CREDITS REMAINING: {} CR      MAX TECH: L{}\n",
                       my_credits, max_tech);
    out << kSectionLine << "\n";
}

// ---------------------------------------------------------------
//  FLEET REGISTRY
// ---------------------------------------------------------------
void RecapCommand::build_fleet_registry(std::ostringstream& out,
                                        int game_id, char me)
{
    DatabaseManager& db = DatabaseManager::instance();

    // All ships ever owned
    std::vector<std::vector<std::string>> ships = db.Query(
        "SELECT ship_code, ship_name, ship_type, built_turn, "
        "destroyed_at, tech_level "
        "FROM ships WHERE game_id=? AND owner=? "
        "ORDER BY ship_code",
        {game_id, me});

    int total = static_cast<int>(ships.size());
    int destroyed = 0;
    for (const std::vector<std::string>& row : ships)
    {
        if (!row[4].empty())
        {
            destroyed++;
        }
    }
    int surviving = total - destroyed;

    out << "\n  FLEET REGISTRY\n";
    out << kSectionLine << "\n";
    out << std::format("Ships Commissioned: {:>5}       Ships Lost: {:>5}\n",
                       total, destroyed);
    out << std::format("Ships Surviving:    {:>5}\n", surviving);
    out << "\n";

    // List each ship
    for (const std::vector<std::string>& row : ships)
    {
        std::string code = row[0];
        std::string name = row[1];
        std::string built = row[3];
        bool is_destroyed = !row[4].empty();

        // Pad name to 14 chars for alignment
        std::string padded_name = name;
        if (padded_name.size() < 14)
        {
            padded_name.resize(14, ' ');
        }

        if (is_destroyed)
        {
            out << std::format("  {:4} {} Built {:5}  LOST\n", code,
                               padded_name, built);
        }
        else
        {
            out << std::format("  {:4} {} Built {:5}  ACTIVE\n", code,
                               padded_name, built);
        }
    }

    out << "\n";
}

// ---------------------------------------------------------------
//  COMBAT RECORD
// ---------------------------------------------------------------
void RecapCommand::build_combat_record(std::ostringstream& out,
                                       int game_id, char me)
{
    DatabaseManager& db = DatabaseManager::instance();
    char enemy = me ^ 0x03;

    // Distinct engagement hexes
    std::vector<std::vector<std::string>> hex_rows = db.Query(
        "SELECT COUNT(DISTINCT hex_id) FROM combat_state WHERE game_id=?",
        {game_id});
    int hexes_contested =
        hex_rows.empty() ? 0 : std::atoi(hex_rows[0][0].c_str());

    // Total combat rounds fought
    std::vector<std::vector<std::string>> round_rows = db.Query(
        "SELECT COALESCE(SUM(round), 0) FROM combat_state WHERE game_id=?",
        {game_id});
    int combat_rounds =
        round_rows.empty() ? 0 : std::atoi(round_rows[0][0].c_str());

    // Enemy ships destroyed
    std::vector<std::vector<std::string>> enemy_lost = db.Query(
        "SELECT COUNT(*) FROM ships "
        "WHERE game_id=? AND owner=? AND destroyed_at IS NOT NULL",
        {game_id, enemy});
    int enemy_destroyed =
        enemy_lost.empty() ? 0 : std::atoi(enemy_lost[0][0].c_str());

    // Own ships lost
    std::vector<std::vector<std::string>> my_lost = db.Query(
        "SELECT COUNT(*) FROM ships "
        "WHERE game_id=? AND owner=? AND destroyed_at IS NOT NULL",
        {game_id, me});
    int own_lost = my_lost.empty() ? 0 : std::atoi(my_lost[0][0].c_str());

    out << kSectionLine << "\n";
    out << "  COMBAT RECORD\n";
    out << kSectionLine << "\n";

    if (hexes_contested == 0)
    {
        out << "No engagements fought.\n";
    }
    else
    {
        out << std::format(
            "Hexes Contested:    {:>5}       Combat Rounds: {:>5}\n",
            hexes_contested, combat_rounds);
        out << std::format(
            "Enemy Ships Killed: {:>5}       Own Ships Lost: {:>4}\n",
            enemy_destroyed, own_lost);
    }

    out << "\n";
}

// ---------------------------------------------------------------
//  ECONOMIC SUMMARY
// ---------------------------------------------------------------
void RecapCommand::build_economic_summary(std::ostringstream& out,
                                          int game_id, char me)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Total resources extracted
    std::vector<std::vector<std::string>> extract_rows = db.Query(
        "SELECT COALESCE(SUM(yield), 0) FROM extract_operations "
        "WHERE game_id=? AND owner=? AND completed=1",
        {game_id, me});
    int total_extracted =
        extract_rows.empty() ? 0 : std::atoi(extract_rows[0][0].c_str());

    // Market totals (aggregate across all resource types)
    std::vector<std::vector<std::string>> market_rows = db.Query(
        "SELECT COALESCE(SUM(total_bought), 0), "
        "COALESCE(SUM(total_sold), 0) "
        "FROM market_prices WHERE game_id=?",
        {game_id});
    int total_bought =
        market_rows.empty() ? 0 : std::atoi(market_rows[0][0].c_str());
    int total_sold =
        market_rows.empty() ? 0 : std::atoi(market_rows[0][1].c_str());

    // Fabrication completed items (grouped by recipe)
    std::vector<std::vector<std::string>> fab_rows = db.Query(
        "SELECT recipe, COUNT(*), COALESCE(SUM(quantity), 0) "
        "FROM fabrication_queue "
        "WHERE game_id=? AND player=? AND status='COMPLETED' "
        "GROUP BY recipe",
        {game_id, me});
    int total_fab_jobs = 0;
    for (const std::vector<std::string>& row : fab_rows)
    {
        total_fab_jobs += std::atoi(row[1].c_str());
    }

    out << kSectionLine << "\n";
    out << "  ECONOMIC SUMMARY\n";
    out << kSectionLine << "\n";
    out << std::format(
        "Resources Extracted: {:>4}       Items Fabricated: {:>4}\n",
        total_extracted, total_fab_jobs);
    out << std::format(
        "Market Bought:       {:>4}       Market Sold:      {:>4}\n",
        total_bought, total_sold);

    // Detail fabrication by recipe if any
    if (!fab_rows.empty())
    {
        out << "\n  Fabrication Detail:\n";
        for (const std::vector<std::string>& row : fab_rows)
        {
            std::string recipe = row[0];
            int qty = std::atoi(row[2].c_str());
            out << std::format("    {:16} {:>4}\n", recipe, qty);
        }
    }

    out << "\n";
}

// ---------------------------------------------------------------
//  EXPLORATION & TERRITORY
// ---------------------------------------------------------------
void RecapCommand::build_exploration(std::ostringstream& out,
                                     int game_id, char me)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Systems explored (codex entries that are not 'Unknown')
    std::vector<std::vector<std::string>> codex_rows = db.Query(
        "SELECT COUNT(*) FROM codex_entries "
        "WHERE game_id=? AND player=? AND knowledge_level != 'Unknown'",
        {game_id, me});
    int systems_explored =
        codex_rows.empty() ? 0 : std::atoi(codex_rows[0][0].c_str());

    // Total systems in module
    int module_id = 1;
    std::vector<std::vector<std::string>> mod_rows =
        db.Query("SELECT module_id FROM games WHERE id=?", {game_id});
    if (!mod_rows.empty())
    {
        module_id = std::atoi(mod_rows[0][0].c_str());
    }
    std::vector<std::vector<std::string>> total_sys = db.Query(
        "SELECT COUNT(*) FROM star_systems WHERE module_id=?", {module_id});
    int total_systems =
        total_sys.empty() ? 0 : std::atoi(total_sys[0][0].c_str());

    // Facilities controlled at end of game
    std::vector<std::vector<std::string>> fac_rows = db.Query(
        "SELECT COUNT(*) FROM facility_control "
        "WHERE game_id=? AND controller=?",
        {game_id, me});
    int facilities_held =
        fac_rows.empty() ? 0 : std::atoi(fac_rows[0][0].c_str());

    // Anomalies discovered
    std::vector<std::vector<std::string>> anom_rows = db.Query(
        "SELECT COUNT(*) FROM anomaly_events "
        "WHERE game_id=? AND player=?",
        {game_id, me});
    int anomalies =
        anom_rows.empty() ? 0 : std::atoi(anom_rows[0][0].c_str());

    // Salvage operations
    std::vector<std::vector<std::string>> salv_rows = db.Query(
        "SELECT COUNT(*) FROM salvage_operations WHERE game_id=?",
        {game_id});
    int salvage_ops =
        salv_rows.empty() ? 0 : std::atoi(salv_rows[0][0].c_str());

    out << kSectionLine << "\n";
    out << "  EXPLORATION & TERRITORY\n";
    out << kSectionLine << "\n";
    out << std::format(
        "Systems Explored: {:>3} / {:>3}     Facilities Held: {:>4}\n",
        systems_explored, total_systems, facilities_held);
    out << std::format(
        "Anomalies Found:      {:>3}       Salvage Ops:     {:>4}\n",
        anomalies, salvage_ops);
    out << "\n";
}

// ---------------------------------------------------------------
//  NOTABLE EVENTS
// ---------------------------------------------------------------
void RecapCommand::build_notable_events(std::ostringstream& out,
                                        int game_id, char me)
{
    DatabaseManager& db = DatabaseManager::instance();

    // Collect events into a timeline: (turn, description)
    struct TimelineEntry
    {
        int turn;
        std::string description;
    };
    std::vector<TimelineEntry> timeline;

    // First enemy sighting
    std::vector<std::vector<std::string>> sight_rows = db.Query(
        "SELECT ship_name, at_system, last_seen_turn FROM sightings "
        "WHERE game_id=? AND observer_owner=? "
        "ORDER BY last_seen_turn LIMIT 1",
        {game_id, me});
    if (!sight_rows.empty())
    {
        int turn = std::atoi(sight_rows[0][2].c_str());
        TimelineEntry entry;
        entry.turn = turn;
        entry.description =
            std::format("First enemy contact at {}", sight_rows[0][1]);
        timeline.push_back(entry);
    }

    // Ship losses (own ships destroyed)
    std::vector<std::vector<std::string>> loss_rows = db.Query(
        "SELECT ship_code, ship_name, at_hex, built_turn FROM ships "
        "WHERE game_id=? AND owner=? AND destroyed_at IS NOT NULL "
        "ORDER BY destroyed_at",
        {game_id, me});
    for (const std::vector<std::string>& row : loss_rows)
    {
        // built_turn is like "R3" - use it as an approximation
        // The actual round lost is harder to extract from destroyed_at timestamp
        // We'll use a simple incrementing approach
        TimelineEntry entry;
        entry.turn = 0;
        std::string hex = row[2].empty() ? "unknown" : row[2];
        entry.description =
            std::format("{} {} lost at hex {}", row[0], row[1], hex);
        timeline.push_back(entry);
    }

    // Facility captures
    std::vector<std::vector<std::string>> cap_rows = db.Query(
        "SELECT system_name, facility_type, occupied_since "
        "FROM facility_control "
        "WHERE game_id=? AND controller=? AND occupied_since IS NOT NULL "
        "ORDER BY occupied_since",
        {game_id, me});
    for (const std::vector<std::string>& row : cap_rows)
    {
        int turn = std::atoi(row[2].c_str());
        TimelineEntry entry;
        entry.turn = turn;
        entry.description =
            std::format("Captured {} at {}", row[1], row[0]);
        timeline.push_back(entry);
    }

    // Victory/defeat (from games table)
    std::vector<std::vector<std::string>> game_rows = db.Query(
        "SELECT winner FROM games WHERE id=?", {game_id});
    if (!game_rows.empty() && !game_rows[0][0].empty())
    {
        GameState s = StateMachine::instance().get_game_state();
        TimelineEntry entry;
        entry.turn = s.round;
        if (KH_EQU(game_rows[0][0][0], me))
        {
            entry.description = "Victory declared";
        }
        else
        {
            entry.description = "Defeat";
        }
        timeline.push_back(entry);
    }

    out << kSectionLine << "\n";
    out << "  NOTABLE EVENTS\n";
    out << kSectionLine << "\n";

    if (timeline.empty())
    {
        out << "No notable events recorded.\n";
    }
    else
    {
        // Cap at 10 entries
        int limit = 10;
        int count = 0;
        for (const TimelineEntry& evt : timeline)
        {
            if (count >= limit)
            {
                break;
            }
            if (evt.turn > 0)
            {
                out << std::format("  R{:<3} {}\n", evt.turn,
                                   evt.description);
            }
            else
            {
                out << std::format("       {}\n", evt.description);
            }
            count++;
        }
    }

    out << "\n";
}

// ---------------------------------------------------------------
//  FOOTER
// ---------------------------------------------------------------
void RecapCommand::build_footer(std::ostringstream& out)
{
    // Pick a quote based on current time (pseudo-random)
    std::time_t now = std::time(nullptr);
    int idx = static_cast<int>(now % kNumQuotes);

    out << kDivider << "\n";
    out << std::format("       {}\n", kClosingQuotes[idx]);
    out << kDivider << "\n";
}
