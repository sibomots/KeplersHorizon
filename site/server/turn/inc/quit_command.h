///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_QUIT_COMMAND_H__
#define __KH_QUIT_COMMAND_H__

#include "icmd.h"

class QuitCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new QuitCommand();
        }
    };

    bool invoke(void) override;

  private:
    QuitCommand()
    {
    }
};

#endif
