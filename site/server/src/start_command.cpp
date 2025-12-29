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

bool StartCommand::invoke(void)
{
    bool res = true;

    // TBD
    
    return res;
}


