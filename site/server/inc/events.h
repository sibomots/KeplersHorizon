//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __EVENTS_H__
#define __EVENTS_H__
#include <string>

#include "db.h"
#include "typs.h"

void handle_events(const HttpRequest *req, Db *db, HttpResponse *resp);
void append_event(Db *db, int game_id, int user_id, const std::string &cmd,
                  const std::string &result, const GameState &s);

#endif
