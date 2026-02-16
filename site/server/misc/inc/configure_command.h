///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_CONFIGURE_COMMAND_H__
#define __KH_CONFIGURE_COMMAND_H__

#include "configure_modes.h"
#include "icmd.h"

#include <string>

class ConfigureCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        ConfigureMode _mode;

        Builder() : _mode(CFG_SHOW)
        {
        }

        Builder& set_show_mode(void)
        {
            _mode = ConfigureMode::CFG_SHOW;
            return *this;
        }
        Builder& set_reload_conf_mode(void)
        {
            _mode = ConfigureMode::CFG_RELOAD_CONF;
            return *this;
        }

        Builder& set_reload_ai_mode(void)
        {
            _mode = ConfigureMode::CFG_RELOAD_AI;
            return *this;
        }

        ICmd* build()
        {
            return new ConfigureCommand(*this);
        }
    };

    bool invoke(void) override;
    ConfigureMode get_mode(void) const
    {
        return m_mode;
    }

  private:
    ConfigureMode m_mode;

    ConfigureCommand(Builder& builder) : m_mode(builder._mode)
    {
    }

    bool check_privilege();
    bool do_show();
    bool do_reload_conf();
    bool do_reload_ai();
};

#endif
