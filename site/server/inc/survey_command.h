//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __SURVEY_COMMAND_H__
#define __SURVEY_COMMAND_H__

#include <string>

#include "icmd.h"

// SurveyCommand - Upgrade player's grimoire knowledge of a star system
// Requires a ship to be present in the target system
class SurveyCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& setSystem(const std::string& name)
        {
            m_system = name;
            return *this;
        }
        ICmd* build()
        {
            return new SurveyCommand(m_system);
        }

      private:
        std::string m_system;
    };

    bool invoke(void) override;

  private:
    SurveyCommand(const std::string& system) : m_system_name(system)
    {
    }

    // Check if player has a ship in the given system
    bool has_ship_in_system(const std::string& system);

    // Upgrade grimoire knowledge level
    std::string upgrade_knowledge(const std::string& current);

    std::string m_system_name;
};

#endif
