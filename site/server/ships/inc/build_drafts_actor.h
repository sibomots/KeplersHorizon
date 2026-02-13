///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_BUILD_DRAFTS_ACTOR_H__
#define __KH_BUILD_DRAFTS_ACTOR_H__

#include <string>

#include "icmd.h"

class BuildDraftsActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() = default;

        ICmd* build()
        {
            return new BuildDraftsActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    BuildDraftsActor(Builder& builder)
    {
        // No members - just lists all drafts for player
    }
};

#endif
