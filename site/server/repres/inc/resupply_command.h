///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_RESUPPLY_COMMAND_H__
#define __KH_RESUPPLY_COMMAND_H__

#include <string>

#include "icmd.h"
#include "typedefs.h"

class ResupplyCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string ship_code;
        int missiles = 0;

        Builder& set_ship_code(const std::string& code)
        {
            ship_code = code;
            return *this;
        }

        Builder& set_missiles(int m)
        {
            missiles = m;
            return *this;
        }

        ICmd* build()
        {
            return new ResupplyCommand(*this);
        }
    };

  private:
    ResupplyCommand(Builder& builder)
        : m_ship_code(std::move(builder.ship_code)),
          m_missiles(std::move(builder.missiles))
    {
    }

    std::string m_ship_code;
    int m_missiles;

  public:
    virtual bool invoke(void);
};

#endif
