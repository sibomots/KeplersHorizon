///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_FLEET_LIST_ACTOR_H__
#define __KH_FLEET_LIST_ACTOR_H__

#include "icmd.h"

class BuildFleetListActor : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new BuildFleetListActor();
        }
    };

    bool invoke(void) override;

  private:
    BuildFleetListActor()
    {
    }
};

#endif
