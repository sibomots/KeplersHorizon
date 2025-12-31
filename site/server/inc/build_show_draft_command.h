//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_SHOW_DRAFT_COMMAND_H__
#define __BUILD_SHOW_DRAFT_COMMAND_H__

#include <string>

#include "icmd.h"
#include "statemachine.h"

class BuildShowDraftCommand : public ICmd
{
  private:
    std::string m_draft_code;

  public:
    class Builder
    {
      public:
        std::string _draft_code;

        Builder()
        {
        }
        Builder& set_draft_code(const std::string& code)
        {
            _draft_code = code;
            return *this;
        }

        ICmd* build()
        {
            return new BuildShowDraftCommand(*this);
        }
    };

  private:
    BuildShowDraftCommand(Builder& builder)
        : m_draft_code(std::move(builder._draft_code))
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
