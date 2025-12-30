//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_SHOW_DRAFT_COMMAND_H__
#define __BUILD_SHOW_DRAFT_COMMAND_H__

#include <string>

#include "icmd.h"
#include "statemachine.h"

class BuildShowDraftCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        StateMachine &_sm;
        std::string _draft_code;

        Builder(StateMachine &sm) : _sm(sm)
        {
        }
        Builder &set_draft_code(const std::string &code)
        {
            _draft_code = code;
            return *this;
        }

        ICmd *build()
        {
            return new BuildShowDraftCommand(_sm, _draft_code);
        }
    };

  private:
    BuildShowDraftCommand(StateMachine &sm, const std::string &draft_code)
        : m_sm(sm), m_draft_code(draft_code)
    {
    }

    StateMachine &m_sm;
    std::string m_draft_code;

  public:
    virtual bool invoke(void);
};

#endif
