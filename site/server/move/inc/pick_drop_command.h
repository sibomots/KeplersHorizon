///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_PICK_DROP_COMMAND_H__
#define __KH_PICK_DROP_COMMAND_H__

#include <string>

#include "icmd.h"

class PickCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _systemship_code;
        std::string _warpship_code;

        Builder()
        {
        }
        Builder& systemship(const std::string& code)
        {
            _systemship_code = code;
            return *this;
        }
        Builder& warpship(const std::string& code)
        {
            _warpship_code = code;
            return *this;
        }
        ICmd* build()
        {
            return new PickCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    PickCommand(Builder& builder)
        : m_systemship_code(std::move(builder._systemship_code)),
          m_warpship_code(std::move(builder._warpship_code))
    {
    }

    std::string m_systemship_code;
    std::string m_warpship_code;
};

class DropCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _systemship_code;
        std::string _warpship_code;

        Builder()
        {
        }
        Builder& systemship(const std::string& code)
        {
            _systemship_code = code;
            return *this;
        }
        Builder& warpship(const std::string& code)
        {
            _warpship_code = code;
            return *this;
        }
        ICmd* build()
        {
            return new DropCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    DropCommand(Builder& builder)
        : m_systemship_code(std::move(builder._systemship_code)),
          m_warpship_code(std::move(builder._warpship_code))
    {
    }

    std::string m_systemship_code;
    std::string m_warpship_code;
};

#endif
