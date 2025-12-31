//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __START_COMMAND_H__
#define __START_COMMAND_H__

#include "db.h"
#include "icmd.h"
#include "typedefs.h"

class StartCommand : public ICmd
{
  private:
    ScenarioType m_scenario;
    int m_game_id;

  public:
    class Builder
    {
      public:
        ScenarioType scenario = ScenarioType::UNDEFINED;
        int game_id;
        Builder& set_scenario(ScenarioType typ)
        {
            scenario = std::move(typ);
            return *this;
        }
        Builder& set_game_id(int id)
        {
            game_id = id;
            return *this;
        }
        ICmd* build()
        {
            return new StartCommand(*this);
        }
    };

  private:
    StartCommand(Builder& builder)
        : m_scenario(std::move(builder.scenario)),
          m_game_id(std::move(builder.game_id))
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
