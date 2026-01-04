//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COMBAT_APPLY_COMMAND_H__
#define __COMBAT_APPLY_COMMAND_H__

#include <map>
#include <string>

#include "icmd.h"

class CombatApplyCommand : public ICmd
{
  private:
    std::string m_ship_code;
    std::map<std::string, int> m_assignments;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::map<std::string, int> _assignments;

        Builder()
        {
        }

        Builder& ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }

        Builder& assign(const std::string& attr, int damage)
        {
            _assignments[attr] = damage;
            return *this;
        }

        ICmd* build()
        {
            return new CombatApplyCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    CombatApplyCommand(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_assignments(std::move(builder._assignments))
    {
    }
};

#endif
