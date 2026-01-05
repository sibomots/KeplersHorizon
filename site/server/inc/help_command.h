//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __HELP_COMMAND_H__
#define __HELP_COMMAND_H__

#include <string>
#include "icmd.h"

class HelpCommand : public ICmd
{
  public:
    std::string m_topic;
    bool        m_demo;

    class Builder
    {
      public:
        std::string _topic;
        bool        _demo;
        Builder() {
           _demo = false;
        }
        Builder& set_demo()
        {
           _demo = true;
           return  *this;
        }
        Builder& set_topic(std::string topik)
        {
           _topic = topik;
           return *this;
        }  
        ICmd* build()
        {
            return new HelpCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    HelpCommand(Builder& builder): m_topic(builder._topic), m_demo(builder._demo)
    {
    }
};

#endif
