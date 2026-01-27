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

#include "attributemap.h"
#include "db.h"
#include "icmd.h"
#include "typedefs.h"

class GameState
{
  public:
    int game_id;

    int round;
    std::string active_player;
    int phase_index;
    int vpA = 0;
    int vpB = 0;
    int creditsA = 0; // Stellar Credits (formerly Build Points)
    int creditsB = 0;
    bool game_over = false;
    std::string winner = "";

    // Territory assignments - set on first deploy
    std::string home_side_A; // 'A' or 'B' (internal only)
    std::string home_side_B; // opposite of home_side_A

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

        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        creditsA = 0;
        creditsB = 0;
        game_over = false;
        winner = "";
    }

    void clear()
    {
        game_id = 0;

        round = 1;
        active_player = "A";
        phase_index = PH_BUILD_SHIPS;
        vpA = 0;
        vpB = 0;
        creditsA = 0;
        creditsB = 0;
        game_over = false;
        winner = "";
        home_side_A = "";
        home_side_B = "";
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
        // NOTE: gameId is NOT included in state_json - it's the database
        // primary key

        o << "\"round\":" << round << ",";
        o << "\"activePlayer\":\"" << active_player << "\",";
        o << "\"phaseIndex\":" << phase_index << ",";
        o << "\"phase\":\"" << json_escape(phase_name()) << "\",";
        o << "\"vp\":{\"A\":" << vpA << ",\"B\":" << vpB << "},";
        o << "\"bp\":{\"A\":" << creditsA << ",\"B\":" << creditsB << "},";
        if (!combat_summary_json.empty())
        {
            o << "\"combat\":" << combat_summary_json << ",";
        }
        if (!home_side_A.empty())
        {
            o << "\"homeSideA\":\"" << home_side_A << "\",";
        }
        if (!home_side_B.empty())
        {
            o << "\"homeSideB\":\"" << home_side_B << "\",";
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
        auto get_num = [&](const std::string& k) -> int
        {
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
        auto get_str = [&](const std::string& k) -> std::string
        {
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
        // NOTE: gameId is NOT parsed from JSON - it comes from database primary
        // key

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
        s.home_side_A = get_str("homeSideA");
        s.home_side_B = get_str("homeSideB");
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

    // Server/Session lifecycle states (NOT turn phases - those use PhaseIndex)
    typedef enum : int
    {
        // Invalid/uninitialized
        INVALID,

        // Server application states
        PREINITIALIZE,
        INITIALIZE,

        // Game lifecycle
        READY_GAME_START,
        GAME_START,
        IN_GAME, // Active gameplay - turn phases tracked via PhaseIndex

    } ServerState;

    typedef struct
    {
        ServerState state = ServerState::INVALID;
        Player initiative = Player::NOPLAYER;
        int game_id = 0;
        char current_player = 'A'; // Who is making the current request
        int current_user_id = 0;   // Database user_id of current requester

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

        // NEW: Single-player mode tracking
        bool is_singleplayer_mode;
        char ai_player_side; // 'A' or 'B', or '\0' if two-player

    } Data;

  public:
    static StateMachine& instance()
    {
        static StateMachine instance;
        return instance;
    }

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    /**
     * @brief Check if a player is AI-controlled
     * @param player The player string ("A" or "B")
     * @return true if player is AI in single-player mode
     */
    bool is_ai_player(const std::string& player) const;

    /**
     * @brief Set game mode (single-player vs two-player)
     * @param singleplayer true for single-player, false for two-player
     * @param ai_player Which side AI controls ('A' or 'B'), ignored if
     * !singleplayer
     */
    void set_game_mode(bool singleplayer, char ai_player);

    // state machine properties and objectives prior to game playability
    bool preinitialize();
    bool initialize();

    GameState load_game(int game_id);

    // BUGBUG
    // legacy block, ingested. needs to be cleaned up
    GameState new_game_state();
    void save_game(const GameState& s);
    void apply_start_of_turn(GameState& s);
    void advance_next(GameState& s);
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

    // Command inhibit checking - determines if a command is allowed
    // based on current game state, phase, and initiative.
    // Returns true if allowed, false if inhibited (error_msg set)
    bool check_inhibits(CommandID cmd, std::string& error_msg);

    // Get player username from game_seats table (returns seat letter if not
    // found)
    std::string get_player_name(int game_id, const std::string& seat);

    // Setters for state properties (used by Commands to set up Transitions)

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

    // Clear game session and return to lobby state
    void clear_game_session()
    {
        data.state = ServerState::READY_GAME_START;
        data.game_id = 0;
        data.current_player = 'A';
        data.current_user_id = 0;
        data.turn_number = 0;
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

  private:
    Data data;

    StateMachine()
    {
        data.state = ServerState::INVALID;
        data.initiative = Player::NOPLAYER;
        data.game_id = 0;
        data.turn_number = 0;

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
