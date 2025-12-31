//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_COMMAND_H__
#define __BUILD_COMMAND_H__

#include <string>

#include "icmd.h"
#include "typedefs.h"

class BuildCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _draft_code;

        Builder& set_draft_code(const std::string& code)
        {
            _draft_code = code;
            return *this;
        }

        ICmd* build()
        {
            return new BuildCommand(*this);
        }
    };

  private:
    BuildCommand(Builder& builder)
        : m_draft_code(std::move(builder._draft_code))
    {
    }

    std::string m_draft_code;

  public:
    virtual bool invoke(void);
};

#endif
