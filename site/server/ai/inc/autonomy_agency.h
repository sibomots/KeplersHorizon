///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_AUTONOMY_AGENCY_H__
#define __KH_AUTONOMY_AGENCY_H__

#include "configure_command.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ----------------------------------------------------------------------------
// AASlate: The AA's cached view of game state
//
// Populated during Gather step (C++), consumed by Calculate step (ECL).
// ----------------------------------------------------------------------------

#define AI_THREAD_PUMP_DELAY (2800)

struct AAShipInfo
{
    std::string code;
    std::string name;
    std::string hex_id;
    int pd;      // Remaining PD for power allocation (base - spent)
    int base_pd; // Physical PD for damage assignment (from ships table)
    int beam;
    int screen;
    int tube;
    int missile;
    int sr;
    int tech_level;
    bool is_warpship;
    std::string racked_in;
    std::vector<std::string> carried_systemships;

    // AI movement planning: farthest reachable waypoint toward strategic goal
    std::string suggested_destination;

    // Combat state
    bool needs_combat_order; // Stage 0: no committed order yet
    int pending_damage;      // Stage 2: damage to assign
    bool escape_pending;     // Successful retreat, needs retreat command

    // Revealed enemy order from prior round (public after both commit)
    char last_tactic; // 'A', 'D', 'E', or '\0' if unknown
    int last_drive;   // Power allocated to drive
    int last_beam;    // Power allocated to beam
    int last_screen;  // Power allocated to screen
    int last_tube;    // Power allocated to tube

    // Economic layer: cargo and capacity
    int cargo_ferrous;
    int cargo_rare_earth;
    int cargo_radioactive;
    int cargo_crystalline;
    int cargo_volatile;
    int cargo_water;
    int cargo_organic;
    int cargo_exotic;
    int cargo_missiles;
    int cargo_capacity;
    int missiles_max;
    std::string at_system;

    // Damage tracking: max values for repair decisions
    int pd_max;
    int beam_max;
    int screen_max;
    int tube_max;

    // Equipment
    int lrs; // Long Range Scanner

    AAShipInfo()
        : pd(0), base_pd(0), beam(0), screen(0), tube(0), missile(0), sr(0),
          tech_level(0), is_warpship(true), needs_combat_order(false),
          pending_damage(0), escape_pending(false), last_tactic('\0'),
          last_drive(0), last_beam(0), last_screen(0), last_tube(0),
          cargo_ferrous(0), cargo_rare_earth(0),
          cargo_radioactive(0), cargo_crystalline(0), cargo_volatile(0),
          cargo_water(0), cargo_organic(0), cargo_exotic(0), cargo_missiles(0),
          cargo_capacity(10), missiles_max(0), pd_max(0), beam_max(0),
          screen_max(0), tube_max(0), lrs(0)
    {
    }
};

// Combat hex info
struct AACombatHex
{
    std::string hex_id;
    int stage; // 0=ORDERS, 1=RESOLVE, 2=DAMAGE, 3=RETREAT
    int round;
    bool ai_committed;     // Has AI committed orders this round?
    int stalemate_counter; // Consecutive no-damage rounds
    bool ai_is_attacker;   // Did AI move into this hex (initiative)?

    AACombatHex()
        : stage(0), round(0), ai_committed(false), stalemate_counter(0),
          ai_is_attacker(false)
    {
    }
};

// Economic layer: player knowledge of systems
struct AACodexEntry
{
    std::string system_name;
    std::string level; // Unknown, Charted, Surveyed, Intimate

    AACodexEntry()
    {
    }
};

// Economic layer: resources at ship locations
struct AAResourceInfo
{
    std::string system;
    std::string type;      // FERROUS, RARE_EARTH, etc.
    std::string abundance; // Rich, High, Moderate, Low, Trace
    int yield;             // Pre-computed expected yield

    AAResourceInfo() : yield(0)
    {
    }
};

// Economic layer: facilities player can use
struct AAFacilityInfo
{
    std::string system;
    std::string type; // SHIPYARD, REFINERY, TRADE_HUB, REPAIR_DOCK
    char controller;  // 'A', 'B', or '\0' for neutral

    AAFacilityInfo() : controller('\0')
    {
    }
};

// Economic layer: market prices for trade decisions
struct AAMarketPrice
{
    std::string resource_type;
    int current_price;
    int base_price;

    AAMarketPrice() : current_price(0), base_price(0)
    {
    }
};

// Economic layer: salvageable objects at ship locations
struct AASalvageableInfo
{
    std::string hex_id;
    std::string object_type;
    std::string state;
    int salvage_value;

    AASalvageableInfo() : salvage_value(0)
    {
    }
};

// Persisted AI metric (cross-turn memory)
struct AAMetric
{
    std::string name;
    double value;
    int updatedRound;

    AAMetric() : value(0.0), updatedRound(0)
    {
    }
};

// BFS hop-count between two key hexes (replaces Manhattan approximation)
struct AAHexDistance
{
    std::string fromHex;
    std::string toHex;
    int cost;

    AAHexDistance() : cost(999)
    {
    }
};

// Hex adjacency entry: a hex and its geometric + warpline neighbors
struct AAHexAdjacency
{
    std::string hexId;
    std::vector<std::string> neighbors;

    AAHexAdjacency()
    {
    }
};

struct AASlate
{
    // Identity
    int game_id;
    char aa_player;

    // Turn state (from StateMachine)
    int round;
    int phase_index;
    char active_player;
    bool is_aa_turn;
    bool game_over;

    // Resources (from GameState)
    int credits;
    int tech_level;
    int vp;
    int enemy_vp;

    // Territory (from DB)
    std::vector<std::string> own_base_hexes;
    std::vector<std::string> enemy_base_hexes;
    std::string home_side;

    // Fleet (from DB: warpships table)
    std::vector<AAShipInfo> own_ships;
    std::vector<AAShipInfo> enemy_ships;

    // Drafts (from DB: ship_drafts table)
    std::vector<AAShipInfo> draft_ships;

    // Combat (from DB)
    bool in_combat;
    std::vector<std::string> contested_hexes;
    std::vector<AACombatHex> active_combats;

    // Economic layer (from DB)
    std::vector<AACodexEntry> codex;
    std::vector<AAResourceInfo> resources;
    std::vector<AAFacilityInfo> facilities;
    std::vector<AAMarketPrice> market_prices;
    std::vector<AASalvageableInfo> salvageables;

    // Cross-turn memory (from aa_metrics table)
    std::vector<AAMetric> persisted_metrics;

    // BFS distances between key hex pairs (computed per cycle via MapGraph)
    std::vector<AAHexDistance> hex_distances;

    // Hexes that lie on warplines (strategic chokepoints)
    std::vector<std::string> warpline_hexes;

    // Hex adjacency (geometric + warpline neighbors for relevant hexes)
    std::vector<AAHexAdjacency> hex_adjacency;

    // Cycle tracking
    unsigned int cycle_id;

    AASlate()
        : game_id(0), aa_player('\0'), round(0), phase_index(0),
          active_player('\0'), is_aa_turn(false), game_over(false), credits(0),
          tech_level(0), vp(0), enemy_vp(0), in_combat(false), cycle_id(0)
    {
    }

    void clear()
    {
        game_id = 0;
        round = 0;
        phase_index = 0;
        active_player = '\0';
        is_aa_turn = false;
        game_over = false;
        credits = 0;
        tech_level = 0;
        vp = 0;
        enemy_vp = 0;
        own_base_hexes.clear();
        enemy_base_hexes.clear();
        home_side.clear();
        own_ships.clear();
        enemy_ships.clear();
        draft_ships.clear();
        in_combat = false;
        contested_hexes.clear();
        active_combats.clear();
        codex.clear();
        resources.clear();
        facilities.clear();
        market_prices.clear();
        salvageables.clear();
        persisted_metrics.clear();
        hex_distances.clear();
        warpline_hexes.clear();
        hex_adjacency.clear();
    }
};

// ----------------------------------------------------------------------------
// AutonomyAgency: Threaded Mealy State Machine for single-player AI
//
// Steps:
//   Gather    (C++)  - Populate AASlate from StateMachine + DB
//   Calculate (ECL)  - Marshal slate to Lisp, get command specs back
//   Render    (C++)  - Inject commands via AICommandInjector
// ----------------------------------------------------------------------------

class AutonomyAgency
{
  public:

    friend class ConfigureCommand;

    static AutonomyAgency& instance();

    // Configuration
    void configure(int game_id, char aa_player);

    // Lifecycle
    void start();
    void stop();
    void join();

    // Wake the thread to run a cycle
    void pump();

    // Accessors
    int get_game_id() const
    {
        return m_game_id;
    }

    char get_aa_player() const
    {
        return m_aa_player;
    }

    bool is_running() const
    {
        return m_running.load();
    }

    // Task synchronization
    void wait_for_task_completion();
    void notify_task_complete();

  private:
    AutonomyAgency();
    ~AutonomyAgency();

    // Prevent copying
    AutonomyAgency(const AutonomyAgency&) = delete;
    AutonomyAgency& operator=(const AutonomyAgency&) = delete;

    // Thread entry
    void thread_main();

    // Run one MSS cycle
    void run_cycle();

    // MSS steps
    void gather();
    bool calculate(std::vector<std::string>& commands_out,
                   std::vector<std::pair<std::string, double>>& metrics_out);
    bool combat_needs_response() const;
    void log_debug_state();

    // Metric persistence
    bool persist_metric(const std::string& name, double value, int round);

    // ECL initialization
    void init_ecl();
    void shutdown_ecl();

  private:
    // Configuration
    int m_game_id;
    char m_aa_player;

    // State
    AASlate m_slate;
    unsigned int m_cycle_counter;

    // Threading
    std::thread m_worker;
    std::atomic<bool> m_running;
    std::atomic<bool> m_stop_requested;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_pumped;

    // Task synchronization
    std::condition_variable m_task_cv;
    std::mutex m_task_mtx;
    bool m_task_done;

    // ECL state
    bool m_ecl_initialized;
};

#endif
