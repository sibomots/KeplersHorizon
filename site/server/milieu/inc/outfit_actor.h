///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_OUTFIT_ACTOR_H__
#define __KH_OUTFIT_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"
#include "outfit_modes.h"
#include "outfit_strategy.h"

// OutfitActor - Purchase equipment for ships using Credits
// Usage: outfit list
//        outfit <ship> lrs
//        outfit <ship> drones
class OutfitActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_mode(OutfitMode::OUTFIT_LIST)
        {
        }
        Builder& set_list(void)
        {
            m_mode = OutfitMode::OUTFIT_LIST;
            return *this;
        }
        Builder& set_lrs(void)
        {
            m_mode = OutfitMode::OUTFIT_LRS;
            return *this;
        }
        Builder& set_drones(void)
        {
            m_mode = OutfitMode::OUTFIT_DRONES;
            return *this;
        }
        Builder& set_ship(const std::string& ship)
        {
            m_ship_code = ship;
            return *this;
        }
        ICmd* build()
        {
            return new OutfitActor(m_mode, m_ship_code);
        }

      private:
        OutfitMode m_mode;
        std::string m_ship_code;
    };

    bool invoke(void) override;

  private:
    OutfitActor(OutfitMode mode, const std::string& ship)
        : m_mode(mode), m_ship_code(ship)
    {
    }
    OutfitMode m_mode;
    std::string m_ship_code;
};

#endif
