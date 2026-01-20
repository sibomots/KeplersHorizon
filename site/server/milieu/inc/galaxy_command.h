//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __GALAXY_COMMAND_H__
#define __GALAXY_COMMAND_H__

#include "icmd.h"

class GalaxyCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new GalaxyCommand();
        }
    };

    bool invoke(void) override;

  private:
    GalaxyCommand()
    {
    }
};

#endif
