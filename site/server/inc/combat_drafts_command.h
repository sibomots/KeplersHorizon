//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COMBAT_DRAFTS_COMMAND_H__
#define __COMBAT_DRAFTS_COMMAND_H__

#include "icmd.h"

class CombatDraftsCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() {}
        ICmd* build() { return new CombatDraftsCommand(); }
    };

    bool invoke(void) override;

  private:
    CombatDraftsCommand() {}
};

#endif
