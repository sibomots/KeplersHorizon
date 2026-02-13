///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_COMMS_H__
#define __KH_COMMS_H__

#include "typedefs.h"

#include <string>

// These are specific to the Comms via REST, etc..

#define KH_REST_LOGIN_ENDPT "/api/login"
#define KH_REST_REGISTER_ENDPT "/api/register"
#define KH_REST_LOGOUT_ENDPT "/api/logout"
#define KH_REST_STATE_ENDPT "/api/state"
#define KH_REST_COMMAND_ENDPT "/api/command"
#define KH_REST_EVENTS_ENDPT "/api/events"
#define KH_REST_MODULES_ENDPT "/api/modules"
#define KH_REST_ROOMS_ENDPT "/api/rooms"
#define KH_REST_ROOMS_PATH "/api/rooms/"
#define KH_REST_SAVE_ENDPT "/api/save"
#define KH_REST_SAVED_ENDPT "/api/saved"
#define KH_REST_GAMES_PATH "/api/games/"

typedef struct HttpRequest
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    ~HttpRequest()
    {
        headers.clear();
    }
} HttpRequest;

typedef struct HttpResponse
{
    int status = 200;
    std::string content_type = "application/json";
    std::string body;
    ~HttpResponse()
    {
    }
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
std::string http_serialize(const HttpResponse* pr);
bool dispatch_request(const HttpRequest* req, HttpResponse* resp);
HttpRequest* http_parse(int fd);
bool authenticated(const HttpRequest* req, HttpResponse* resp);

// Account handlers (account.cpp)
void handle_login(const HttpRequest* req, HttpResponse* resp);
void handle_logout(const HttpRequest* req, HttpResponse* resp);
void handle_register(const HttpRequest* req, HttpResponse* resp);

// Room handlers (rooms_api.cpp)
void handle_rooms(const HttpRequest* req, HttpResponse* resp);
void handle_modules_list(const HttpRequest* req, HttpResponse* resp);

#endif
