//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __OUTFIT_COMMAND_H__
#define __OUTFIT_COMMAND_H__

#include <string>

#include "icmd.h"

// OutfitCommand - Purchase equipment for ships using Credits
// Usage: outfit list
//        outfit <ship> lrs
//        outfit <ship> tb
//        outfit <ship> drones
class OutfitCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_ship_code(""), m_equipment("")
        {
        }
        Builder& ship(const std::string& s)
        {
            m_ship_code = s;
            return *this;
        }
        Builder& equipment(const std::string& e)
        {
            m_equipment = e;
            return *this;
        }
        OutfitCommand* build()
        {
            return new OutfitCommand(m_ship_code, m_equipment);
        }

      private:
        std::string m_ship_code;
        std::string m_equipment;
    };

    bool invoke(void) override;

  private:
    OutfitCommand(const std::string& ship, const std::string& equip)
        : m_ship_code(ship), m_equipment(equip)
    {
    }
    std::string m_ship_code;
    std::string m_equipment;

    void show_equipment();
    bool do_outfit();
};

#endif
