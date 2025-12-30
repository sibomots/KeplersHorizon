//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_LIST_DRAFTS_COMMAND_H__
#define __BUILD_LIST_DRAFTS_COMMAND_H__

#include "icmd.h"
#include "statemachine.h"

class BuildListDraftsCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        StateMachine &_sm;

        Builder(StateMachine &sm) : _sm(sm)
        {
        }

        ICmd *build()
        {
            return new BuildListDraftsCommand(_sm);
        }
    };

  private:
    BuildListDraftsCommand(StateMachine &sm) : m_sm(sm)
    {
    }

    StateMachine &m_sm;

  public:
    virtual bool invoke(void);
};

#endif
