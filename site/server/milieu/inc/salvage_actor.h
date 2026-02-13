///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SALVAGE_ACTOR_H__
#define __KH_SALVAGE_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"
#include "salvage_strategy.h"

class SalvageActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : mode(SalvageMode::SALVAGE_SCAN)
        {
        }
        Builder& set_scan_mode()
        {
            mode = SalvageMode::SALVAGE_SCAN;
            return *this;
        }
        Builder& set_salvage_mode()
        {
            mode = SalvageMode::SALVAGE_OPERATION;
            return *this;
        }
        Builder& set_resource(const std::string& _resource)
        {
            resource = _resource;
            return *this;
        }
        Builder& set_ship_code(std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }
        ICmd* build()
        {
            return new SalvageActor(mode, ship_code, resource);
        }

      private:
        SalvageMode mode;
        std::string resource;
        std::string ship_code;
    };

    bool invoke(void) override;

    SalvageMode get_salvage_mode() const
    {
        return m_mode;
    }
    std::string get_ship_code() const
    {
        return m_ship_code;
    }
    std::string get_resource() const
    {
        return m_resource;
    }

  private:
    SalvageActor(SalvageMode mode, const std::string& ship_code,
                 const std::string& resource)
        : m_mode(mode), m_resource(resource), m_ship_code(ship_code)
    {
    }
    SalvageMode m_mode;
    std::string m_resource;
    std::string m_ship_code;
};

#endif
