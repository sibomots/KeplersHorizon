//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_SET_ACTOR_H__
#define __BUILD_SET_ACTOR_H__

#include <string>
#include "attributemap.h"
#include "icmd.h"

class BuildSetActor : public ICmd
{
  private:
    std::string m_target;
    AttributeMap m_attributes;

  public:
    class Builder
    {
      public:
        std::string _target;
        AttributeMap _attributes;

        Builder() = default;

        Builder& set_target(const std::string& target)
        {
            _target = target;
            return *this;
        }

        Builder& set_attributes(const AttributeMap& attrs)
        {
            _attributes = attrs;
            return *this;
        }

        Builder& set_pd(int val)
        {
            _attributes[AttributeID::POWER_DRIVE] = val;
            return *this;
        }

        Builder& set_b(int val)
        {
            _attributes[AttributeID::BEAM] = val;
            return *this;
        }

        Builder& set_s(int val)
        {
            _attributes[AttributeID::SCREEN] = val;
            return *this;
        }

        Builder& set_t(int val)
        {
            _attributes[AttributeID::TUBE] = val;
            return *this;
        }

        Builder& set_m(int val)
        {
            _attributes[AttributeID::MISSILE] = val;
            return *this;
        }

        Builder& set_sr(int val)
        {
            _attributes[AttributeID::SYSTEM_RACK] = val;
            return *this;
        }

        ICmd* build()
        {
            return new BuildSetActor(*this);
        }
    };

    bool invoke(void) override;

  private:
    BuildSetActor(Builder& builder)
        : m_target(std::move(builder._target)),
          m_attributes(std::move(builder._attributes))
    {
    }
};

#endif
