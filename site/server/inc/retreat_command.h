//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __RETREAT_COMMAND_H__
#define __RETREAT_COMMAND_H__

#include <string>

#include "icmd.h"

/**
 * RetreatCommand - Move a ship to an adjacent hex after escape/stalemate.
 *
 * Usage: retreat <ship_code> <destination_hex>
 *
 * Requirements:
 * - Ship must have escape_pending flag set (from combat escape or stalemate)
 * - Destination must be adjacent to ship's current position
 * - No PD cost for retreat movement
 */
class RetreatCommand : public ICmd
{
  public:
    RetreatCommand(const std::string& ship_code, const std::string& dest_hex)
        : m_ship_code(ship_code), m_dest_hex(dest_hex)
    {
    }

    bool invoke(void) override;

  private:
    std::string m_ship_code;
    std::string m_dest_hex;
};

#endif
