//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_AUTONOMY_AGENCY_H__
#define __KH_AUTONOMY_AGENCY_H__

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

struct AAShipInfo
{
    std::string code;
    std::string name;
    std::string hex_id;
    int pd;
    int beam;
    int screen;
    int tube;
    int missile;
    int sr;
    int tech_level;
    bool is_warpship;
    std::vector<std::string> carried_systemships;

    // AI movement planning: farthest reachable waypoint toward strategic goal
    std::string suggested_destination;

    AAShipInfo()
        : pd(0), beam(0), screen(0), tube(0), missile(0), sr(0), tech_level(0),
          is_warpship(true)
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
        home_side.clear();
        own_ships.clear();
        enemy_ships.clear();
        draft_ships.clear();
        in_combat = false;
        contested_hexes.clear();
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
    void calculate(std::vector<std::string>& commands_out);
    void render(const std::vector<std::string>& commands);

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

    // ECL state
    bool m_ecl_initialized;
};

#endif
