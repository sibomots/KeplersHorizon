//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __DEPLOY_COMMAND_H__
#define __DEPLOY_COMMAND_H__

#include <string>

#include "db.h"
#include "icmd.h"
#include "statemachine.h"

class DeployCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder(StateMachine &sm);
        Builder &ship_code(const std::string &code);
        Builder &system_name(const std::string &sys);
        ICmd *build();

      private:
        StateMachine &m_sm;
        std::string m_ship_code;
        std::string m_system_name;
    };

    bool invoke(void) override;

  private:
    DeployCommand(StateMachine &sm, const std::string &ship_code,
                  const std::string &system_name);

    StateMachine &m_sm;
    std::string m_ship_code;
    std::string m_system_name;
};

#endif
