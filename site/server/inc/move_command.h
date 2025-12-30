//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MOVE_COMMAND_H__
#define __MOVE_COMMAND_H__

#include <string>
#include <vector>

#include "db.h"
#include "icmd.h"
#include "statemachine.h"

class MoveCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder(StateMachine &sm);
        Builder &ship_code(const std::string &code);
        Builder &add_destination(const std::string &dest);
        ICmd *build();

      private:
        StateMachine &m_sm;
        std::string m_ship_code;
        std::vector<std::string> m_destinations;
    };

    bool invoke(void) override;

  private:
    MoveCommand(StateMachine &sm, const std::string &ship_code,
                const std::vector<std::string> &destinations);

    StateMachine &m_sm;
    std::string m_ship_code;
    std::vector<std::string> m_destinations;
};

#endif
