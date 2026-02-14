///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_LICENSE_COMMAND_H__
#define __KH_LICENSE_COMMAND_H__

#include "icmd.h"

class LicenseCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ICmd* build()
        {
            return new LicenseCommand();
        }
    };

    bool invoke(void) override;

  private:
    LicenseCommand()
    {
    }
};

#endif
