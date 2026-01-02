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
"?"        { return TOK_HELP; }

"start"    { return TOK_START_GAME; }
"reset"    { return TOK_RESET; }
"save"     { return TOK_SAVE; }
"load"     { return TOK_LOAD; }
"delete"   { return TOK_DELETE; }
"quit"     { return TOK_QUIT; }
"basic"    { return TOK_BASIC; }
"learning" { return TOK_LEARNING; }
"advanced" { return TOK_ADVANCED; }

"crt"      { return TOK_CRT; }
"status"   { return TOK_STATUS; }
"drafts"   { return TOK_DRAFTS; }
"list"     { return TOK_LIST; }

"online"   { return TOK_ONLINE; }
"stats"    { return TOK_STATS; }
"fleet"    { return TOK_FLEET; }
"hex"      { return TOK_HEX; }
"system"   { return TOK_SYSTEM; }
"galaxy"   { return TOK_GALAXY; }

"next"     { return TOK_NEXT; }
"n"        { return TOK_NEXT; }
"done"     { return TOK_DONE; }

"cancel"   { return TOK_CANCEL; }
"commit"   { return TOK_COMMIT; }

"combat"   { return TOK_COMBAT; }
"order"    { return TOK_ORDER; }
"apply"    { return TOK_APPLY; }
"attack"   { return TOK_ATTACK;}
"a"        { return TOK_ATTACK; }
"dodge"    { return TOK_DODGE; }
"escape"   { return TOK_ESCAPE; }
"e"        { return TOK_ESCAPE; }

"build"    { return TOK_BUILD; }
"new"      { return TOK_NEW; }
"set"      { return TOK_SET_ATTR; }

"deploy"   { return TOK_DEPLOY; }

"move"     { return TOK_MOVE; }
"pick"     { return TOK_PICK; }
"drop"     { return TOK_DROP; }

"repair"   { return TOK_REPAIR; }
"resupply" { return TOK_RESUPPLY; }



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

"pd"       { return TOK_POWER_DRIVE; }
"d"        { return TOK_POWER_DRIVE; }
"beam"     { return TOK_BEAM; }
"b"        { return TOK_BEAM; }
"screen"   { return TOK_SCREEN; }
"s"        { return TOK_SCREEN; }
"tube"     { return TOK_TUBE; }
"t"        { return TOK_TUBE; }
"missile"  { return TOK_MISSILE; }
"m"        { return TOK_MISSILE; }



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
