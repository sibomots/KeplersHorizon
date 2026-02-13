///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SURVEY_ACTOR_H__
#define __KH_SURVEY_ACTOR_H__

#include <string>

#include "icmd.h"
#include "milieuagent.h"
#include "survey_strategy.h"

// SurveyActor - Upgrade player's Milieu Codex knowledge of a star system
// Requires a ship to be present in the target system
class SurveyActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_system_name(const std::string& _system_name)
        {
            system_name = _system_name;
            return *this;
        }
        Builder& set_mode(const SurveyMode& _mode)
        {
            mode = _mode;
            return *this;
        }

        ICmd* build()
        {
            return new SurveyActor(system_name, mode);
        }

      private:
        std::string system_name;
        SurveyMode mode;
    };

    bool invoke(void) override;

    std::string get_system_name() const
    {
        return m_system_name;
    }
    SurveyMode get_mode() const
    {
        return m_mode;
    }

  private:
    SurveyActor(const std::string& system, const SurveyMode mode)
        : m_system_name(system), m_mode(mode)
    {
    }

    std::string m_system_name;
    SurveyMode m_mode;
};

#endif
