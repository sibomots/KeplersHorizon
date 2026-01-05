//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __HELP_COMMAND_H__
#define __HELP_COMMAND_H__

#include "icmd.h"

class HelpCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new HelpCommand();
        }
    };

    bool invoke(void) override;

  private:
    HelpCommand()
    {
    }
};

#endif
