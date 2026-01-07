//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __CARGO_COMMAND_H__
#define __CARGO_COMMAND_H__

#include <string>

#include "icmd.h"

class CargoCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& ship(const std::string& code)
        {
            m_ship_code = code;
            return *this;
        }
        ICmd* build()
        {
            return new CargoCommand(m_ship_code);
        }

      private:
        std::string m_ship_code;
    };

    bool invoke(void) override;

  private:
    CargoCommand(const std::string& ship_code) : m_ship_code(ship_code)
    {
    }
    std::string m_ship_code;
};

#endif
