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
    TorpedoSet m_firing_torpedoes;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _target_id;
        char _tactic;
        AttributeMap _attributes;
        TorpedoSet _firing_torpedoes;

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

        Builder& set_torpedoes(const TorpedoSet& ms)
        {
            _firing_torpedoes = ms;
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
          m_firing_torpedoes(std::move(builder._firing_torpedoes))
    {
    }
};

#endif
