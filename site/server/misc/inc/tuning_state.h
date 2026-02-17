///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_TUNING_STATE_H__
#define __KH_TUNING_STATE_H__

#include <map>
#include <string>

// TuningState - Singleton holding tuning parameters loaded from kh.conf.
//
// Provides override modifiers for combat/movement and arbitrary key-value
// settings (event probabilities, etc.) managed via the configure command.
class TuningState
{
  public:
    static TuningState& instance();

    int get_combat_modifier() const;
    int get_movement_modifier() const;
    bool is_enabled() const;

    void set_combat_modifier(int mod);
    void set_movement_modifier(int mod);
    void reset();
    void enable(bool on);

    // Formatted status dump
    void get_status(std::string& out) const;

    // Arbitrary key-value settings from kh.conf
    bool get_setting(const std::string& key, std::string& outVal) const;
    bool get_setting_int(const std::string& key, int& outVal) const;
    void set_setting(const std::string& key, const std::string& val);
    void clear_settings();

  private:
    TuningState();
    ~TuningState();
    TuningState(const TuningState&) = delete;
    TuningState& operator=(const TuningState&) = delete;

    bool m_enabled;
    int m_combat_mod;
    int m_move_mod;
    std::map<std::string, std::string> m_settings;
};

#endif
