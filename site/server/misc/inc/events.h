///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_EVENTS_H__
#define __KH_EVENTS_H__
#include <string>

#include "comms.h"
#include "statemachine.h"

void handle_events(const HttpRequest* req, HttpResponse* resp);
void append_event(int game_id, int user_id, const std::string& cmd,
                  const std::string& result, const GameState& s);

#endif
