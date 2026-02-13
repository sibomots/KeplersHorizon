///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_GAMEDEV_COMMAND_H__
#define __KH_GAMEDEV_COMMAND_H__

#include <string>

#include "icmd.h"

/**
 * GameDevCommand - Debug commands for game development testing.
 *
 * Commands:
 *   gamedev               - Show current settings
 *   gamedev reset         - Reset all to defaults
 *   gamedev environment N - Set environment effect probability (0-100%)
 *   gamedev combat N      - Set combat modifier override (-N to +N)
 *   gamedev movement N    - Set movement modifier override (-N to +N)
 *   gamedev crt N         - Force CRT roll result (1-12, 0=natural)
 *   gamedev vp N          - Set VP for current player (0-10, testing)
 */
class GameDevCommand : public ICmd
{
  public:
    enum SubCommand
    {
        GD_STATUS,
        GD_RESET,
        GD_ENVIRONMENT,
        GD_COMBAT,
        GD_MOVEMENT,
        GD_CRT,
        GD_VP
    };

    class Builder
    {
      public:
        SubCommand _subcmd;
        int _value;

        Builder() : _subcmd(GD_STATUS), _value(0)
        {
        }

        Builder& status()
        {
            _subcmd = GD_STATUS;
            return *this;
        }

        Builder& reset()
        {
            _subcmd = GD_RESET;
            return *this;
        }

        Builder& environment(int pct)
        {
            _subcmd = GD_ENVIRONMENT;
            _value = pct;
            return *this;
        }

        Builder& combat(int mod)
        {
            _subcmd = GD_COMBAT;
            _value = mod;
            return *this;
        }

        Builder& movement(int mod)
        {
            _subcmd = GD_MOVEMENT;
            _value = mod;
            return *this;
        }

        Builder& crt(int result)
        {
            _subcmd = GD_CRT;
            _value = result;
            return *this;
        }

        Builder& vp(int points)
        {
            _subcmd = GD_VP;
            _value = points;
            return *this;
        }

        ICmd* build()
        {
            return new GameDevCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    GameDevCommand(Builder& builder)
        : m_subcmd(builder._subcmd), m_value(builder._value)
    {
    }

    SubCommand m_subcmd;
    int m_value;
};

#endif
