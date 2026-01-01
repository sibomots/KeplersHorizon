//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__

#include <memory>

#include "db.h"
//#include "game.h"
#include "icmd.h"
#include "typedefs.h"

class GameState
{
  public:
    int game_id;
    std::string scenario; // "", "learning","basic","advanced"
    int round;
    std::string active_player;
    int phase_index;
    int vpA = 0;
    int vpB = 0;
    int bpA = 0;
    int bpB = 0;
    bool game_over = false;
    std::string winner = "";

    // ship lists (future)
    // right now: none
    std::string combat_summary_json; // JSON object for combat state

  public:
    GameState()
    {
        clear();
    }

    void create_empty_game()
    {
        clear();
        game_id = 0;
        scenario = "";
        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        bpA = 0;
        bpB = 0;
        game_over = false;
        winner = "";
    }

    void clear()
    {
        game_id = 0;
        scenario = "";
        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        vpA = 0;
        vpB = 0;
        bpA = 0;
        bpB = 0;
        game_over = false;
        winner = "";
    }
    std::string phase_name() const
    {
        static const char* P[] = {"Build Ships", "Movement", "Resolve Combat",
                                  "SystemShip Pick/Drop", "End of Turn"};
        if (phase_index < PH_BUILD_SHIPS || phase_index > PH_END_TURN)
            return "Build Ships";
        return P[phase_index];
    }

    std::string notes() const
    {
        if (game_over)
            return "Game over. Use 'list' / 'list all' to review.";
        if (scenario.empty())
            return "Type: start learning|basic|advanced";
        if (phase_index == PH_BUILD_SHIPS)
            return "Build/repair/resupply. Use build/deploy/pickup/drop then "
                   "'next'.";
        if (phase_index == PH_MOVEMENT)
            return "Movement (not implemented). Use 'next' to continue.";
        if (phase_index == PH_RESOLVE_COMBAT)
            return "Combat (not implemented). Use 'next' to continue.";
        if (phase_index == PH_SYSTEM_PICKDROP)
            return "SystemShip shuffle (not implemented). Use 'next' to "
                   "continue.";
        if (phase_index == PH_END_TURN)
            return "End of turn. Use 'next' to pass initiative.";
        return "";
    }

    std::string to_json() const
    {
        std::ostringstream o;
        o << "{";
        // NOTE: gameId is NOT included in state_json - it's the database primary key
        o << "\"scenario\":\"" << json_escape(scenario) << "\",";
        o << "\"round\":" << round << ",";
        o << "\"activePlayer\":\"" << active_player << "\",";
        o << "\"phaseIndex\":" << phase_index << ",";
        o << "\"phase\":\"" << json_escape(phase_name()) << "\",";
        o << "\"vp\":{\"A\":" << vpA << ",\"B\":" << vpB << "},";
        o << "\"bp\":{\"A\":" << bpA << ",\"B\":" << bpB << "},";
        if (!combat_summary_json.empty())
        {
            o << "\"combat\":" << combat_summary_json << ",";
        }
        o << "\"notes\":\"" << json_escape(notes()) << "\"";
        o << "}";
        return o.str();
    }

    static GameState from_json_min(const std::string& js)
    {
        // Minimal parse by searching for known fields; not a general parser.
        GameState s;
        s.game_id = 0; // Will be set by load_game() from database primary key
        s.game_over = false;
        s.winner = "";
        auto get_num = [&](const std::string& k) -> int {
            std::string pat = "\"" + k + "\":";
            size_t p = js.find(pat);
            if (p == std::string::npos)
                return 0;
            p += pat.size();
            while (p < js.size() && std::isspace((unsigned char)js[p]))
                p++;
            int sign = 1;
            if (p < js.size() && js[p] == '-')
            {
                sign = -1;
                p++;
            }
            long v = 0;
            while (p < js.size() && std::isdigit((unsigned char)js[p]))
            {
                v = v * 10 + (js[p] - '0');
                p++;
            }
            return (int)(v * sign);
        };
        auto get_str = [&](const std::string& k) -> std::string {
            std::string pat = "\"" + k + "\":\"";
            size_t p = js.find(pat);
            if (p == std::string::npos)
                return "";
            p += pat.size();
            size_t e = js.find("\"", p);
            if (e == std::string::npos)
                return "";
            return js.substr(p, e - p);
        };
        // NOTE: gameId is NOT parsed from JSON - it comes from database primary key
        s.scenario = get_str("scenario");
        s.round = std::max(1, get_num("round"));
        s.active_player = get_str("activePlayer");
        if (s.active_player != "A" && s.active_player != "B")
            s.active_player = "A";
        s.phase_index = get_num("phaseIndex");
        s.vpA = get_num("A"); // NOTE: this will catch first "A" occurrence;
                              // acceptable for now
        s.vpB = 0;
        // We'll avoid parsing vp/bp deeply; state is small; for now server is
        // source of truth in RAM anyway.
        return s;
    }
};

class StateMachine
{
  public:
    typedef enum : int
    {
        PLAYER_A,
        PLAYER_B,
        NOPLAYER,
    } Player;

    typedef enum : int
    {
        // Not a valid State
        INVALID,

        // Not game specific.
        // Specific to the State Machine and Server application.
        PREINITIALIZE,
        INITIALIZE,

        // Session specific.
        PENDING_LOGIN_A,
        LOGIN_A,
        PENDING_LOGIN_B,
        LOGIN_B,

        // Idle until we start
        READY_GAME_START,
        GAME_START,

        ACQUIRE_INITIATIVE,

        ENTER_VICTORY_POINT_COUNT_PHASE,
        VICTORY_POINT_COUNT_PHASE,
        EXIT_VICTORY_POINT_COUNT_PHASE,

        ENTER_BUILD_PHASE,
        BUILD_PHASE,
        EXIT_BUILD_PHASE,

        ENTER_DEPLOY_PHASE,
        DEPLOY_PHASE,
        EXIT_DEPLOY_PHASE,

        ENTER_MOVE_PHASE,
        MOVE_PHASE,
        EXIT_MOVE_PHASE,

        ENTER_TEST_CONFLICT_PHASE,
        TEST_CONFLICT_PHASE,
        EXIT_TEST_CONFLICT_PHASE,

        ENTER_COMBAT_PHASE,
        COMBAT_PHASE,
        ENTER_ORDER_PHASE,
        ORDER_PHASE,
        EXIT_ORDER_PHASE,
        ENTER_APPLY_CRT_PHASE,
        APPLY_CRT_PHASE,
        EXIT_APPLY_CRT_PHASE,
        RESUME_COMBAT_PHASE,
        RESOLVE_STALEMATE,
        CLEANUP_COMBAT,
        EXIT_COMBAT_PHASE,

        ENTER_PICKDROP_PHASE,
        PICKDROP_PHASE,
        EXIT_PICKDROP_PHASE,

        ENTER_REPAIR_RESUPPLY,
        REPAIR_RESUPPLY,
        EXIT_REPAIR_RESUPPLY,

        ENTER_TURN_FINALE,
        TURN_FINALE,
        PASS_INITIATIVE,

    } PlayerState;

    typedef struct
    {
        PlayerState state = PlayerState::INVALID;
        Player initiative = Player::NOPLAYER;
        int game_id = 0;
        char current_player = 'A'; // Who is making the current request
        int current_user_id = 0;   // Database user_id of current requester
        ScenarioType scenario = ScenarioType::UNDEFINED;
        int turn_number = 0; // For tech level calculation

        // Build Phase properties
        bool pending_build_commit = false;
        bool pending_build_cancel = false;
        bool pending_build_list_drafts = false;
        std::string pending_build_show_draft;
        std::string pending_build_draft;
        AttributeMap pending_build_attributes; // For build set command

        // Repair/Resupply Phase properties
        std::string pending_repair_ship;
        std::string pending_repair_attribute;
        int pending_repair_amount = 0;
        std::string pending_resupply_ship;
        int pending_resupply_missiles = 0;
    } Data;

  public:
    static StateMachine& getInstance()
    {
        static StateMachine instance;
        return instance;
    }

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    // state machine properties and objectives prior to game playability
    bool preinitialize();
    bool initialize();

    // user-facing invokables
    bool active_player_execute(ICmd* pICmd);
    bool nonactive_player_execute(ICmd* pICmd);

    GameState load_game(int game_id);

    // BUGBUG
    // legacy block, ingested. needs to be cleaned up
    GameState new_game_state_for_scenario(const std::string& scenario);
    void apply_start_of_turn(GameState& s);
    void advance_next(GameState& s);
    void save_game(const GameState& s);
    int next_event_seq(int game_id);
    // end of legacy block

    // Basic setup
    void set_game_id(int id)
    {
        data.game_id = id;
    }
    int get_game_id() const
    {
        return data.game_id;
    }
    
    void set_current_player(char player)
    {
        data.current_player = player;
    }
    char get_current_player() const
    {
        return data.current_player;
    }

    void set_current_user_id(int user_id)
    {
        data.current_user_id = user_id;
    }
    int get_current_user_id() const
    {
        return data.current_user_id;
    }

    void set_turn_number(int turn)
    {
        data.turn_number = turn;
    }
    int get_turn_number() const
    {
        return data.turn_number;
    }

    // Get current game state (loads from DB using stored game_id)
    GameState get_game_state()
    {
        return load_game(data.game_id);
    }

    // Setters for state properties (used by Commands to set up Transitions)
    void set_scenario(ScenarioType s)
    {
        data.scenario = s;
    }

    void set_pending_build_commit(bool val)
    {
        data.pending_build_commit = val;
    }
    void set_pending_build_cancel(bool val)
    {
        data.pending_build_cancel = val;
    }
    void set_pending_build_list_drafts(bool val)
    {
        data.pending_build_list_drafts = val;
    }
    void set_pending_build_show_draft(const std::string& code)
    {
        data.pending_build_show_draft = code;
    }
    void set_pending_build_attributes(const AttributeMap& attrs)
    {
        data.pending_build_attributes = attrs;
    }
    void set_pending_build_draft(const std::string& code)
    {
        data.pending_build_draft = code;
    }

    void set_pending_repair(const std::string& ship, const std::string& attr,
                            int amount)
    {
        data.pending_repair_ship = ship;
        data.pending_repair_attribute = attr;
        data.pending_repair_amount = amount;
    }

    void set_pending_resupply(const std::string& ship, int missiles)
    {
        data.pending_resupply_ship = ship;
        data.pending_resupply_missiles = missiles;
    }

    // inward facing utilities
    bool start_game_for_random_player();

    // Core state transition logic
    bool transition();

  private:
    Data data;

    StateMachine()
    {
        data.state = PlayerState::INVALID;
        data.initiative = Player::NOPLAYER;
        data.game_id = 0;
        data.turn_number = 0;
        data.scenario = ScenarioType::UNDEFINED;
        data.pending_build_commit = false;
        data.pending_build_cancel = false;
        data.pending_build_list_drafts = false;
        data.pending_build_show_draft.clear();
        data.pending_build_attributes.clear();
        data.pending_build_draft.clear();
        data.pending_repair_ship.clear();
        data.pending_repair_attribute.clear();
        data.pending_repair_amount = 0;
        data.pending_resupply_ship.clear();
        data.pending_resupply_missiles = 0;
    }
};

#endif
