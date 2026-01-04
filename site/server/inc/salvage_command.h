//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __SALVAGE_COMMAND_H__
#define __SALVAGE_COMMAND_H__

#include <string>

#include "icmd.h"

// SalvageCommand - Salvage wreckage at The Graveyard (UMMA) or similar sites
// Usage: salvage <ship>
class SalvageCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_ship_code("") {}
        Builder& ship(const std::string& code)
        {
            m_ship_code = code;
            return *this;
        }
        SalvageCommand* build() { return new SalvageCommand(m_ship_code); }

      private:
        std::string m_ship_code;
    };

    bool invoke(void) override;

  private:
    SalvageCommand(const std::string& ship) : m_ship_code(ship) {}
    std::string m_ship_code;
};

#endif
