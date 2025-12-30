//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __NEXT_COMMAND_H__
#define __NEXT_COMMAND_H__

#include "db.h"
#include "icmd.h"
#include "statemachine.h"

class NextCommand : public ICmd
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
    NextCommand(StateMachine &sm);

    StateMachine &m_sm;
};

#endif
