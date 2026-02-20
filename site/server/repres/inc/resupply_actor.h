///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_RESUPPLY_ACTOR_H__
#define __KH_RESUPPLY_ACTOR_H__

#include <string>

#include "icmd.h"
#include "resupply_modes.h"

class ResupplyActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder();
        Builder& set_list_mode();
        Builder& set_ship_code(const std::string& code);
        Builder& set_torpedoes(int n);
        ICmd* build();

      private:
        ResupplyMode m_mode;
        std::string m_ship_code;
        int m_torpedoes;
    };

    bool invoke(void) override;

  private:
    ResupplyActor(ResupplyMode mode, const std::string& ship, int torpedoes);

    ResupplyMode m_mode;
    std::string m_ship_code;
    int m_torpedoes;
};

#endif
