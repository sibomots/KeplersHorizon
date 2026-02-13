///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_RECAP_COMMAND_H__
#define __KH_RECAP_COMMAND_H__

#include <sstream>
#include <string>

#include "icmd.h"

// RecapCommand - Post-game summary report (NetHack-style)
// Usage: recap
// Only available after game_over. Auto-triggered at end of game.
class RecapCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder()
        {
        }
        RecapCommand* build()
        {
            return new RecapCommand();
        }
    };

    bool invoke(void) override;

    // Auto-trigger path: emit recap via telemetry queue (add_tell)
    // Called from turn_end when game ends, not from a command session
    static bool emit_recap(int game_id, char player);

  private:
    RecapCommand()
    {
    }

    // Section builders - each appends to the output stream
    void build_header(std::ostringstream& out, int game_id, char me);
    void build_fleet_registry(std::ostringstream& out, int game_id, char me);
    void build_combat_record(std::ostringstream& out, int game_id, char me);
    void build_economic_summary(std::ostringstream& out, int game_id, char me);
    void build_exploration(std::ostringstream& out, int game_id, char me);
    void build_notable_events(std::ostringstream& out, int game_id, char me);
    void build_footer(std::ostringstream& out);

    // Shared logic for building the full recap text
    static bool build_recap_text(int game_id, char player,
                                 std::string& result_text);
};

#endif
