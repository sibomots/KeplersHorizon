///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_CARGO_ACTOR_H__
#define __KH_CARGO_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"

class CargoActor : public ICmd
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
            return new CargoActor(m_ship_code);
        }

      private:
        std::string m_ship_code;
    };

    bool invoke(void) override;

  private:
    CargoActor(const std::string& ship_code) : m_ship_code(ship_code)
    {
    }
    std::string m_ship_code;
};

#endif
