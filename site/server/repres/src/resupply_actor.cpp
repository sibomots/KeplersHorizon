///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "resupply_actor.h"

#include <format>

#include "represagent.h"
#include "statemachine.h"
#include "telemetry.h"
#include "typedefs.h"

ResupplyActor::Builder::Builder()
    : m_mode(ResupplyMode::RESUPPLY_LIST), m_torpedoes(0)
{
}

ResupplyActor::Builder& ResupplyActor::Builder::set_list_mode()
{
    m_mode = ResupplyMode::RESUPPLY_LIST;
    return *this;
}

ResupplyActor::Builder& ResupplyActor::Builder::set_ship_code(const std::string& code)
{
    m_ship_code = code;
    return *this;
}

ResupplyActor::Builder& ResupplyActor::Builder::set_torpedoes(int n)
{
    m_torpedoes = n;
    m_mode = ResupplyMode::RESUPPLY_TORPEDOES;
    return *this;
}

ICmd* ResupplyActor::Builder::build()
{
    return new ResupplyActor(m_mode, m_ship_code, m_torpedoes);
}

ResupplyActor::ResupplyActor(ResupplyMode mode, const std::string& ship,
                             int torpedoes)
    : m_mode(mode), m_ship_code(ship), m_torpedoes(torpedoes)
{
}

bool ResupplyActor::invoke(void)
{
    bool bres = false;
    std::string inhibit_error;
    bool bAllowed = StateMachine::instance().check_inhibits(CommandID::RESUPPLY,
                                                            inhibit_error);
    if (!bAllowed)
    {
        Telemetry::instance().write(std::format("Error: {}", inhibit_error));
    }
    else
    {
        GameState s = StateMachine::instance().get_game_state();
        char owner = StateMachine::instance().get_current_player();

        ResupplyParam::Builder rpb;
        rpb.set_game_id(s.game_id).set_player(owner);

        if (m_mode == ResupplyMode::RESUPPLY_LIST)
        {
            rpb.set_list_mode();
        }
        else
        {
            rpb.set_torpedoes_mode()
               .set_ship_code(m_ship_code)
               .set_torpedoes(m_torpedoes);
        }

        ResupplyParam rp = rpb.build();
        RepresAgentParam param(rp);
        bres = RepresAgent::instance().apply(param);
    }

    return bres;
}
