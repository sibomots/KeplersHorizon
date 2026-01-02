//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COMBAT_CANCEL_COMMAND_H__
#define __COMBAT_CANCEL_COMMAND_H__

#include "icmd.h"

class CombatCancelCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() {}
        ICmd* build() { return new CombatCancelCommand(); }
    };

    bool invoke(void) override;

  private:
    CombatCancelCommand() {}
};

#endif
