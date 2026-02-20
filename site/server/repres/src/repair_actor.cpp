///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "repair_actor.h"

#include <cassert>
#include <format>

#include "represagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

RepairActor::Builder::Builder()
    : m_mode(RepairMode::REPAIR_LIST),
      m_attribute(AttributeID::UNKNOWN),
      m_amount(0)
{
}

RepairActor::Builder& RepairActor::Builder::set_list_mode()
{
    m_mode = RepairMode::REPAIR_LIST;
    return *this;
}

RepairActor::Builder& RepairActor::Builder::set_ship_code(const std::string& code)
{
    m_ship_code = code;
    return *this;
}

RepairActor::Builder& RepairActor::Builder::set_attributes(const AttributeMap& attrs)
{
    assert(attrs.data.size() == 1);
    auto it = attrs.data.begin();
    AttributeID attr = it->first;
    assert(attr == AttributeID::POWER_DRIVE ||
           attr == AttributeID::PHASIC ||
           attr == AttributeID::SHIELD ||
           attr == AttributeID::LAUNCHER);
    m_attribute = attr;
    m_amount = it->second;
    m_mode = RepairMode::REPAIR_APPLY;
    return *this;
}

ICmd* RepairActor::Builder::build()
{
    return new RepairActor(m_mode, m_ship_code, m_attribute, m_amount);
}

RepairActor::RepairActor(RepairMode mode, const std::string& ship,
                         AttributeID attr, int amount)
    : m_mode(mode), m_ship_code(ship), m_attribute(attr), m_amount(amount)
{
}

bool RepairActor::invoke(void)
{
    bool bres = false;
    std::string inhibit_error;
    bool bAllowed = StateMachine::instance().check_inhibits(CommandID::REPAIR,
                                                            inhibit_error);
    if (!bAllowed)
    {
        Telemetry::instance().write(std::format("Error: {}", inhibit_error));
    }
    else
    {
        GameState s = StateMachine::instance().get_game_state();
        char owner = StateMachine::instance().get_current_player();

        RepairParam::Builder rpb;
        rpb.set_game_id(s.game_id).set_player(owner);

        if (m_mode == RepairMode::REPAIR_LIST)
        {
            rpb.set_list_mode();
        }
        else
        {
            rpb.set_ship_code(m_ship_code)
               .set_attribute(m_attribute)
               .set_amount(m_amount);
        }

        RepairParam rp = rpb.build();
        RepresAgentParam param(rp);
        bres = RepresAgent::instance().apply(param);
    }

    return bres;
}
