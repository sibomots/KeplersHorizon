/* 
//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
// 
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
*/
%{
#include <string>
#include <cstring>

// from yacc
#include "lang.hpp"

// Location tracking for bison %locations
extern YYLTYPE yylloc;
static int g_line = 1;
static int g_col  = 1;

static void set_loc_for_token()
{
    yylloc.first_line = g_line;
    yylloc.first_column = g_col;
    yylloc.last_line = g_line;
    yylloc.last_column = g_col + (int)yyleng - 1;
}

static void advance_col()
{
    g_col += (int)yyleng;
}
#define YY_DECL extern "C" int yylex()

%}

%option noyywrap
%option case-insensitive

%x HELPARG

%%

[ \t]+   { set_loc_for_token(); advance_col(); }
\n+      { set_loc_for_token(); g_line += yyleng; g_col = 1; }


<HELPARG>[ \t]+   { set_loc_for_token(); advance_col(); }
<HELPARG>\n+      { set_loc_for_token(); g_line += yyleng; g_col = 1; BEGIN(INITIAL); }
<HELPARG><<EOF>>  { BEGIN(INITIAL); return 0; }

<HELPARG>[a-zA-Z][a-zA-Z0-9]* {
        set_loc_for_token();
        yylval.sval = new std::string(yytext);
        advance_col();
        BEGIN(INITIAL);
        return TOK_STRING;
}

"help"     { set_loc_for_token(); advance_col(); BEGIN(HELPARG); return TOK_HELP; }
"?"        { set_loc_for_token(); advance_col(); BEGIN(HELPARG); return TOK_HELP; }
"license"  { set_loc_for_token(); advance_col(); return TOK_LICENSE; }
"demo"     { set_loc_for_token(); advance_col(); return TOK_DEMO; }

"save"     { set_loc_for_token(); advance_col(); return TOK_SAVE; }
"load"     { set_loc_for_token(); advance_col(); return TOK_LOAD; }
"accept"   { set_loc_for_token(); advance_col(); return TOK_ACCEPT; }
"reject"   { set_loc_for_token(); advance_col(); return TOK_REJECT; }
"delete"   { set_loc_for_token(); advance_col(); return TOK_DELETE; }
"clear"    { set_loc_for_token(); advance_col(); return TOK_CLEAR; }
"cls"      { set_loc_for_token(); advance_col(); return TOK_CLEAR; }

"crt"      { set_loc_for_token(); advance_col(); return TOK_CRT; }
"status"   { set_loc_for_token(); advance_col(); return TOK_STATUS; }

"fleet"    { set_loc_for_token(); advance_col(); return TOK_FLEET; }
"hex"      { set_loc_for_token(); advance_col(); return TOK_HEX; }

"system"   { set_loc_for_token(); advance_col(); return TOK_SYSTEM; }
"sy"       { set_loc_for_token(); advance_col(); return TOK_SYSTEM; }
"anomalies" { set_loc_for_token(); advance_col(); return TOK_ANOMALIES; }
"facilities" { set_loc_for_token(); advance_col(); return TOK_FACILITIES; }
"resources" { set_loc_for_token(); advance_col(); return TOK_RESOURCES; }
"planets" { set_loc_for_token(); advance_col(); return TOK_PLANETS; }

"survey"   { set_loc_for_token(); advance_col(); return TOK_SURVEY; }
"sv"       { set_loc_for_token(); advance_col(); return TOK_SURVEY; }
"score"    { set_loc_for_token(); advance_col(); return TOK_SCORE; }
"recap"    { set_loc_for_token(); advance_col(); return TOK_RECAP; }
"extract"  { set_loc_for_token(); advance_col(); return TOK_EXTRACT; }
"ex"       { set_loc_for_token(); advance_col(); return TOK_EXTRACT; }
"market"   { set_loc_for_token(); advance_col(); return TOK_MARKET; }
"mk"       { set_loc_for_token(); advance_col(); return TOK_MARKET; }
"trade"    { set_loc_for_token(); advance_col(); return TOK_TRADE; }
"tr"       { set_loc_for_token(); advance_col(); return TOK_TRADE; }
"fabricate" { set_loc_for_token(); advance_col(); return TOK_FABRICATE; }
"fab"      { set_loc_for_token(); advance_col(); return TOK_FABRICATE; }
"outfit"   { set_loc_for_token(); advance_col(); return TOK_OUTFIT; }
"out"      { set_loc_for_token(); advance_col(); return TOK_OUTFIT; }
"list"     { set_loc_for_token(); advance_col(); return TOK_LIST; }
"scan"     { set_loc_for_token(); advance_col(); return TOK_SCAN; }
"buy"      { set_loc_for_token(); advance_col(); return TOK_BUY; }
"sell"     { set_loc_for_token(); advance_col(); return TOK_SELL; }
"transfer" { set_loc_for_token(); advance_col(); return TOK_TRANSFER; }
"ts"       { set_loc_for_token(); advance_col(); return TOK_TRANSFER; }
"salvage"  { set_loc_for_token(); advance_col(); return TOK_SALVAGE; }
"jk"       { set_loc_for_token(); advance_col(); return TOK_SALVAGE; }
"cargo"    { set_loc_for_token(); advance_col(); return TOK_CARGO; }

"next"     { set_loc_for_token(); advance_col(); return TOK_NEXT; }
"n"        { set_loc_for_token(); advance_col(); return TOK_NEXT; }
"done"     { set_loc_for_token(); advance_col(); return TOK_DONE; }
"ZZ"       { set_loc_for_token(); advance_col(); return TOK_DONE; }

"cancel"   { set_loc_for_token(); advance_col(); return TOK_CANCEL; }
"commit"   { set_loc_for_token(); advance_col(); return TOK_COMMIT; }

"combat"   { set_loc_for_token(); advance_col(); return TOK_COMBAT; }
"cm"       { set_loc_for_token(); advance_col(); return TOK_COMBAT; }
"cd"       { set_loc_for_token(); advance_col(); return TOK_COMBAT_DRAFTS; }
"order"    { set_loc_for_token(); advance_col(); return TOK_ORDER; }
"co"       { set_loc_for_token(); advance_col(); return TOK_COMBAT_ORDER; }
"apply"    { set_loc_for_token(); advance_col(); return TOK_APPLY; }
"ca"       { set_loc_for_token(); advance_col(); return TOK_COMBAT_APPLY; }
"cc"       { set_loc_for_token(); advance_col(); return TOK_COMBAT_COMMIT; }
"cx"       { set_loc_for_token(); advance_col(); return TOK_COMBAT_CANCEL; }
"attack"   { set_loc_for_token(); advance_col(); return TOK_ATTACK; }
"a"        { set_loc_for_token(); advance_col(); return TOK_ATTACK; }
"dodge"    { set_loc_for_token(); advance_col(); return TOK_DODGE; }
"d"        { set_loc_for_token(); advance_col(); return TOK_DODGE; }
"escape"   { set_loc_for_token(); advance_col(); return TOK_ESCAPE; }
"e"        { set_loc_for_token(); advance_col(); return TOK_ESCAPE; }

"build"    { set_loc_for_token(); advance_col(); return TOK_BUILD; }
"bb"       { set_loc_for_token(); advance_col(); return TOK_BUILD; }
"bc"       { set_loc_for_token(); advance_col(); return TOK_BUILD_COMMIT; }
"new"      { set_loc_for_token(); advance_col(); return TOK_NEW; }
"bn"       { set_loc_for_token(); advance_col(); return TOK_BUILD_NEW; }
"bd"       { set_loc_for_token(); advance_col(); return TOK_BUILD_DRAFTS; }
"bx"       { set_loc_for_token(); advance_col(); return TOK_BUILD_CANCEL; }
"set"      { set_loc_for_token(); advance_col(); return TOK_SET_ATTR; }
"bs"       { set_loc_for_token(); advance_col(); return TOK_BUILD_SET_ATTR; }

"deploy"   { set_loc_for_token(); advance_col(); return TOK_DEPLOY; }
"ds"       { set_loc_for_token(); advance_col(); return TOK_DEPLOY; }

"move"     { set_loc_for_token(); advance_col(); return TOK_MOVE; }
"m"        { set_loc_for_token(); advance_col(); return TOK_MOVE; }
"chart"    { set_loc_for_token(); advance_col(); return TOK_CHART; }
"ch"       { set_loc_for_token(); advance_col(); return TOK_CHART; }
"pick"     { set_loc_for_token(); advance_col(); return TOK_PICK; }
"pu"       { set_loc_for_token(); advance_col(); return TOK_PICK; }
"drop"     { set_loc_for_token(); advance_col(); return TOK_DROP; }
"do"       { set_loc_for_token(); advance_col(); return TOK_DROP; }

"repair"   { set_loc_for_token(); advance_col(); return TOK_REPAIR; }
"rp"       { set_loc_for_token(); advance_col(); return TOK_REPAIR; }
"resupply" { set_loc_for_token(); advance_col(); return TOK_RESUPPLY; }
"rs"       { set_loc_for_token(); advance_col(); return TOK_RESUPPLY; }
"retreat"  { set_loc_for_token(); advance_col(); return TOK_RETREAT; }
"rt"       { set_loc_for_token(); advance_col(); return TOK_RETREAT; }

"configure" { set_loc_for_token(); advance_col(); return TOK_CONFIGURE; }
"cfg"       { set_loc_for_token(); advance_col(); return TOK_CONFIGURE; }
"reload"    { set_loc_for_token(); advance_col(); return TOK_RELOAD; }
"conf"      { set_loc_for_token(); advance_col(); return TOK_CONF; }
"ai"        { set_loc_for_token(); advance_col(); return TOK_AI; }

"lrs"      { set_loc_for_token(); advance_col(); return TOK_LRS; }

"pd"[ \t]*"="[ \t]*[0-9]+   {
    set_loc_for_token();
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    advance_col();
    return TOK_PD_ASSIGN; 
}
"d"[ \t]*"="[ \t]*[0-9]+    {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_PD_ASSIGN; 
}
"phasic"[ \t]*"="[ \t]*[0-9]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_P_ASSIGN; 
}
"p"[ \t]*"="[ \t]*[0-9]+    {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_P_ASSIGN; 
}
"shield"[ \t]*"="[ \t]*[0-9]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_S_ASSIGN; 
}
"s"[ \t]*"="[ \t]*[0-9]+    {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_S_ASSIGN; 
}
"launcher"[ \t]*"="[ \t]*[0-9]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_L_ASSIGN; 
}
"l"[ \t]*"="[ \t]*[0-9]+    {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_L_ASSIGN; 
}
"torpedo"[ \t]*"="[ \t]*[0-9]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_T_ASSIGN; 
}
"t"[ \t]*"="[ \t]*[0-9]+    {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_T_ASSIGN; 
}
"h"[ \t]*"="[ \t]*[0-9]+   {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    advance_col();
    return TOK_H_ASSIGN; 
}

"pd"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);  // Store the bad value
    advance_col();
    return TOK_PD_ASSIGN_INVALID;
}
"d"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);  // Store the bad value
    advance_col();
    return TOK_PD_ASSIGN_INVALID;
}
"phasic"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_P_ASSIGN_INVALID;
}
"p"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_P_ASSIGN_INVALID;
}
"shield"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_S_ASSIGN_INVALID;
}
"s"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_S_ASSIGN_INVALID;
}
"launcher"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_L_ASSIGN_INVALID;
}
"l"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_L_ASSIGN_INVALID;
}
"torpedo"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_T_ASSIGN_INVALID;
}
"t"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_T_ASSIGN_INVALID;
}
"h"[ \t]*"="[ \t]*[^ \t\n]+ {
    set_loc_for_token();
    char* eq = strchr(yytext, '=');
    yylval.sval = new std::string(eq + 1);
    advance_col();
    return TOK_H_ASSIGN_INVALID;
}

[a-zA-Z][\-a-zA-Z0-9]* {
        set_loc_for_token();
        yylval.sval = new std::string(yytext);
        advance_col();
        return TOK_STRING;
}

[0-9]+ {
        set_loc_for_token();
        yylval.ival = std::atoi(yytext);
        advance_col();
        return TOK_INT;
}

.  { set_loc_for_token(); advance_col(); return TOK_UNKNOWN; }

%%
