///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_NEXT_COMMAND_H__
#define __KH_NEXT_COMMAND_H__

#include "db.h"
#include "icmd.h"
#include "statemachine.h"

class NextCommand : public ICmd
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
            return new NextCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    NextCommand(Builder& builder)
    {
    }
};

#endif
