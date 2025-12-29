#include "repair_command.h"

#include "statemachine.h"

bool RepairCommand::invoke(void)
{
    // Delegate to StateMachine to handle repair logic.
    StateMachine &sm = StateMachine::getInstance();
    sm.set_pending_repair(m_ship_code, m_attribute, m_amount);
    return sm.transition();
}
