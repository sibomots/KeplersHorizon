//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __DELETE_COMMAND_H__
#define __DELETE_COMMAND_H__

#include <string>

#include "icmd.h"

class DeleteCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& setSaveName(const std::string& name)
        {
            m_save_name = name;
            return *this;
        }

        ICmd* build()
        {
            return new DeleteCommand(m_save_name);
        }

      private:
        std::string m_save_name;
    };

    bool invoke(void) override;

  private:
    std::string m_save_name;

    DeleteCommand(const std::string& name) : m_save_name(name)
    {
    }
};

#endif
