//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_NEW_ACTOR_H__
#define __BUILD_NEW_ACTOR_H__

#include <string>

#include "icmd.h"

class BuildNewActor : public ICmd
{
  private:
    std::string m_ship_code;
    std::string m_ship_name;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _ship_name;

        Builder() = default;

        Builder& set_ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }

        Builder& set_ship_name(const std::string& name)
        {
            _ship_name = name;
            return *this;
        }

        ICmd* build()
        {
            return new BuildNewActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    BuildNewActor(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_ship_name(std::move(builder._ship_name))
    {
    }
};

#endif
