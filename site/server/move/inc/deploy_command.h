///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_DEPLOY_COMMAND_H__
#define __KH_DEPLOY_COMMAND_H__

#include <string>

#include "icmd.h"

class DeployCommand : public ICmd
{
  private:
    std::string m_ship_code;
    std::string m_system_name;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _system_name;

        Builder()
        {
        }

        Builder& target(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }

        Builder& system(const std::string& sys)
        {
            _system_name = sys;
            return *this;
        }

        ICmd* build()
        {
            return new DeployCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    DeployCommand(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_system_name(std::move(builder._system_name))
    {
    }
};

#endif
