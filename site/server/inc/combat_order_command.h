//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COMBAT_ORDER_COMMAND_H__
#define __COMBAT_ORDER_COMMAND_H__

#include <string>

#include "icmd.h"

class CombatOrderCommand : public ICmd
{
  private:
    std::string m_ship_code;
    std::string m_target_id;
    char m_tactic;
    int m_power_d;
    int m_power_b;
    int m_power_s;
    int m_power_t;
    std::string m_missiles_json;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _target_id;
        char _tactic = 'A';
        int _power_d = 0;
        int _power_b = 0;
        int _power_s = 0;
        int _power_t = 0;
        std::string _missiles_json = "[]";

        Builder()
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

        Builder& drive_power(int d)
        {
            _power_d = d;
            return *this;
        }

        Builder& beam_power(int b)
        {
            _power_b = b;
            return *this;
        }

        Builder& screen_power(int s)
        {
            _power_s = s;
            return *this;
        }

        Builder& tube_power(int t)
        {
            _power_t = t;
            return *this;
        }

        Builder& missiles(const std::string& m)
        {
            _missiles_json = m;
            return *this;
        }

        ICmd* build()
        {
            return new CombatOrderCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    CombatOrderCommand(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_target_id(std::move(builder._target_id)), m_tactic(builder._tactic),
          m_power_d(builder._power_d), m_power_b(builder._power_b),
          m_power_s(builder._power_s), m_power_t(builder._power_t),
          m_missiles_json(std::move(builder._missiles_json))
    {
    }
};

#endif
