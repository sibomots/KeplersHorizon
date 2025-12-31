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
        Builder()
        {
        }

        ICmd* build()
        {
            return new BuildListDraftsCommand(*this);
        }
    };

  private:
    BuildListDraftsCommand(Builder& builder)
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
