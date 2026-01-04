//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __EXTRACT_COMMAND_H__
#define __EXTRACT_COMMAND_H__

#include <string>

#include "icmd.h"

// ExtractCommand - Extract resources from current location
// Usage: extract scan           -- Requires LRS to scan for resources
//        extract <ship> <res>   -- Requires TB or DR to extract
class ExtractCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_ship_code(""), m_resource_type(""), m_scan_mode(false) {}
        Builder& ship(const std::string& code)
        {
            m_ship_code = code;
            return *this;
        }
        Builder& resource(const std::string& res)
        {
            m_resource_type = res;
            return *this;
        }
        Builder& scanMode()
        {
            m_scan_mode = true;
            return *this;
        }
        ExtractCommand* build()
        {
            return new ExtractCommand(m_ship_code, m_resource_type, m_scan_mode);
        }

      private:
        std::string m_ship_code;
        std::string m_resource_type;
        bool m_scan_mode;
    };

    bool invoke(void) override;

  private:
    ExtractCommand(const std::string& ship, const std::string& res, bool scan)
        : m_ship_code(ship), m_resource_type(res), m_scan_mode(scan)
    {
    }
    std::string m_ship_code;
    std::string m_resource_type;
    bool m_scan_mode;

    void do_scan();
    bool do_extract();
};

#endif
