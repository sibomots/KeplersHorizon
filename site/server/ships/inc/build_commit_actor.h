///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_BUILD_COMMIT_ACTOR_H__
#define __KH_BUILD_COMMIT_ACTOR_H__

#include <string>

#include "icmd.h"

class BuildCommitActor : public ICmd
{
  private:
    std::string m_target;

  public:
    class Builder
    {
      public:
        std::string _target;

        Builder() = default;

        Builder& set_target(const std::string& target)
        {
            _target = target;
            return *this;
        }

        ICmd* build()
        {
            return new BuildCommitActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    BuildCommitActor(Builder& builder) : m_target(std::move(builder._target))
    {
    }
};

#endif
