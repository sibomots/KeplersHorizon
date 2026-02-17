///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_FABRICATE_ACTOR_H__
#define __KH_FABRICATE_ACTOR_H__

#include <string>

#include "fabricate_modes.h"
#include "fabricate_strategy.h"
#include "icmd.h"
#include "milieuagent.h"

// FabricateActor - Manufacture items from raw materials
// Usage: fabricate list
//        fabricate torpedoes <qty>
//        fabricate launchers <qty>
//        fabricate tech
class FabricateActor : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_mode(FabricateMode::LIST_PLANS), m_qty(1)
        {
        }
        Builder& show_plans(void)
        {
            m_mode = FabricateMode::LIST_PLANS;
            return *this;
        }
        Builder& set_plan(const std::string& planName)
        {
            // Translate user-typed plan name to FabricateMode
            m_mode = FabricateActor::plan_name_to_mode(planName);
            return *this;
        }
        Builder& set_qty(int qty)
        {
            m_qty = qty;
            return *this;
        }
        ICmd* build()
        {
            return new FabricateActor(m_mode, m_qty);
        }

      private:
        FabricateMode m_mode;
        int m_qty;
    };

    bool invoke(void) override;
    static FabricateMode plan_name_to_mode(const std::string& name);

  private:

    FabricateActor(FabricateMode mode, int qty) : m_mode(mode), m_qty(qty)
    {
    }
    FabricateMode m_mode;
    int m_qty;
};

#endif
