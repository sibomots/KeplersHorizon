//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_CANCEL_ACTOR_H__
#define __BUILD_CANCEL_ACTOR_H__

#include <string>
#include "icmd.h"

class BuildCancelActor : public ICmd
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
            return new BuildCancelActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    BuildCancelActor(Builder& builder)
        : m_target(std::move(builder._target))
    {
    }
};

#endif
