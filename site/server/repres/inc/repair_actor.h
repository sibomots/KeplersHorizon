///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_REPAIR_ACTOR_H__
#define __KH_REPAIR_ACTOR_H__

#include <string>

#include "attributemap.h"
#include "icmd.h"
#include "repair_modes.h"

class RepairActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder();
        Builder& set_list_mode();
        Builder& set_ship_code(const std::string& code);
        Builder& set_attributes(const AttributeMap& attrs);
        ICmd* build();

      private:
        RepairMode m_mode;
        std::string m_ship_code;
        AttributeID m_attribute;
        int m_amount;
    };

    bool invoke(void) override;

  private:
    RepairActor(RepairMode mode, const std::string& ship,
                AttributeID attr, int amount);

    RepairMode m_mode;
    std::string m_ship_code;
    AttributeID m_attribute;
    int m_amount;
};

#endif
