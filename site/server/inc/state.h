//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __STATE_H__
#define __STATE_H__

#include <string>

#include "db.h"
#include "typs.h"

std::string json_ok_with_state_and_event(const GameState &s,
                                         const std::string &eventText);
void handle_state(const HttpRequest *req, Db *db, HttpResponse *resp);

#endif
