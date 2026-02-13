///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SYSTEM_ACTOR_H__
#define __KH_SYSTEM_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"
#include "system_strategy.h"

// SystemActor - Display information about a star system
// Information is filtered based on player's Milieu Codex knowledge level

class SystemActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_system_name(const std::string& _name)
        {
            system_name = _name;
            return *this;
        }
        Builder& set_anomalies_mode(void)
        {
            mode = SystemMode::SYS_ANOMALIES;
            return *this;
        }
        Builder& set_facilities_mode(void)
        {
            mode = SystemMode::SYS_FACILITIES;
            return *this;
        }
        Builder& set_resources_mode(void)
        {
            mode = SystemMode::SYS_RESOURCES;
            return *this;
        }
        Builder& set_planets_mode(void)
        {
            mode = SystemMode::SYS_PLANETS;
            return *this;
        }
        Builder& set_show_mode(void)
        {
            mode = SystemMode::SYS_SHOW_SYSTEM;
            return *this;
        }

        ICmd* build()
        {
            return new SystemActor(system_name, mode);
        }

      private:
        std::string system_name;
        SystemMode mode;
    };

    bool invoke(void) override;
    std::string get_system_name(void) const
    {
        return m_system_name;
    }
    bool get_anomalies_mode(void) const
    {
        return (KH_EQU(m_mode, SystemMode::SYS_ANOMALIES));
    }
    bool get_facilities_mode(void) const
    {
        return (KH_EQU(m_mode, SystemMode::SYS_FACILITIES));
    }
    bool get_resources_mode(void) const
    {
        return (KH_EQU(m_mode, SystemMode::SYS_RESOURCES));
    }
    bool get_planets_mode(void) const
    {
        return (KH_EQU(m_mode, SystemMode::SYS_PLANETS));
    }
    bool get_show_mode(void) const
    {
        return (KH_EQU(m_mode, SystemMode::SYS_SHOW_SYSTEM));
    }
    SystemMode get_mode(void) const
    {
        return m_mode;
    }

  private:
    SystemActor(const std::string& name, const SystemMode& mode)
        : m_system_name(name), m_mode(mode)
    {
    }

    // Get player's knowledge level for this system
    std::string get_knowledge_level();
    int knowledge_rank(const std::string& level);

    std::string m_system_name;
    SystemMode m_mode;
};

#endif
