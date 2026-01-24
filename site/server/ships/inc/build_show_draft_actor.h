//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_SHOW_DRAFT_ACTOR_H__
#define __BUILD_SHOW_DRAFT_ACTOR_H__

#include <string>
#include "icmd.h"

class BuildShowDraftActor : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _target;
        Builder()
        {
        }

        Builder& set_target(const std::string& target)
        {
            _target = target;
            return *this;
        }

        ICmd* build()
        {
            return new BuildShowDraftActor(*this);
        }
    };

    bool invoke(void) override;

    std::string get_target() const {
         return m_target;
    }
  private:
    BuildShowDraftActor(Builder& builder)
        : m_target(std::move(builder._target))
    {
    }
    std::string m_target;
};

#endif
