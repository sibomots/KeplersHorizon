//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_CANCEL_COMMAND_H__
#define __BUILD_CANCEL_COMMAND_H__

#include "icmd.h"
#include "statemachine.h"

class BuildCancelCommand : public ICmd
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
            return new BuildCancelCommand(_sm);
        }
    };

  private:
    BuildCancelCommand(StateMachine &sm) : m_sm(sm)
    {
    }

    StateMachine &m_sm;

  public:
    virtual bool invoke(void);
};

#endif
