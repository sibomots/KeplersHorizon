///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_TUNING_COMMAND_H__
#define __KH_TUNING_COMMAND_H__

#include "tuning_modes.h"
#include "icmd.h"

#include <string>

class TuningCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        TuningMode _mode;

        Builder() : _mode(TN_SHOW)
        {
        }

        Builder& set_show_mode(void)
        {
            _mode = TuningMode::TN_SHOW;
            return *this;
        }
        Builder& set_reload_conf_mode(void)
        {
            _mode = TuningMode::TN_RELOAD_CONF;
            return *this;
        }

        Builder& set_reload_ai_mode(void)
        {
            _mode = TuningMode::TN_RELOAD_AI;
            return *this;
        }

        ICmd* build()
        {
            return new TuningCommand(*this);
        }
    };

    bool invoke(void) override;
    TuningMode get_mode(void) const
    {
        return m_mode;
    }

    // Load kh.conf at startup (no Telemetry, Logger only).
    // Returns count of settings loaded, or -1 on file-not-found.
    static int load_conf_file();

  private:
    TuningMode m_mode;

    TuningCommand(Builder& builder) : m_mode(builder._mode)
    {
    }

    bool check_privilege();
    bool do_show();
    bool do_reload_conf();
    bool do_reload_ai();
};

#endif
