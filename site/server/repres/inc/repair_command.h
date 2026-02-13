///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_REPAIR_COMMAND_H__
#define __KH_REPAIR_COMMAND_H__

#include <string>

#include "icmd.h"
#include "typedefs.h"

class RepairCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string ship_code;
        std::string attribute;
        int amount = 0;

        Builder& set_ship_code(const std::string& code)
        {
            ship_code = code;
            return *this;
        }

        Builder& set_attribute(const std::string& attr)
        {
            attribute = attr;
            return *this;
        }

        Builder& set_amount(int amt)
        {
            amount = amt;
            return *this;
        }

        ICmd* build()
        {
            return new RepairCommand(*this);
        }
    };

  private:
    RepairCommand(Builder& builder)
        : m_ship_code(std::move(builder.ship_code)),
          m_attribute(std::move(builder.attribute)),
          m_amount(std::move(builder.amount))
    {
    }

    std::string m_ship_code;
    std::string m_attribute;
    int m_amount;

  public:
    virtual bool invoke(void);
};

#endif
