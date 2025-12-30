//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __REPAIR_COMMAND_H__
#define __REPAIR_COMMAND_H__

#include <string>

#include "icmd.h"
#include "typedefs.h"

class RepairCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::string _attribute;
        int _amount = 0;

        Builder &set_ship_code(const std::string &code)
        {
            _ship_code = code;
            return *this;
        }

        Builder &set_attribute(const std::string &attr)
        {
            _attribute = attr;
            return *this;
        }

        Builder &set_amount(int amt)
        {
            _amount = amt;
            return *this;
        }

        ICmd *build()
        {
            return new RepairCommand(_ship_code, _attribute, _amount);
        }
    };

  private:
    RepairCommand(const std::string &ship_code, const std::string &attribute,
                  int amount)
        : m_ship_code(ship_code), m_attribute(attribute), m_amount(amount)
    {
    }

    std::string m_ship_code;
    std::string m_attribute;
    int m_amount;

  public:
    virtual bool invoke(void);
};

#endif
