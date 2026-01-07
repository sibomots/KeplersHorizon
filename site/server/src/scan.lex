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

// from yacc
#include "lang.hpp"

#define YY_DECL extern "C" int yylex()

%}

%option noyywrap
%option case-insensitive

%%

[ \t\n]	; 

"help"     { return TOK_HELP; }
"demo"     { return TOK_DEMO; }
"?"        { return TOK_HELP; }

"start"    { return TOK_START_GAME; }
"reset"    { return TOK_RESET; }
"save"     { return TOK_SAVE; }
"load"     { return TOK_LOAD; }
"accept"   { return TOK_ACCEPT; }
"reject"   { return TOK_REJECT; }
"delete"   { return TOK_DELETE; }
"clear"    { return TOK_CLEAR; }
"cls"      { return TOK_CLEAR; }
"quit"     { return TOK_QUIT; }
"basic"    { return TOK_BASIC; }
"learning" { return TOK_LEARNING; }
"advanced" { return TOK_ADVANCED; }

"crt"      { return TOK_CRT; }
"status"   { return TOK_STATUS; }
"drafts"   { return TOK_DRAFTS; }

"stats"    { return TOK_STATS; }
"fleet"    { return TOK_FLEET; }
"f"        { return TOK_FLEET; }
"hex"      { return TOK_HEX; }
"system"   { return TOK_SYSTEM; }
"sy"       { return TOK_SYSTEM; }
"survey"   { return TOK_SURVEY; }
"sv"       { return TOK_SURVEY; }
"score"    { return TOK_SCORE; }
"s"        { return TOK_SCORE; }
"extract"  { return TOK_EXTRACT; }
"ex"       { return TOK_EXTRACT; }
"market"   { return TOK_MARKET; }
"mk"       { return TOK_MARKET; }
"trade"    { return TOK_TRADE; }
"tr"       { return TOK_TRADE; }
"fabricate" { return TOK_FABRICATE; }
"fab"      { return TOK_FABRICATE; }
"list"     { return TOK_LIST; }
"scan"     { return TOK_SCAN; }
"buy"      { return TOK_BUY; }
"sell"     { return TOK_SELL; }
"transfer" { return TOK_TRANSFER; }
"ts"       { return TOK_TRANSFER; }
"salvage"  { return TOK_SALVAGE; }
"j"        { return TOK_SALVAGE; }
"galaxy"   { return TOK_GALAXY; }
"gx"       { return TOK_GALAXY; }

"next"     { return TOK_NEXT; }
"n"        { return TOK_NEXT; }
"done"     { return TOK_DONE; }
"ZZ"       { return TOK_DONE; }

"cancel"   { return TOK_CANCEL; }
"commit"   { return TOK_COMMIT; }

"combat"   { return TOK_COMBAT; }
"c"        { return TOK_COMBAT; }
"cd"       { return TOK_COMBAT_DRAFTS; }
"order"    { return TOK_ORDER; }
"co"       { return TOK_COMBAT_ORDER; }
"apply"    { return TOK_APPLY; }
"ca"       { return TOK_COMBAT_APPLY; }
"cc"       { return TOK_COMBAT_COMMIT; }
"cx"       { return TOK_COMBAT_CANCEL; }
"attack"   { return TOK_ATTACK;}
"a"        { return TOK_ATTACK; }
"dodge"    { return TOK_DODGE; }
"d"        { return TOK_DODGE; }
"escape"   { return TOK_ESCAPE; }
"e"        { return TOK_ESCAPE; }

"build"    { return TOK_BUILD; }
"b"        { return TOK_BUILD; }
"bc"       { return TOK_BUILD_COMMIT; }
"new"      { return TOK_NEW; }
"bn"       { return TOK_BUILD_NEW; }
"bd"       { return TOK_BUILD_DRAFTS; }
"bx"       { return TOK_BUILD_CANCEL; }
"set"      { return TOK_SET_ATTR; }
"bs"       { return TOK_BUILD_SET_ATTR; }

"deploy"   { return TOK_DEPLOY; }
"ds"       { return TOK_DEPLOY; }

"move"     { return TOK_MOVE; }
"m"        { return TOK_MOVE; }
"pick"     { return TOK_PICK; }
"p"        { return TOK_PICK; }
"drop"     { return TOK_DROP; }
"dd"       { return TOK_DROP; }

"repair"   { return TOK_REPAIR; }
"rp"       { return TOK_REPAIR; }
"resupply" { return TOK_RESUPPLY; }
"rs"       { return TOK_RESUPPLY; }

"pd"[ \t]*"="[ \t]*[0-9]+   { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_PD_ASSIGN; 
}
"d"[ \t]*"="[ \t]*[0-9]+    { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_PD_ASSIGN; 
}
"beam"[ \t]*"="[ \t]*[0-9]+ { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_B_ASSIGN; 
}
"b"[ \t]*"="[ \t]*[0-9]+    { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_B_ASSIGN; 
}
"screen"[ \t]*"="[ \t]*[0-9]+ { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_S_ASSIGN; 
}
"s"[ \t]*"="[ \t]*[0-9]+    { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_S_ASSIGN; 
}
"tube"[ \t]*"="[ \t]*[0-9]+ { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_T_ASSIGN; 
}
"t"[ \t]*"="[ \t]*[0-9]+    { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_T_ASSIGN; 
}
"missile"[ \t]*"="[ \t]*[0-9]+ { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_M_ASSIGN; 
}
"m"[ \t]*"="[ \t]*[0-9]+    { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_M_ASSIGN; 
}
"sr"[ \t]*"="[ \t]*[0-9]+   { 
    char* eq = strchr(yytext, '=');
    yylval.ival = std::atoi(eq + 1);
    return TOK_SR_ASSIGN; 
}

[a-zA-Z][a-zA-Z0-9]* {
        yylval.sval = new std::string(yytext);
        return TOK_STRING;
}

[0-9]+ {
        yylval.ival = std::atoi(yytext);
        return TOK_INT;
}

.  { return TOK_UNKNOWN; }

%%
