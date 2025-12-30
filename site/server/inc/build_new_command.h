//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_NEW_COMMAND_H__
#define __BUILD_NEW_COMMAND_H__

#include <string>

#include "icmd.h"
#include "statemachine.h"

class BuildNewCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        StateMachine &_sm;
        std::string _ship_code;
        std::string _ship_name;

        Builder(StateMachine &sm) : _sm(sm)
        {
        }
        Builder &set_ship_code(const std::string &code)
        {
            _ship_code = code;
            return *this;
        }
        Builder &set_ship_name(const std::string &name)
        {
            _ship_name = name;
            return *this;
        }

        ICmd *build()
        {
            return new BuildNewCommand(*this);
        }
    };

  private:
    BuildNewCommand(Builder &builder)
        : m_sm(builder._sm), m_ship_code(std::move(builder._ship_code)),
          m_ship_name(std::move(builder._ship_name))
    {
    }

    StateMachine &m_sm;
    std::string m_ship_code;
    std::string m_ship_name;

  public:
    virtual bool invoke(void);
};

#endif
