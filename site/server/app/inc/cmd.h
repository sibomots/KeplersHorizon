//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __CMD_H__
#define __CMD_H__

#include "comms.h"

bool handle_usr_command(const HttpRequest* req, HttpResponse* resp);
void handle_login(const HttpRequest* req, HttpResponse* resp);
void handle_logout(const HttpRequest* req, HttpResponse* resp);
int internal_command_handler_body(const std::string cmdline, std::string& errmsg);
#endif
