//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __DONE_COMMAND_H__
#define __DONE_COMMAND_H__

#include "db.h"
#include "icmd.h"
#include "statemachine.h"

class DoneCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder(StateMachine &sm);
        ICmd *build();

      private:
        StateMachine &m_sm;
    };

    bool invoke(void) override;

  private:
    DoneCommand(StateMachine &sm);

    StateMachine &m_sm;
};

#endif
