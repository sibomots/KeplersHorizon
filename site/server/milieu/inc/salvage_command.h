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

// SalvageCommand - Salvage wreckage from discovered salvageables
// Usage: salvage scan
//        salvage <ship>
//        salvage <ship> <target_name>
class SalvageCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_ship_code(""), m_target_name(""), m_scan_mode(false)
        {
        }
        Builder& ship(const std::string& code)
        {
            m_ship_code = code;
            return *this;
        }
        Builder& target(const std::string& name)
        {
            m_target_name = name;
            return *this;
        }
        Builder& scan()
        {
            m_scan_mode = true;
            return *this;
        }
        SalvageCommand* build()
        {
            return new SalvageCommand(m_ship_code, m_target_name, m_scan_mode);
        }

      private:
        std::string m_ship_code;
        std::string m_target_name;
        bool m_scan_mode;
    };

    bool invoke(void) override;

  private:
    SalvageCommand(const std::string& ship, const std::string& target,
                   bool scan)
        : m_ship_code(ship), m_target_name(target), m_scan_mode(scan)
    {
    }
    std::string m_ship_code;
    std::string m_target_name;
    bool m_scan_mode;

    void do_scan();
    bool do_salvage();
};

#endif
