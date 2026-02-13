///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMBAT_CANCEL_ACTOR_H__
#define __KH_COMBAT_CANCEL_ACTOR_H__

#include "icmd.h"

class CombatCancelActor : public ICmd
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
            return new CombatCancelActor;
        }
    };

    bool invoke(void) override;

  private:
    CombatCancelActor()
    {
    }
};

#endif
