//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_SET_ATTRIBUTE_COMMAND_H__
#define __BUILD_SET_ATTRIBUTE_COMMAND_H__

#include "icmd.h"
#include "statemachine.h"
#include "typedefs.h"

class BuildSetAttributeCommand : public ICmd
{
  private:
    AttributeMap m_attributes;

  public:
    class Builder
    {
      public:
        AttributeMap _attributes;

        Builder()
        {
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
            return new BuildSetAttributeCommand(*this);
        }
    };

  private:
    BuildSetAttributeCommand(Builder& builder)
        : m_attributes(std::move(builder._attributes))
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
