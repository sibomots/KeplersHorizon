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
#ifndef __BUILD_SET_ATTRIBUTE_COMMAND_H__
#define __BUILD_SET_ATTRIBUTE_COMMAND_H__



#include "db.h"
#include "icmd.h"
#include "typedefs.h"

class BuildSetAttributeCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Db *_db = nullptr;
        int _game_id = 0;
        AttributeMap _attributes;

        Builder &set_db(Db *db)
        {
            _db = db;
            return *this;
        }
        Builder &set_game_id(int id)
        {
            _game_id = id;
            return *this;
        }

        Builder &set_pd(int val)
        {
            _attributes[AttributeID::POWER_DRIVE] = val;
            return *this;
        }

        Builder &set_b(int val)
        {
            _attributes[AttributeID::BEAM] = val;
            return *this;
        }

        Builder &set_s(int val)
        {
            _attributes[AttributeID::SCREEN] = val;
            return *this;
        }

        Builder &set_t(int val)
        {
            _attributes[AttributeID::TUBE] = val;
            return *this;
        }

        Builder &set_m(int val)
        {
            _attributes[AttributeID::MISSILE] = val;
            return *this;
        }

        Builder &set_sr(int val)
        {
            _attributes[AttributeID::SYSTEM_RACK] = val;
            return *this;
        }

        ICmd* build()
        {
            return new BuildSetAttributeCommand(_db, _game_id, _attributes);
        }
    };

  private:
    BuildSetAttributeCommand(Db *db, int game_id, const AttributeMap &attrs)
        : m_db(db), m_game_id(game_id), m_attributes(attrs)
    {
    }

    Db *m_db;
    int m_game_id;
    AttributeMap m_attributes;

  public:
    virtual bool invoke(void);
};

#endif
