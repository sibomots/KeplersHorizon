///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// 
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#ifndef __GAME_H__
#define __GAME_H__

#include <vector>

#include "db.h"
#include "typs.h"

GameState load_game(Db *db, int game_id);
int count_racked_in(Db *db, int game_id, char owner,
                    const std::string &warpship_code);
void insert_ship(Db *db, int game_id, char owner, const ShipRow &s);
void set_current_draft(Db *db, int game_id, char owner,
                       const std::string &code_or_null);
bool draft_exists(Db *db, int game_id, char owner, const std::string &code);
std::string get_current_draft(Db *db, int game_id, char owner);
GameState new_game_state_for_scenario(const std::string &scenario);
void apply_start_of_turn(Db *db, GameState &s);
void advance_next(Db *db, GameState &s);
int next_event_seq(Db *db, int game_id);
void save_game(Db *db, const GameState &s);
bool ship_exists(Db *db, int game_id, char owner, const std::string &code);
std::vector<DraftRow> load_drafts(Db *db, int game_id, char owner);
DraftRow load_draft(Db *db, int game_id, char owner, const std::string &code);
void insert_draft(Db *db, int game_id, char owner, const DraftRow &d);
void update_draft_attrs(Db *db, int game_id, char owner,
                        const std::string &code, const DraftRow &d);
void delete_draft(Db *db, int game_id, char owner, const std::string &code);
std::vector<ShipRow> load_ships(Db *db, int game_id, char owner);
ShipRow load_ship(Db *db, int game_id, char owner, const std::string &code);
void update_ship_location(Db *db, int game_id, char owner,
                          const std::string &code, const std::string &at_system,
                          const std::string &at_hex,
                          const std::string &racked_in);
#endif
