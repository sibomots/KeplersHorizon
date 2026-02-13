///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_HEX_COMMAND_H__
#define __KH_HEX_COMMAND_H__

#include <string>

#include "icmd.h"

class HexCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& setLocation(const std::string& loc)
        {
            m_location = loc;
            return *this;
        }

        ICmd* build()
        {
            return new HexCommand(m_location);
        }

      private:
        std::string m_location;
    };

    bool invoke(void) override;

  private:
    std::string m_location;

    HexCommand(const std::string& loc) : m_location(loc)
    {
    }
};

#endif
