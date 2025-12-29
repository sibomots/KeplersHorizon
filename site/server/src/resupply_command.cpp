#include "resupply_command.h"
#include "statemachine.h"

bool ResupplyCommand::invoke(void)
{
    // Delegate to StateMachine to handle resupply logic.
    StateMachine& sm = StateMachine::getInstance();
    sm.set_pending_resupply(m_ship_code, m_missiles);
    return sm.transition();
}
