///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_DONE_COMMAND_H__
#define __KH_DONE_COMMAND_H__

#include "db.h"
#include "icmd.h"

class DoneCommand : public ICmd
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
            return new DoneCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    DoneCommand(Builder& builder)
    {
    }
};

#endif
