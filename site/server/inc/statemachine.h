///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__

#include <memory>
#include "typedefs.h"
#include "icmd.h"
#include "db.h"

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
        PlayerState state;
        Player initiative;
        int game_id;
        int turn_number; // For tech level calculation
        ScenarioType scenario;

        // Build Phase properties
        bool pending_build_commit;
        bool pending_build_cancel;
        bool pending_build_list_drafts;
        std::string pending_build_show_draft;
        AttributeMap pending_build_attributes; // For build set command
        std::string pending_build_draft;       // For old BuildCommand
        std::string pending_repair_ship;
        std::string pending_repair_attribute;
        int pending_repair_amount;
        std::string pending_resupply_ship;
        int pending_resupply_missiles;
    } Data;

  public:
    static StateMachine &getInstance()
    {
        static StateMachine instance;
        return instance;
    }

    StateMachine(const StateMachine &) = delete;
    StateMachine &operator=(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = delete;
    StateMachine &operator=(StateMachine &&) = delete;

    // state machine properties and objectives prior to game playability
    bool preinitialize();
    bool initialize();


    // user-facing invokables
    bool active_player_execute(ICmd *pICmd);
    bool nonactive_player_execute(ICmd *pICmd);

    // Basic setup
    void set_db(Db *db)
    {
        m_db = db;
    }
    Db *get_db() const
    {
        return m_db;
    }

    void set_game_id(int id)
    {
        data.game_id = id;
    }
    int get_game_id() const
    {
        return data.game_id;
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
    GameState get_game_state() const
    {
        return load_game(m_db, data.game_id);
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
    void set_pending_build_show_draft(const std::string &code)
    {
        data.pending_build_show_draft = code;
    }
    void set_pending_build_attributes(const AttributeMap &attrs)
    {
        data.pending_build_attributes = attrs;
    }
    void set_pending_build_draft(const std::string &code)
    {
        data.pending_build_draft = code;
    }

    void set_pending_repair(const std::string &ship, const std::string &attr,
                            int amount)
    {
        data.pending_repair_ship = ship;
        data.pending_repair_attribute = attr;
        data.pending_repair_amount = amount;
    }

    void set_pending_resupply(const std::string &ship, int missiles)
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
    Db *m_db = nullptr;

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
