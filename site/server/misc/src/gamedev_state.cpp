///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "gamedev_state.h"

#include <sstream>

GameDevState& GameDevState::instance()
{
    static GameDevState singleton;
    return singleton;
}

GameDevState::GameDevState()
    : m_enabled(false), m_environment_chance(0), m_combat_mod(0), m_move_mod(0),
      m_force_crt(0)
{
}

GameDevState::~GameDevState()
{
}

void GameDevState::set_environment_chance(int pct)
{
    if (pct < 0)
    {
        pct = 0;
    }
    if (pct > 100)
    {
        pct = 100;
    }
    m_environment_chance = pct;
    m_enabled = true;
}

void GameDevState::set_combat_modifier(int mod)
{
    m_combat_mod = mod;
    m_enabled = true;
}

void GameDevState::set_movement_modifier(int mod)
{
    m_move_mod = mod;
    m_enabled = true;
}

void GameDevState::set_force_crt(int result)
{
    if (result < 0)
    {
        result = 0;
    }
    if (result > 12)
    {
        result = 12;
    }
    m_force_crt = result;
    m_enabled = true;
}

void GameDevState::reset()
{
    m_environment_chance = 0;
    m_combat_mod = 0;
    m_move_mod = 0;
    m_force_crt = 0;
    m_enabled = false;
}

void GameDevState::enable(bool on)
{
    m_enabled = on;
}

void GameDevState::get_status(std::string& out) const
{
    std::ostringstream oss;
    oss << "GAMEDEV DEBUG MODE: " << (m_enabled ? "ENABLED" : "DISABLED")
        << "\n";
    if (m_enabled)
    {
        oss << "  environment: " << m_environment_chance << "% chance\n";
        oss << "  combat_mod:  " << (m_combat_mod >= 0 ? "+" : "")
            << m_combat_mod << "\n";
        oss << "  move_mod:    " << (m_move_mod >= 0 ? "+" : "") << m_move_mod
            << "\n";
        if (m_force_crt > 0)
        {
            oss << "  force_crt:   " << m_force_crt << " (forced roll)\n";
        }
        else
        {
            oss << "  force_crt:   natural\n";
        }
    }
    out = oss.str();
}
