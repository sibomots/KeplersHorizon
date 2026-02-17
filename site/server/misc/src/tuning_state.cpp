///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "tuning_state.h"

#include <cstdlib>
#include <sstream>

TuningState& TuningState::instance()
{
    static TuningState singleton;
    return singleton;
}

TuningState::TuningState()
    : m_enabled(false), m_combat_mod(0), m_move_mod(0)
{
}

TuningState::~TuningState()
{
}

void TuningState::set_combat_modifier(int mod)
{
    m_combat_mod = mod;
    m_enabled = true;
}

void TuningState::set_movement_modifier(int mod)
{
    m_move_mod = mod;
    m_enabled = true;
}

void TuningState::reset()
{
    m_combat_mod = 0;
    m_move_mod = 0;
    m_enabled = false;
    clear_settings();
}

void TuningState::enable(bool on)
{
    m_enabled = on;
}

int TuningState::get_combat_modifier() const
{
    return m_combat_mod;
}

int TuningState::get_movement_modifier() const
{
    return m_move_mod;
}

bool TuningState::is_enabled() const
{
    return m_enabled;
}

void TuningState::get_status(std::string& out) const
{
    std::ostringstream oss;
    oss << "TUNING: " << (m_enabled ? "ENABLED" : "DISABLED") << "\n";
    if (m_enabled)
    {
        oss << "  combat_mod:  " << (m_combat_mod >= 0 ? "+" : "")
            << m_combat_mod << "\n";
        oss << "  move_mod:    " << (m_move_mod >= 0 ? "+" : "")
            << m_move_mod << "\n";

        for (const std::pair<const std::string, std::string>& kv : m_settings)
        {
            oss << "  " << kv.first << " = " << kv.second << "\n";
        }
    }
    out = oss.str();
}

bool TuningState::get_setting(const std::string& key, std::string& outVal) const
{
    std::map<std::string, std::string>::const_iterator it = m_settings.find(key);
    if (it == m_settings.end())
    {
        return false;
    }
    outVal = it->second;
    return true;
}

bool TuningState::get_setting_int(const std::string& key, int& outVal) const
{
    std::string strVal;
    bool bFound = get_setting(key, strVal);
    if (!bFound)
    {
        return false;
    }
    outVal = std::atoi(strVal.c_str());
    return true;
}

void TuningState::set_setting(const std::string& key, const std::string& val)
{
    m_settings[key] = val;
}

void TuningState::clear_settings()
{
    m_settings.clear();
}
