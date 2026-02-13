///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMBAT_COMMIT_ACTOR_H__
#define __KH_COMBAT_COMMIT_ACTOR_H__

#include "icmd.h"

class CombatCommitActor : public ICmd
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
            return new CombatCommitActor;
        }
    };

    bool invoke(void) override;

  private:
    CombatCommitActor()
    {
    }
};

#endif
