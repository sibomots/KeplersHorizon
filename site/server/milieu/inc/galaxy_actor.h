///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_GALAXY_ACTOR_H__
#define __KH_GALAXY_ACTOR_H__

#include "icmd.h"
#include "milieuagent.h"

class GalaxyActor : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new GalaxyActor();
        }
    };

    bool invoke(void) override;

  private:
    GalaxyActor()
    {
    }
};

#endif
