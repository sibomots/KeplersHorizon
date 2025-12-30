///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_NEW_COMMAND_H__
#define __BUILD_NEW_COMMAND_H__

#include <string>

#include "icmd.h"
#include "statemachine.h"

class BuildNewCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        StateMachine &_sm;
        std::string _ship_code;
        std::string _ship_name;

        Builder(StateMachine &sm) : _sm(sm)
        {
        }
        Builder &set_ship_code(const std::string &code)
        {
            _ship_code = code;
            return *this;
        }
        Builder &set_ship_name(const std::string &name)
        {
            _ship_name = name;
            return *this;
        }

        ICmd *build()
        {
            return new BuildNewCommand(*this);
        }
    };

  private:
    BuildNewCommand(Builder &builder)
        : m_sm(builder._sm),
          m_ship_code(std::move(builder._ship_code)),
          m_ship_name(std::move(builder._ship_name))
    {
    }

    StateMachine &m_sm;
    std::string m_ship_code;
    std::string m_ship_name;

  public:
    virtual bool invoke(void);
};

#endif
