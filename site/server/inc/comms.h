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

#include "typedefs.h"

// These are specific to the Comms via REST, etc..

typedef struct
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
} HttpRequest;

typedef struct
{
    int status = 200;
    std::string content_type = "application/json";
    std::string body;
} HttpResponse;

typedef struct
{
    int user_id = 0;
    std::string username;
    std::string token;
    int game_id = 0;
    char player = 0; // 'A' or 'B'
} AuthContext;

AuthContext require_auth(const HttpRequest* req, HttpResponse* resp);
std::string pick_bearer(const HttpRequest* req);
std::string http_serialize(const HttpResponse& r);
void dispatch_request(const HttpRequest* req, HttpResponse* resp);
HttpRequest http_parse(int fd);
bool authenticated(const HttpRequest* req, HttpResponse* resp);

#endif
