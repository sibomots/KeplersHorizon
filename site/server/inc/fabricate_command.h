//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __FABRICATE_COMMAND_H__
#define __FABRICATE_COMMAND_H__

#include <string>

#include "icmd.h"

// FabricateCommand - Manufacture items from raw materials
// Usage: fabricate list
//        fabricate missiles <qty>
//        fabricate tubes <qty>
//        fabricate tech
class FabricateCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder() : m_recipe(""), m_quantity(1) {}
        Builder& recipe(const std::string& r)
        {
            m_recipe = r;
            return *this;
        }
        Builder& quantity(int qty)
        {
            m_quantity = qty;
            return *this;
        }
        FabricateCommand* build()
        {
            return new FabricateCommand(m_recipe, m_quantity);
        }

      private:
        std::string m_recipe;
        int m_quantity;
    };

    bool invoke(void) override;

  private:
    FabricateCommand(const std::string& recipe, int qty)
        : m_recipe(recipe), m_quantity(qty)
    {
    }
    std::string m_recipe;
    int m_quantity;

    void show_recipes();
    bool do_fabricate();
};

#endif
