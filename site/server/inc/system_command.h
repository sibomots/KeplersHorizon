//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __SYSTEM_COMMAND_H__
#define __SYSTEM_COMMAND_H__

#include <string>

#include "icmd.h"

// SystemCommand - Display information about a star system
// Information is filtered based on player's grimoire knowledge level
class SystemCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& setName(const std::string& name)
        {
            m_name = name;
            return *this;
        }
        Builder& setSubcommand(const std::string& sub)
        {
            m_subcommand = sub;
            return *this;
        }
        ICmd* build()
        {
            return new SystemCommand(m_name, m_subcommand);
        }

      private:
        std::string m_name;
        std::string m_subcommand;
    };

    bool invoke(void) override;

  private:
    SystemCommand(const std::string& name, const std::string& subcmd)
        : m_system_name(name), m_subcommand(subcmd)
    {
    }

    // Helper methods for subcommands
    void show_overview();
    void show_planets();
    void show_resources();
    void show_populations();
    void show_facilities();
    void show_anomalies();

    // Get player's knowledge level for this system
    std::string get_knowledge_level();
    int knowledge_rank(const std::string& level);

    std::string m_system_name;
    std::string m_subcommand;
};

#endif
