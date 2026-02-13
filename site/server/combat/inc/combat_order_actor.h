///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMBAT_ORDER_COMMAND_H__
#define __KH_COMBAT_ORDER_COMMAND_H__

#include <string>
#include <variant>
#include "attributemap.h"
#include "combattypes.h"
#include "icmd.h"

class CombatOrderActor : public ICmd
{
  private:
    std::string m_ship_code;
    std::string m_target_id;
    char m_tactic;
    AttributeMap m_attributes;
    MissileSet m_firing_missiles;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _target_id;
        char _tactic;
        AttributeMap _attributes;
        MissileSet _firing_missiles;

        Builder() : _tactic(KH_N_TACTIC)
        {
        }

        Builder& ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }

        Builder& target(const std::string& tgt)
        {
            _target_id = tgt;
            return *this;
        }

        Builder& tactic(char t)
        {
            _tactic = t;
            return *this;
        }

        Builder& set_attributes(const AttributeMap& atmap)
        {
            _attributes = atmap;
            return *this;
        }

        Builder& set_missiles(const MissileSet& ms)
        {
            _firing_missiles = ms;
            return *this;
        }

        ICmd* build()
        {
            return new CombatOrderActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    CombatOrderActor(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_target_id(std::move(builder._target_id)), m_tactic(builder._tactic),
          m_attributes(std::move(builder._attributes)),
          m_firing_missiles(std::move(builder._firing_missiles))
    {
    }
};

#endif
