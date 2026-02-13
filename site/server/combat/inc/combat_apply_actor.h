///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMBAT_APPLY_ACTOR_H__
#define __KH_COMBAT_APPLY_ACTOR_H__

#include <map>
#include <string>

#include "attributemap.h"
#include "icmd.h"

class CombatApplyActor : public ICmd
{
  private:
    std::string m_ship_code;
    AttributeMap m_assignments;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        AttributeMap _assignments;

        Builder()
        {
        }

        Builder& ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }

        Builder& set_assignments(const AttributeMap& damage)
        {
            _assignments = damage;
            return *this;
        }

        ICmd* build()
        {
            return new CombatApplyActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    CombatApplyActor(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_assignments(std::move(builder._assignments))
    {
    }
};

#endif
