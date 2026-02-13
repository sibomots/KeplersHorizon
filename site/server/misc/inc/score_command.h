///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SCORE_COMMAND_H__
#define __KH_SCORE_COMMAND_H__

#include <string>

#include "icmd.h"

// ScoreCommand - Displays essential game status
// Usage: score
class ScoreCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder()
        {
        }
        ScoreCommand* build()
        {
            return new ScoreCommand();
        }
    };

    bool invoke(void) override;

  private:
    ScoreCommand()
    {
    }
    void show_overview();
};

#endif
