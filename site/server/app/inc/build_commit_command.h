//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_COMMIT_COMMAND_H__
#define __BUILD_COMMIT_COMMAND_H__

#include "icmd.h"
#include "statemachine.h"

class BuildCommitCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder()
        {
        }

        ICmd* build()
        {
            return new BuildCommitCommand(*this);
        }
    };

  private:
    BuildCommitCommand(Builder& builder)
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
