//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
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
