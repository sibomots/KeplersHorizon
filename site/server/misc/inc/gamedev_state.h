///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_GAMEDEV_STATE_H__
#define __KH_GAMEDEV_STATE_H__

#include <map>
#include <string>

/**
 * GameDevState - Singleton holding debug/test parameters for game development.
 *
 * Allows forcing rare events and environmental effects for testing purposes.
 * Parameters:
 *   - environment: 0-100, probability % of environmental effects triggering
 *   - combat_mod: -N to +N, override combat modifier for all hexes
 *   - move_mod: -N to +N, override movement modifier for all hexes
 *   - force_crt: 0-12, force CRT roll result (0 = natural roll)
 */
class GameDevState
{
  public:
    static GameDevState& instance();

    // Getters
    int get_environment_chance() const
    {
        return m_environment_chance;
    }
    int get_combat_modifier() const
    {
        return m_combat_mod;
    }
    int get_movement_modifier() const
    {
        return m_move_mod;
    }
    int get_force_crt() const
    {
        return m_force_crt;
    }
    bool is_enabled() const
    {
        return m_enabled;
    }

    // Setters
    void set_environment_chance(int pct);
    void set_combat_modifier(int mod);
    void set_movement_modifier(int mod);
    void set_force_crt(int result);
    void reset();
    void enable(bool on);

    // Get all settings as formatted string
    void get_status(std::string& out) const;

  private:
    GameDevState();
    ~GameDevState();
    GameDevState(const GameDevState&) = delete;
    GameDevState& operator=(const GameDevState&) = delete;

    bool m_enabled;
    int m_environment_chance;
    int m_combat_mod;
    int m_move_mod;
    int m_force_crt;
};

#endif
