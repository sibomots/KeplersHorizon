///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_EXTRACT_ACTOR_H__
#define __KH_EXTRACT_ACTOR_H__

#include <string>

#include "extract_strategy.h"
#include "icmd.h"
#include "milieuagent.h"

// ExtractActor - Extract resources from current location
// Usage: extract scan           -- Requires LRS to scan for resources
//        extract <ship> <res>   -- Requires TB or DR to extract

class ExtractActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : mode(ExtractMode::EXT_SCAN)
        {
        }
        Builder& set_ship_code(const std::string& _code)
        {
            ship_code = _code;
            return *this;
        }
        Builder& set_resource(const std::string& _resource)
        {
            resource = _resource;
            return *this;
        }
        Builder& set_scan_mode()
        {
            mode = ExtractMode::EXT_SCAN;
            return *this;
        }
        Builder& set_extract_mode()
        {
            mode = ExtractMode::EXT_EXTRACT;
            return *this;
        }
        ExtractActor* build()
        {
            return new ExtractActor(ship_code, resource, mode);
        }

      private:
        std::string ship_code;
        std::string resource;
        ExtractMode mode;
    };

    bool invoke(void) override;

    bool get_scan_mode(void) const
    {
        return KH_EQU(m_mode, ExtractMode::EXT_SCAN);
    }

    bool get_extract_mode(void) const
    {
        return KH_EQU(m_mode, ExtractMode::EXT_EXTRACT);
    }

    std::string get_resource(void) const
    {
        return m_resource;
    }

    std::string get_ship_code(void) const
    {
        return m_ship_code;
    }

  private:
    ExtractActor(const std::string& ship, const std::string& res,
                 ExtractMode& mode)
        : m_ship_code(ship), m_resource(res), m_mode(mode)
    {
    }
    std::string m_ship_code;
    std::string m_resource;
    ExtractMode m_mode;
};

#endif
