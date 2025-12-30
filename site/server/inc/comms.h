//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COMMS_H__
#define __COMMS_H__

#include <string>

#include "typs.h"

AuthContext require_auth(const HttpRequest *req, HttpResponse *resp);
std::string pick_bearer(const HttpRequest *req);
std::string http_serialize(const HttpResponse &r);
void dispatch_request(const HttpRequest *req, HttpResponse *resp);
HttpRequest http_parse(int fd);

#endif
