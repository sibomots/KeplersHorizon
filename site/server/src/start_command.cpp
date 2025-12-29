#include "start_command.h"

/// @brief invoke SQL to cause game start to be established
///
/// What else do I need to do or know in order to put
/// the Database in a condition that a game has started?
/// Visit the StateMachine to propel the state to
///  READY_GAME_START.
/// The state machine will know what to do to affect
/// evolution to the state 
///   GAME_START
/// and so on.
/// We prefer one return in a method or function.
/// early exit from functions/methods leads to confusion
/// some of the time.
/// @param[in] none
/// @param[out] none
/// @return true if successful

#include "db.h"
#include "game.h"
bool StartCommand::invoke(void)
{
    // Delegate to StateMachine to handle game initialization specific to this command.
    // The Command's job is simply to trigger the State Change with the necessary data.
    StateMachine& sm = StateMachine::getInstance();
    sm.set_scenario(m_scenario);
    return sm.transition();
}


