/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_HOME_JDW_GH_KEPLERSHORIZON_SITE_SERVER_B22_LANG_HPP_INCLUDED
# define YY_YY_HOME_JDW_GH_KEPLERSHORIZON_SITE_SERVER_B22_LANG_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 62 "src/lang.y"

#include <string>
#include <vector>

#line 54 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.hpp"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOK_INT = 258,                 /* TOK_INT  */
    TOK_STRING = 259,              /* TOK_STRING  */
    TOK_ADVANCED = 260,            /* TOK_ADVANCED  */
    TOK_APPLY = 261,               /* TOK_APPLY  */
    TOK_ATTACK = 262,              /* TOK_ATTACK  */
    TOK_BASIC = 263,               /* TOK_BASIC  */
    TOK_BEAM = 264,                /* TOK_BEAM  */
    TOK_BUILD = 265,               /* TOK_BUILD  */
    TOK_BUILD_COMMIT = 266,        /* TOK_BUILD_COMMIT  */
    TOK_BUILD_NEW = 267,           /* TOK_BUILD_NEW  */
    TOK_BUILD_DRAFTS = 268,        /* TOK_BUILD_DRAFTS  */
    TOK_BUILD_CANCEL = 269,        /* TOK_BUILD_CANCEL  */
    TOK_BUILD_SET_ATTR = 270,      /* TOK_BUILD_SET_ATTR  */
    TOK_CANCEL = 271,              /* TOK_CANCEL  */
    TOK_COMBAT = 272,              /* TOK_COMBAT  */
    TOK_COMBAT_DRAFTS = 273,       /* TOK_COMBAT_DRAFTS  */
    TOK_COMBAT_ORDER = 274,        /* TOK_COMBAT_ORDER  */
    TOK_COMBAT_APPLY = 275,        /* TOK_COMBAT_APPLY  */
    TOK_COMBAT_COMMIT = 276,       /* TOK_COMBAT_COMMIT  */
    TOK_COMBAT_CANCEL = 277,       /* TOK_COMBAT_CANCEL  */
    TOK_COMMIT = 278,              /* TOK_COMMIT  */
    TOK_CRT = 279,                 /* TOK_CRT  */
    TOK_DELETE = 280,              /* TOK_DELETE  */
    TOK_DEPLOY = 281,              /* TOK_DEPLOY  */
    TOK_DODGE = 282,               /* TOK_DODGE  */
    TOK_DONE = 283,                /* TOK_DONE  */
    TOK_DRAFTS = 284,              /* TOK_DRAFTS  */
    TOK_DROP = 285,                /* TOK_DROP  */
    TOK_ESCAPE = 286,              /* TOK_ESCAPE  */
    TOK_FLEET = 287,               /* TOK_FLEET  */
    TOK_GALAXY = 288,              /* TOK_GALAXY  */
    TOK_HELP = 289,                /* TOK_HELP  */
    TOK_HEX = 290,                 /* TOK_HEX  */
    TOK_LEARNING = 291,            /* TOK_LEARNING  */
    TOK_LIST = 292,                /* TOK_LIST  */
    TOK_LOAD = 293,                /* TOK_LOAD  */
    TOK_MISSILE = 294,             /* TOK_MISSILE  */
    TOK_MOVE = 295,                /* TOK_MOVE  */
    TOK_NEW = 296,                 /* TOK_NEW  */
    TOK_NEXT = 297,                /* TOK_NEXT  */
    TOK_NEXT_SHORT = 298,          /* TOK_NEXT_SHORT  */
    TOK_ONLINE = 299,              /* TOK_ONLINE  */
    TOK_ORDER = 300,               /* TOK_ORDER  */
    TOK_PICK = 301,                /* TOK_PICK  */
    TOK_POWER_DRIVE = 302,         /* TOK_POWER_DRIVE  */
    TOK_QUIT = 303,                /* TOK_QUIT  */
    TOK_REPAIR = 304,              /* TOK_REPAIR  */
    TOK_RESET = 305,               /* TOK_RESET  */
    TOK_RESUPPLY = 306,            /* TOK_RESUPPLY  */
    TOK_SAVE = 307,                /* TOK_SAVE  */
    TOK_SCREEN = 308,              /* TOK_SCREEN  */
    TOK_SET_ATTR = 309,            /* TOK_SET_ATTR  */
    TOK_START_GAME = 310,          /* TOK_START_GAME  */
    TOK_STATS = 311,               /* TOK_STATS  */
    TOK_STATUS = 312,              /* TOK_STATUS  */
    TOK_SYSTEM = 313,              /* TOK_SYSTEM  */
    TOK_SURVEY = 314,              /* TOK_SURVEY  */
    TOK_TUBE = 315,                /* TOK_TUBE  */
    TOK_PD_ASSIGN = 316,           /* TOK_PD_ASSIGN  */
    TOK_B_ASSIGN = 317,            /* TOK_B_ASSIGN  */
    TOK_S_ASSIGN = 318,            /* TOK_S_ASSIGN  */
    TOK_T_ASSIGN = 319,            /* TOK_T_ASSIGN  */
    TOK_M_ASSIGN = 320,            /* TOK_M_ASSIGN  */
    TOK_SR_ASSIGN = 321,           /* TOK_SR_ASSIGN  */
    TOK_UNKNOWN = 322              /* TOK_UNKNOWN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 67 "src/lang.y"

   int ival;
   std::string* sval;
   //std::vector<std::string>* vec_sval;
   std::vector<std::string>* vec_sval;

#line 145 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_HOME_JDW_GH_KEPLERSHORIZON_SITE_SERVER_B22_LANG_HPP_INCLUDED  */
