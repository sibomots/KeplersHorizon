//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __FLEET_COMMAND_H__
#define __FLEET_COMMAND_H__

#include "icmd.h"

class FleetCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build() { return new FleetCommand(); }
    };

    bool invoke(void) override;

  private:
    FleetCommand() {}
};

#endif
