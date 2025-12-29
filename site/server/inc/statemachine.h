#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__

class StateMachine {
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
       MOVE_PHASE
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



    typedef struct {
       PlayerState  state; 
       Player initiative;
       int game_id;
       ScenarioType scenario; // The "Noun" for the Start Verb
    } Data;

public:       
    static StateMachine& getInstance() {
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

#include <memory>
#include "icmd.h"

    // user-facing invokables
    bool active_player_execute(ICmd* pICmd);
    bool nonactive_player_execute(ICmd* pICmd);

    // Basic setup
    void set_db(Db* db) { m_db = db; }
    Db* get_db() const { return m_db; }

    void set_game_id(int id) { data.game_id = id; }
    int get_game_id() const { return data.game_id; }

    // Setters for pending state properties (used by Commands to set up Transitions)
    void set_scenario(ScenarioType s) { data.scenario = s; }

    // inward facing utilities
    bool start_game_for_random_player();
    
    // Core state transition logic
    bool transition();

private:
    Data  data;
    Db* m_db = nullptr;
 
    StateMachine() {
       data.state = PlayerState::INVALID;
       data.initiative = Player::NOPLAYER;
       data.game_id = 0;
       data.scenario = ScenarioType::UNDEFINED;
    }
};

#endif

