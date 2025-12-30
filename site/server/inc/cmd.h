//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __CMD_H__
#define __CMD_H__

#include "db.h"
#include "typs.h"

void handle_usr_command(const HttpRequest *req, Db *db, HttpResponse *resp);

void handle_login(const HttpRequest *req, Db *db, HttpResponse *resp);
void handle_logout(const HttpRequest *req, Db *db, HttpResponse *resp);

#endif
