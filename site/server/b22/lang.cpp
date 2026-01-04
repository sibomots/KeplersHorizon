/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 8 "src/lang.y"

#include <cstdio>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "typedefs.h"

#include "logger.h"
#include "start_command.h"
#include "build_command.h"
#include "build_commit_command.h"
#include "build_new_command.h"
#include "build_cancel_command.h"
#include "build_list_drafts_command.h"
#include "build_show_draft_command.h"
#include "build_set_attribute_command.h"
#include "next_command.h"
#include "done_command.h"
#include "deploy_command.h"
#include "move_command.h"
#include "combat_order_command.h"
#include "combat_apply_command.h"
#include "combat_drafts_command.h"
#include "combat_commit_command.h"
#include "combat_cancel_command.h"
#include "fleet_command.h"
#include "system_command.h"
#include "survey_command.h"
#include "repair_command.h"
#include "resupply_command.h"
#include "statemachine.h"
// #include "game.h"
#include "db.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);

// BUGBUG
// Global builder for accumulating build set attributes
BuildSetAttributeCommand::Builder* g_build_set_builder = new BuildSetAttributeCommand::Builder();
// Global builders for combat commands (accumulate power/damage specs from sub-rules)
CombatOrderCommand::Builder* g_combat_order_builder = new CombatOrderCommand::Builder();
CombatApplyCommand::Builder* g_combat_apply_builder = new CombatApplyCommand::Builder();

// Global builder for repair command
RepairCommand::Builder* g_repair_builder = new RepairCommand::Builder();


#line 125 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "lang.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TOK_INT = 3,                    /* TOK_INT  */
  YYSYMBOL_TOK_STRING = 4,                 /* TOK_STRING  */
  YYSYMBOL_TOK_ADVANCED = 5,               /* TOK_ADVANCED  */
  YYSYMBOL_TOK_APPLY = 6,                  /* TOK_APPLY  */
  YYSYMBOL_TOK_ATTACK = 7,                 /* TOK_ATTACK  */
  YYSYMBOL_TOK_BASIC = 8,                  /* TOK_BASIC  */
  YYSYMBOL_TOK_BEAM = 9,                   /* TOK_BEAM  */
  YYSYMBOL_TOK_BUILD = 10,                 /* TOK_BUILD  */
  YYSYMBOL_TOK_BUILD_COMMIT = 11,          /* TOK_BUILD_COMMIT  */
  YYSYMBOL_TOK_BUILD_NEW = 12,             /* TOK_BUILD_NEW  */
  YYSYMBOL_TOK_BUILD_DRAFTS = 13,          /* TOK_BUILD_DRAFTS  */
  YYSYMBOL_TOK_BUILD_CANCEL = 14,          /* TOK_BUILD_CANCEL  */
  YYSYMBOL_TOK_BUILD_SET_ATTR = 15,        /* TOK_BUILD_SET_ATTR  */
  YYSYMBOL_TOK_CANCEL = 16,                /* TOK_CANCEL  */
  YYSYMBOL_TOK_COMBAT = 17,                /* TOK_COMBAT  */
  YYSYMBOL_TOK_COMBAT_DRAFTS = 18,         /* TOK_COMBAT_DRAFTS  */
  YYSYMBOL_TOK_COMBAT_ORDER = 19,          /* TOK_COMBAT_ORDER  */
  YYSYMBOL_TOK_COMBAT_APPLY = 20,          /* TOK_COMBAT_APPLY  */
  YYSYMBOL_TOK_COMBAT_COMMIT = 21,         /* TOK_COMBAT_COMMIT  */
  YYSYMBOL_TOK_COMBAT_CANCEL = 22,         /* TOK_COMBAT_CANCEL  */
  YYSYMBOL_TOK_COMMIT = 23,                /* TOK_COMMIT  */
  YYSYMBOL_TOK_CRT = 24,                   /* TOK_CRT  */
  YYSYMBOL_TOK_DELETE = 25,                /* TOK_DELETE  */
  YYSYMBOL_TOK_DEPLOY = 26,                /* TOK_DEPLOY  */
  YYSYMBOL_TOK_DODGE = 27,                 /* TOK_DODGE  */
  YYSYMBOL_TOK_DONE = 28,                  /* TOK_DONE  */
  YYSYMBOL_TOK_DRAFTS = 29,                /* TOK_DRAFTS  */
  YYSYMBOL_TOK_DROP = 30,                  /* TOK_DROP  */
  YYSYMBOL_TOK_ESCAPE = 31,                /* TOK_ESCAPE  */
  YYSYMBOL_TOK_FLEET = 32,                 /* TOK_FLEET  */
  YYSYMBOL_TOK_GALAXY = 33,                /* TOK_GALAXY  */
  YYSYMBOL_TOK_HELP = 34,                  /* TOK_HELP  */
  YYSYMBOL_TOK_HEX = 35,                   /* TOK_HEX  */
  YYSYMBOL_TOK_LEARNING = 36,              /* TOK_LEARNING  */
  YYSYMBOL_TOK_LIST = 37,                  /* TOK_LIST  */
  YYSYMBOL_TOK_LOAD = 38,                  /* TOK_LOAD  */
  YYSYMBOL_TOK_MISSILE = 39,               /* TOK_MISSILE  */
  YYSYMBOL_TOK_MOVE = 40,                  /* TOK_MOVE  */
  YYSYMBOL_TOK_NEW = 41,                   /* TOK_NEW  */
  YYSYMBOL_TOK_NEXT = 42,                  /* TOK_NEXT  */
  YYSYMBOL_TOK_NEXT_SHORT = 43,            /* TOK_NEXT_SHORT  */
  YYSYMBOL_TOK_ONLINE = 44,                /* TOK_ONLINE  */
  YYSYMBOL_TOK_ORDER = 45,                 /* TOK_ORDER  */
  YYSYMBOL_TOK_PICK = 46,                  /* TOK_PICK  */
  YYSYMBOL_TOK_POWER_DRIVE = 47,           /* TOK_POWER_DRIVE  */
  YYSYMBOL_TOK_QUIT = 48,                  /* TOK_QUIT  */
  YYSYMBOL_TOK_REPAIR = 49,                /* TOK_REPAIR  */
  YYSYMBOL_TOK_RESET = 50,                 /* TOK_RESET  */
  YYSYMBOL_TOK_RESUPPLY = 51,              /* TOK_RESUPPLY  */
  YYSYMBOL_TOK_SAVE = 52,                  /* TOK_SAVE  */
  YYSYMBOL_TOK_SCREEN = 53,                /* TOK_SCREEN  */
  YYSYMBOL_TOK_SET_ATTR = 54,              /* TOK_SET_ATTR  */
  YYSYMBOL_TOK_START_GAME = 55,            /* TOK_START_GAME  */
  YYSYMBOL_TOK_STATS = 56,                 /* TOK_STATS  */
  YYSYMBOL_TOK_STATUS = 57,                /* TOK_STATUS  */
  YYSYMBOL_TOK_SYSTEM = 58,                /* TOK_SYSTEM  */
  YYSYMBOL_TOK_SURVEY = 59,                /* TOK_SURVEY  */
  YYSYMBOL_TOK_TUBE = 60,                  /* TOK_TUBE  */
  YYSYMBOL_TOK_PD_ASSIGN = 61,             /* TOK_PD_ASSIGN  */
  YYSYMBOL_TOK_B_ASSIGN = 62,              /* TOK_B_ASSIGN  */
  YYSYMBOL_TOK_S_ASSIGN = 63,              /* TOK_S_ASSIGN  */
  YYSYMBOL_TOK_T_ASSIGN = 64,              /* TOK_T_ASSIGN  */
  YYSYMBOL_TOK_M_ASSIGN = 65,              /* TOK_M_ASSIGN  */
  YYSYMBOL_TOK_SR_ASSIGN = 66,             /* TOK_SR_ASSIGN  */
  YYSYMBOL_TOK_UNKNOWN = 67,               /* TOK_UNKNOWN  */
  YYSYMBOL_YYACCEPT = 68,                  /* $accept  */
  YYSYMBOL_commands = 69,                  /* commands  */
  YYSYMBOL_command = 70,                   /* command  */
  YYSYMBOL_session_cmd = 71,               /* session_cmd  */
  YYSYMBOL_info_cmd = 72,                  /* info_cmd  */
  YYSYMBOL_looking_cmd = 73,               /* looking_cmd  */
  YYSYMBOL_turn_cmd = 74,                  /* turn_cmd  */
  YYSYMBOL_combat_cmd = 75,                /* combat_cmd  */
  YYSYMBOL_building_draft_ship = 76,       /* building_draft_ship  */
  YYSYMBOL_combat_initiator_ship = 77,     /* combat_initiator_ship  */
  YYSYMBOL_combat_damaged_ship = 78,       /* combat_damaged_ship  */
  YYSYMBOL_combat_tactic = 79,             /* combat_tactic  */
  YYSYMBOL_combat_target_ship = 80,        /* combat_target_ship  */
  YYSYMBOL_combat_order_spec = 81,         /* combat_order_spec  */
  YYSYMBOL_additional_combat_order_spec = 82, /* additional_combat_order_spec  */
  YYSYMBOL_combat_application_spec = 83,   /* combat_application_spec  */
  YYSYMBOL_additional_combat_application_spec = 84, /* additional_combat_application_spec  */
  YYSYMBOL_build_cmd = 85,                 /* build_cmd  */
  YYSYMBOL_build_attr_spec = 86,           /* build_attr_spec  */
  YYSYMBOL_additional_build_attr_spec = 87, /* additional_build_attr_spec  */
  YYSYMBOL_deployable_ship = 88,           /* deployable_ship  */
  YYSYMBOL_deploy_cmd = 89,                /* deploy_cmd  */
  YYSYMBOL_target_systemship = 90,         /* target_systemship  */
  YYSYMBOL_chain_move_location = 91,       /* chain_move_location  */
  YYSYMBOL_move_cmd = 92,                  /* move_cmd  */
  YYSYMBOL_warpship_pick_destination = 93, /* warpship_pick_destination  */
  YYSYMBOL_warpship_drop_source = 94,      /* warpship_drop_source  */
  YYSYMBOL_pickdrop_cmd = 95,              /* pickdrop_cmd  */
  YYSYMBOL_rep_cmd = 96,                   /* rep_cmd  */
  YYSYMBOL_repair_attr_spec = 97,          /* repair_attr_spec  */
  YYSYMBOL_help_cmd = 98,                  /* help_cmd  */
  YYSYMBOL_help_composite = 99,            /* help_composite  */
  YYSYMBOL_help_command = 100              /* help_command  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   190

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  33
/* YYNRULES -- Number of rules.  */
#define YYNRULES  162
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  223

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   322


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   152,   152,   154,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   172,   182,   192,   202,   207,
     216,   221,   230,   234,   242,   260,   263,   269,   274,   277,
     280,   285,   291,   298,   303,   309,   316,   322,   334,   339,
     345,   353,   360,   367,   374,   381,   392,   403,   414,   427,
     436,   445,   454,   458,   462,   469,   478,   481,   484,   487,
     490,   495,   497,   500,   503,   506,   509,   515,   518,   521,
     524,   527,   532,   534,   537,   540,   543,   546,   554,   565,
     578,   591,   600,   609,   620,   632,   642,   652,   661,   669,
     678,   690,   693,   696,   699,   702,   705,   710,   712,   715,
     718,   721,   724,   727,   735,   746,   750,   771,   781,   784,
     798,   804,   835,   842,   851,   859,   866,   873,   883,   890,
     901,   908,   918,   922,   926,   930,   943,   950,   954,   961,
     965,   969,   973,   977,   981,   985,   989,   993,   997,  1001,
    1005,  1009,  1013,  1017,  1021,  1025,  1029,  1033,  1037,  1041,
    1045,  1049,  1053,  1057,  1061,  1065,  1069,  1073,  1077,  1081,
    1085,  1089,  1093
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TOK_INT",
  "TOK_STRING", "TOK_ADVANCED", "TOK_APPLY", "TOK_ATTACK", "TOK_BASIC",
  "TOK_BEAM", "TOK_BUILD", "TOK_BUILD_COMMIT", "TOK_BUILD_NEW",
  "TOK_BUILD_DRAFTS", "TOK_BUILD_CANCEL", "TOK_BUILD_SET_ATTR",
  "TOK_CANCEL", "TOK_COMBAT", "TOK_COMBAT_DRAFTS", "TOK_COMBAT_ORDER",
  "TOK_COMBAT_APPLY", "TOK_COMBAT_COMMIT", "TOK_COMBAT_CANCEL",
  "TOK_COMMIT", "TOK_CRT", "TOK_DELETE", "TOK_DEPLOY", "TOK_DODGE",
  "TOK_DONE", "TOK_DRAFTS", "TOK_DROP", "TOK_ESCAPE", "TOK_FLEET",
  "TOK_GALAXY", "TOK_HELP", "TOK_HEX", "TOK_LEARNING", "TOK_LIST",
  "TOK_LOAD", "TOK_MISSILE", "TOK_MOVE", "TOK_NEW", "TOK_NEXT",
  "TOK_NEXT_SHORT", "TOK_ONLINE", "TOK_ORDER", "TOK_PICK",
  "TOK_POWER_DRIVE", "TOK_QUIT", "TOK_REPAIR", "TOK_RESET", "TOK_RESUPPLY",
  "TOK_SAVE", "TOK_SCREEN", "TOK_SET_ATTR", "TOK_START_GAME", "TOK_STATS",
  "TOK_STATUS", "TOK_SYSTEM", "TOK_SURVEY", "TOK_TUBE", "TOK_PD_ASSIGN",
  "TOK_B_ASSIGN", "TOK_S_ASSIGN", "TOK_T_ASSIGN", "TOK_M_ASSIGN",
  "TOK_SR_ASSIGN", "TOK_UNKNOWN", "$accept", "commands", "command",
  "session_cmd", "info_cmd", "looking_cmd", "turn_cmd", "combat_cmd",
  "building_draft_ship", "combat_initiator_ship", "combat_damaged_ship",
  "combat_tactic", "combat_target_ship", "combat_order_spec",
  "additional_combat_order_spec", "combat_application_spec",
  "additional_combat_application_spec", "build_cmd", "build_attr_spec",
  "additional_build_attr_spec", "deployable_ship", "deploy_cmd",
  "target_systemship", "chain_move_location", "move_cmd",
  "warpship_pick_destination", "warpship_drop_source", "pickdrop_cmd",
  "rep_cmd", "repair_attr_spec", "help_cmd", "help_composite",
  "help_command", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-63)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-108)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
     -63,    69,   -63,    -4,   -63,     2,     4,   -63,   103,   100,
     -63,     5,    16,   -63,   -63,   -63,    17,    18,   -63,    19,
     -63,   -63,     1,    22,    24,    28,   -63,   -63,    35,   -63,
      45,   -63,    48,    49,    62,   -63,   -63,    56,    58,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,     4,    59,   103,    60,   -63,
     -63,   109,   109,   109,   109,   109,   109,   -63,    16,   -63,
     -63,   -63,     5,   -63,   107,   -63,    85,   -63,   -63,    61,
      67,    74,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,    81,
      67,    88,   -48,    63,   -63,   -63,   -63,   -63,    92,   -63,
     -63,    96,   -63,   -63,   109,   109,   109,   109,   109,   109,
     -63,   -63,   -63,   -63,   -63,   -63,    85,   107,   -63,   -63,
     -63,   101,   115,   115,   115,   115,   115,   -63,   -63,   -63,
     -63,   104,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   101,
     -63,   120,   115,   115,   115,   115,   115,   -63,   -63,   -63,
     -63,   -63,   104,   -63,   120,   125,   125,   125,   125,   125,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   125,   125,
     125,   125,   125,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    78,    88,     0,    82,    90,     0,    38,
      40,     0,     0,    42,    44,    26,    22,   105,    37,     0,
      30,    35,   126,     0,    20,   110,    36,    28,     0,    24,
     118,    18,   120,     0,     0,    29,    25,     0,    33,     3,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       4,   127,   129,    89,    87,    81,     0,     0,     0,    49,
      84,    97,    97,    97,    97,    97,    97,    86,     0,    43,
      41,    39,     0,    50,     0,    51,     0,    23,   104,     0,
     113,     0,   115,   128,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     130,   157,   158,   159,   160,   161,   162,    27,    21,     0,
     114,     0,     0,     0,    19,    17,    16,    15,    31,    34,
      83,     0,    85,    80,    97,    97,    97,    97,    97,    97,
      91,    92,    93,    94,    95,    96,     0,     0,    52,    53,
      54,     0,    72,    72,    72,    72,    72,    48,   106,   113,
     117,   108,   112,   116,   122,   123,   124,   125,   119,   121,
      32,    79,    98,    99,   100,   101,   102,   103,    47,     0,
      55,     0,    72,    72,    72,    72,    72,    67,    68,    69,
      70,    71,   108,   111,     0,    61,    61,    61,    61,    61,
      46,    73,    74,    75,    76,    77,   109,    45,    61,    61,
      61,    61,    61,    56,    57,    58,    59,    60,    62,    63,
      64,    65,    66
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,    55,    40,
      54,   -12,   -43,   -51,   -57,    -9,   -23,   -63,    87,   -62,
     -63,   -63,   128,   -35,   -63,   -63,    77,   -63,   -63,   -63,
     -63,   -63,   -63
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,    39,    40,    41,    42,    43,    44,    60,    74,
      76,   151,   181,   200,   213,   157,   187,    45,    67,   140,
      79,    46,    81,   193,    47,   163,    82,    48,    49,   168,
      50,    51,    52
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     141,   142,   143,   144,   145,    83,    58,    84,    59,    73,
      85,    86,    53,   164,   165,   166,   167,    87,    88,    54,
      75,    77,    78,    80,    89,    55,   117,    90,   118,    91,
      92,    93,   119,    94,    95,    96,    97,    56,    98,   120,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   122,
      57,   108,   123,   124,   109,   110,   111,   112,   113,   114,
     128,   115,   129,   131,   133,   158,   169,   125,   116,     2,
     126,  -107,   172,   173,   174,   175,   176,   177,   159,     3,
       4,     5,     6,     7,     8,   161,     9,    10,    11,    12,
      13,    14,   162,    15,    16,    17,   170,    18,   127,    19,
     171,    20,    21,    22,    23,   180,    68,    24,   192,    25,
     130,    26,   147,    27,   148,    28,    69,    29,    30,    31,
      32,    33,   146,    70,    34,    35,    36,    37,    38,    71,
     188,   189,   190,   191,   149,   179,   194,   178,   150,   214,
     215,   216,   217,   207,   132,    72,   152,   153,   154,   155,
     156,   218,   219,   220,   221,   222,   121,   206,   160,   201,
     202,   203,   204,   205,    61,    62,    63,    64,    65,    66,
     134,   135,   136,   137,   138,   139,   182,   183,   184,   185,
     186,   195,   196,   197,   198,   199,   208,   209,   210,   211,
     212
};

static const yytype_uint8 yycheck[] =
{
      62,    63,    64,    65,    66,     4,     4,     6,     4,     4,
       9,    10,    16,    61,    62,    63,    64,    16,    17,    23,
       4,     4,     4,     4,    23,    29,     4,    26,     4,    28,
      29,    30,     4,    32,    33,    34,    35,    41,    37,     4,
      39,    40,    41,    42,    43,    44,    45,    46,    47,     4,
      54,    50,     4,     4,    53,    54,    55,    56,    57,    58,
       4,    60,     4,     4,     4,     4,     3,     5,    67,     0,
       8,     4,   134,   135,   136,   137,   138,   139,     4,    10,
      11,    12,    13,    14,    15,     4,    17,    18,    19,    20,
      21,    22,     4,    24,    25,    26,     4,    28,    36,    30,
       4,    32,    33,    34,    35,     4,     6,    38,     4,    40,
      55,    42,    72,    44,     7,    46,    16,    48,    49,    50,
      51,    52,    68,    23,    55,    56,    57,    58,    59,    29,
     153,   154,   155,   156,    27,   147,   179,   146,    31,   196,
     197,   198,   199,   194,    57,    45,    61,    62,    63,    64,
      65,   208,   209,   210,   211,   212,    28,   192,    81,   182,
     183,   184,   185,   186,    61,    62,    63,    64,    65,    66,
      61,    62,    63,    64,    65,    66,    61,    62,    63,    64,
      65,    61,    62,    63,    64,    65,    61,    62,    63,    64,
      65
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    69,     0,    10,    11,    12,    13,    14,    15,    17,
      18,    19,    20,    21,    22,    24,    25,    26,    28,    30,
      32,    33,    34,    35,    38,    40,    42,    44,    46,    48,
      49,    50,    51,    52,    55,    56,    57,    58,    59,    70,
      71,    72,    73,    74,    75,    85,    89,    92,    95,    96,
      98,    99,   100,    16,    23,    29,    41,    54,     4,     4,
      76,    61,    62,    63,    64,    65,    66,    86,     6,    16,
      23,    29,    45,     4,    77,     4,    78,     4,     4,    88,
       4,    90,    94,     4,     6,     9,    10,    16,    17,    23,
      26,    28,    29,    30,    32,    33,    34,    35,    37,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    50,    53,
      54,    55,    56,    57,    58,    60,    67,     4,     4,     4,
       4,    90,     4,     4,     4,     5,     8,    36,     4,     4,
      76,     4,    86,     4,    61,    62,    63,    64,    65,    66,
      87,    87,    87,    87,    87,    87,    78,    77,     7,    27,
      31,    79,    61,    62,    63,    64,    65,    83,     4,     4,
      94,     4,     4,    93,    61,    62,    63,    64,    97,     3,
       4,     4,    87,    87,    87,    87,    87,    87,    83,    79,
       4,    80,    61,    62,    63,    64,    65,    84,    84,    84,
      84,    84,     4,    91,    80,    61,    62,    63,    64,    65,
      81,    84,    84,    84,    84,    84,    91,    81,    61,    62,
      63,    64,    65,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    68,    69,    69,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    72,    72,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    74,    74,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    76,
      77,    78,    79,    79,    79,    80,    81,    81,    81,    81,
      81,    82,    82,    82,    82,    82,    82,    83,    83,    83,
      83,    83,    84,    84,    84,    84,    84,    84,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    86,    86,    86,    86,    86,    86,    87,    87,    87,
      87,    87,    87,    87,    88,    89,    89,    90,    91,    91,
      92,    92,    93,    94,    95,    95,    95,    95,    96,    96,
      96,    96,    97,    97,    97,    97,    98,    98,    99,    99,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     2,     1,     2,
       1,     2,     1,     2,     1,     1,     1,     2,     1,     1,
       1,     2,     3,     1,     2,     1,     1,     1,     1,     2,
       1,     2,     1,     2,     1,     6,     5,     4,     3,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       2,     0,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     0,     2,     2,     2,     2,     2,     1,     4,
       3,     2,     1,     3,     2,     3,     2,     2,     1,     2,
       1,     2,     2,     2,     2,     2,     2,     0,     2,     2,
       2,     2,     2,     2,     1,     1,     3,     1,     0,     2,
       1,     4,     1,     1,     2,     2,     3,     3,     1,     3,
       1,     3,     1,     1,     1,     1,     1,     1,     2,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 15: /* session_cmd: TOK_START_GAME TOK_LEARNING  */
#line 173 "src/lang.y"
   {
        Logger::instance().info("Start a learning game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_scenario(ScenarioType::LEARNING)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
#line 1394 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 16: /* session_cmd: TOK_START_GAME TOK_BASIC  */
#line 183 "src/lang.y"
   {
        Logger::instance().info("Start a basic game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_scenario(ScenarioType::BASIC)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
#line 1407 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 17: /* session_cmd: TOK_START_GAME TOK_ADVANCED  */
#line 193 "src/lang.y"
   {
        Logger::instance().info("Start an advanced game scenario");
        ICmd *pCmd = StartCommand::Builder()
                      .set_scenario(ScenarioType::ADVANCED)
                      .build();
        pCmd->invoke();
        SafeDelete(pCmd);
   }
#line 1420 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 18: /* session_cmd: TOK_RESET  */
#line 203 "src/lang.y"
   {
        Logger::instance().info("Reset the current game, wiping everything");
   }
#line 1428 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 19: /* session_cmd: TOK_SAVE TOK_STRING  */
#line 208 "src/lang.y"
   {
        // the name of the 'thing' to save is supplied by the user.
        // it's  in the same vein as a filename.. just a-zA-Z0-9
        std::string save_record(*(yyvsp[0].sval));
        Logger::instance().info("Save game with id >"
                                + save_record + "<");
   }
#line 1440 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 20: /* session_cmd: TOK_LOAD  */
#line 217 "src/lang.y"
   {
        Logger::instance().info("List the games that are saved, if any");
   }
#line 1448 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 21: /* session_cmd: TOK_LOAD TOK_STRING  */
#line 222 "src/lang.y"
   {
        // the user can learn the filenames (they aren't files, they are
        // records in the DB) that are loadable by the name used when
        // the game was saved.
        std::string load_record(*(yyvsp[0].sval));
        Logger::instance().info("Load game >"
                     + load_record + "<");
   }
#line 1461 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 22: /* session_cmd: TOK_DELETE  */
#line 230 "src/lang.y"
                {
        // deleting the most recent game saved, if any
        Logger::instance().info("Delete the most recent saved");
   }
#line 1470 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 23: /* session_cmd: TOK_DELETE TOK_STRING  */
#line 234 "src/lang.y"
                           {
        // delete the named game (the same naming convention used for saving,
        // loading)
        std::string target_game(*(yyvsp[0].sval));
        Logger::instance().info("Delete the game saved under "
               ">" + target_game + "<");
   }
#line 1482 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 24: /* session_cmd: TOK_QUIT  */
#line 243 "src/lang.y"
   {
        // quit the game.  it will automatically save the game under the name
        // 'lastgame' overwriting lastgame.
        // the user has to 'load lastgame' to retrieve it when they play again
        // Everythign about the state of the game, ships, everything is saved
        //
        // Stateful conditions like player A and player B are logged in must be
        // true of coure for the State to be in GAME_START
        Logger::instance().info("Quit the game. Same as Reset but unsets game scenario.");
   }
#line 1497 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 25: /* info_cmd: TOK_STATUS  */
#line 260 "src/lang.y"
              {
      Logger::instance().info("Show Game Status, important records.");
   }
#line 1505 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 26: /* info_cmd: TOK_CRT  */
#line 263 "src/lang.y"
             {
      Logger::instance().info("Show the CRT");
   }
#line 1513 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 27: /* looking_cmd: TOK_HEX TOK_STRING  */
#line 269 "src/lang.y"
                     {
      std::string identifier(*(yyvsp[0].sval));
      Logger::instance().info("Location and spatial information "
                              "about current hex named >" + identifier + "<");
  }
#line 1523 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 28: /* looking_cmd: TOK_ONLINE  */
#line 274 "src/lang.y"
               {
      Logger::instance().info("Who's online and when?");
  }
#line 1531 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 29: /* looking_cmd: TOK_STATS  */
#line 277 "src/lang.y"
              {
      Logger::instance().info("Statistics about game, fleet, opponent");
  }
#line 1539 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 30: /* looking_cmd: TOK_FLEET  */
#line 280 "src/lang.y"
              {
      ICmd* pCmd = FleetCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1549 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 31: /* looking_cmd: TOK_SYSTEM TOK_STRING  */
#line 285 "src/lang.y"
                          {
      std::string identifier(*(yyvsp[0].sval));
      ICmd* pCmd = SystemCommand::Builder().setName(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1560 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 32: /* looking_cmd: TOK_SYSTEM TOK_STRING TOK_STRING  */
#line 291 "src/lang.y"
                                     {
      std::string identifier(*(yyvsp[-1].sval));
      std::string subcmd(*(yyvsp[0].sval));
      ICmd* pCmd = SystemCommand::Builder().setName(identifier).setSubcommand(subcmd).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1572 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 33: /* looking_cmd: TOK_SURVEY  */
#line 298 "src/lang.y"
               {
      ICmd* pCmd = SurveyCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1582 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 34: /* looking_cmd: TOK_SURVEY TOK_STRING  */
#line 303 "src/lang.y"
                          {
      std::string identifier(*(yyvsp[0].sval));
      ICmd* pCmd = SurveyCommand::Builder().setSystem(identifier).build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1593 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 35: /* looking_cmd: TOK_GALAXY  */
#line 309 "src/lang.y"
               {
      Logger::instance().info("Complete run-down of all systems, "
                              " all ships in the game.");
  }
#line 1602 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 36: /* turn_cmd: TOK_NEXT  */
#line 316 "src/lang.y"
           {
      Logger::instance().info("Advance active player to next phase");
      ICmd* pCmd = NextCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1613 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 37: /* turn_cmd: TOK_DONE  */
#line 322 "src/lang.y"
             {
      Logger::instance().info("Advance active player to first phase "
                      "of opponent, if possible");
      ICmd* pCmd = DoneCommand::Builder().build();
      if (pCmd && pCmd->invoke()) { /* success */ }
      SafeDelete(pCmd);
  }
#line 1625 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 38: /* combat_cmd: TOK_COMBAT  */
#line 334 "src/lang.y"
              {
      Logger::instance().info("Status of combat situation");
   }
#line 1633 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 39: /* combat_cmd: TOK_COMBAT TOK_DRAFTS  */
#line 339 "src/lang.y"
                           {
       ICmd* pCmd = CombatDraftsCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1643 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 40: /* combat_cmd: TOK_COMBAT_DRAFTS  */
#line 345 "src/lang.y"
                       {
       ICmd* pCmd = CombatDraftsCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1653 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 41: /* combat_cmd: TOK_COMBAT TOK_COMMIT  */
#line 353 "src/lang.y"
                           {
       ICmd* pCmd = CombatCommitCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1663 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 42: /* combat_cmd: TOK_COMBAT_COMMIT  */
#line 360 "src/lang.y"
                       {
       ICmd* pCmd = CombatCommitCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1673 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 43: /* combat_cmd: TOK_COMBAT TOK_CANCEL  */
#line 367 "src/lang.y"
                           {
       ICmd* pCmd = CombatCancelCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1683 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 44: /* combat_cmd: TOK_COMBAT_CANCEL  */
#line 374 "src/lang.y"
                       {
       ICmd* pCmd = CombatCancelCommand::Builder().build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
   }
#line 1693 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 45: /* combat_cmd: TOK_COMBAT TOK_ORDER combat_initiator_ship combat_tactic combat_target_ship combat_order_spec  */
#line 381 "src/lang.y"
                                                                                                   {
       // Build and invoke combat order command
       ICmd* pCmd = g_combat_order_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_order_builder;
       g_combat_order_builder = new CombatOrderCommand::Builder();
   }
#line 1707 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 46: /* combat_cmd: TOK_COMBAT_ORDER combat_initiator_ship combat_tactic combat_target_ship combat_order_spec  */
#line 392 "src/lang.y"
                                                                                               {
       // Build and invoke combat order command
       ICmd* pCmd = g_combat_order_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_order_builder;
       g_combat_order_builder = new CombatOrderCommand::Builder();
   }
#line 1721 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 47: /* combat_cmd: TOK_COMBAT TOK_APPLY combat_damaged_ship combat_application_spec  */
#line 403 "src/lang.y"
                                                                      {
       // Build and invoke combat apply command
       ICmd* pCmd = g_combat_apply_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_apply_builder;
       g_combat_apply_builder = new CombatApplyCommand::Builder();
   }
#line 1735 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 48: /* combat_cmd: TOK_COMBAT_APPLY combat_damaged_ship combat_application_spec  */
#line 414 "src/lang.y"
                                                                  {
       // Build and invoke combat apply command
       ICmd* pCmd = g_combat_apply_builder->build();
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       // Reset builder for next command
       delete g_combat_apply_builder;
       g_combat_apply_builder = new CombatApplyCommand::Builder();
   }
#line 1749 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 49: /* building_draft_ship: TOK_STRING  */
#line 427 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Drafting ship: "
                              ">" + ship_id + "<" );
      (yyval.sval) = (yyvsp[0].sval);  // Pass the string up
  }
#line 1760 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 50: /* combat_initiator_ship: TOK_STRING  */
#line 436 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Combat ship initiator: >" + ship_id + "<");
      g_combat_order_builder->ship_code(ship_id);
      delete (yyvsp[0].sval);
  }
#line 1771 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 51: /* combat_damaged_ship: TOK_STRING  */
#line 445 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Combat damaged ship: >" + ship_id + "<");
      g_combat_apply_builder->ship_code(ship_id);
      delete (yyvsp[0].sval);
  }
#line 1782 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 52: /* combat_tactic: TOK_ATTACK  */
#line 454 "src/lang.y"
             {
      Logger::instance().info("Combat Attack Tactic");
      g_combat_order_builder->tactic('A');
  }
#line 1791 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 53: /* combat_tactic: TOK_DODGE  */
#line 458 "src/lang.y"
              {
      Logger::instance().info("Combat Dodge Tactic");
      g_combat_order_builder->tactic('D');
  }
#line 1800 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 54: /* combat_tactic: TOK_ESCAPE  */
#line 462 "src/lang.y"
               {
      Logger::instance().info("Combat Escape Tactic");
      g_combat_order_builder->tactic('R');
  }
#line 1809 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 55: /* combat_target_ship: TOK_STRING  */
#line 469 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Combat target ship: >" + ship_id + "<");
      g_combat_order_builder->target(ship_id);
      delete (yyvsp[0].sval);
  }
#line 1820 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 56: /* combat_order_spec: TOK_PD_ASSIGN additional_combat_order_spec  */
#line 478 "src/lang.y"
                                             {
      g_combat_order_builder->drive_power((yyvsp[-1].ival));
  }
#line 1828 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 57: /* combat_order_spec: TOK_B_ASSIGN additional_combat_order_spec  */
#line 481 "src/lang.y"
                                              {
      g_combat_order_builder->beam_power((yyvsp[-1].ival));
  }
#line 1836 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 58: /* combat_order_spec: TOK_S_ASSIGN additional_combat_order_spec  */
#line 484 "src/lang.y"
                                              {
      g_combat_order_builder->screen_power((yyvsp[-1].ival));
  }
#line 1844 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 59: /* combat_order_spec: TOK_T_ASSIGN additional_combat_order_spec  */
#line 487 "src/lang.y"
                                              {
      g_combat_order_builder->tube_power((yyvsp[-1].ival));
  }
#line 1852 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 60: /* combat_order_spec: TOK_M_ASSIGN additional_combat_order_spec  */
#line 490 "src/lang.y"
                                              {
      g_combat_order_builder->missiles(std::to_string((yyvsp[-1].ival)));
  }
#line 1860 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 62: /* additional_combat_order_spec: TOK_PD_ASSIGN additional_combat_order_spec  */
#line 497 "src/lang.y"
                                               {
      g_combat_order_builder->drive_power((yyvsp[-1].ival));
  }
#line 1868 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 63: /* additional_combat_order_spec: TOK_B_ASSIGN additional_combat_order_spec  */
#line 500 "src/lang.y"
                                              {
      g_combat_order_builder->beam_power((yyvsp[-1].ival));
  }
#line 1876 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 64: /* additional_combat_order_spec: TOK_S_ASSIGN additional_combat_order_spec  */
#line 503 "src/lang.y"
                                              {
      g_combat_order_builder->screen_power((yyvsp[-1].ival));
  }
#line 1884 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 65: /* additional_combat_order_spec: TOK_T_ASSIGN additional_combat_order_spec  */
#line 506 "src/lang.y"
                                              {
      g_combat_order_builder->tube_power((yyvsp[-1].ival));
  }
#line 1892 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 66: /* additional_combat_order_spec: TOK_M_ASSIGN additional_combat_order_spec  */
#line 509 "src/lang.y"
                                              {
      g_combat_order_builder->missiles(std::to_string((yyvsp[-1].ival)));
  }
#line 1900 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 67: /* combat_application_spec: TOK_PD_ASSIGN additional_combat_application_spec  */
#line 515 "src/lang.y"
                                                   {
      g_combat_apply_builder->assign("D", (yyvsp[-1].ival));
  }
#line 1908 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 68: /* combat_application_spec: TOK_B_ASSIGN additional_combat_application_spec  */
#line 518 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("B", (yyvsp[-1].ival));
  }
#line 1916 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 69: /* combat_application_spec: TOK_S_ASSIGN additional_combat_application_spec  */
#line 521 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("S", (yyvsp[-1].ival));
  }
#line 1924 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 70: /* combat_application_spec: TOK_T_ASSIGN additional_combat_application_spec  */
#line 524 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("T", (yyvsp[-1].ival));
  }
#line 1932 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 71: /* combat_application_spec: TOK_M_ASSIGN additional_combat_application_spec  */
#line 527 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("M", (yyvsp[-1].ival));
  }
#line 1940 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 73: /* additional_combat_application_spec: TOK_PD_ASSIGN additional_combat_application_spec  */
#line 534 "src/lang.y"
                                                     {
      g_combat_apply_builder->assign("D", (yyvsp[-1].ival));
  }
#line 1948 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 74: /* additional_combat_application_spec: TOK_B_ASSIGN additional_combat_application_spec  */
#line 537 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("B", (yyvsp[-1].ival));
  }
#line 1956 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 75: /* additional_combat_application_spec: TOK_S_ASSIGN additional_combat_application_spec  */
#line 540 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("S", (yyvsp[-1].ival));
  }
#line 1964 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 76: /* additional_combat_application_spec: TOK_T_ASSIGN additional_combat_application_spec  */
#line 543 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("T", (yyvsp[-1].ival));
  }
#line 1972 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 77: /* additional_combat_application_spec: TOK_M_ASSIGN additional_combat_application_spec  */
#line 546 "src/lang.y"
                                                    {
      g_combat_apply_builder->assign("M", (yyvsp[-1].ival));
  }
#line 1980 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 78: /* build_cmd: TOK_BUILD  */
#line 554 "src/lang.y"
            {
      Logger::instance().info("Without arguments, shows "
                              "current build state");
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 1994 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 79: /* build_cmd: TOK_BUILD TOK_NEW TOK_STRING TOK_STRING  */
#line 565 "src/lang.y"
                                            {
      std::string code(*(yyvsp[-1].sval));
      std::string name(*(yyvsp[0].sval));
      Logger::instance().info("Create new draft: " + code + " '" + name + "'");
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2010 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 80: /* build_cmd: TOK_BUILD_NEW TOK_STRING TOK_STRING  */
#line 578 "src/lang.y"
                                        {
      std::string code(*(yyvsp[-1].sval));
      std::string name(*(yyvsp[0].sval));
      Logger::instance().info("Create new draft: " + code + " '" + name + "'");
      ICmd *pCmd = BuildNewCommand::Builder()
                  .set_ship_code(code)
                  .set_ship_name(name)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2026 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 81: /* build_cmd: TOK_BUILD TOK_DRAFTS  */
#line 591 "src/lang.y"
                         {
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2038 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 82: /* build_cmd: TOK_BUILD_DRAFTS  */
#line 600 "src/lang.y"
                     {
      Logger::instance().info("List all pending build drafts");
      ICmd *pCmd = BuildListDraftsCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2050 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 83: /* build_cmd: TOK_BUILD TOK_DRAFTS building_draft_ship  */
#line 609 "src/lang.y"
                                             {
      std::string ship_code = *(yyvsp[0].sval);
      Logger::instance().info("Show draft details: " + ship_code);
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_code(ship_code)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2064 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 84: /* build_cmd: TOK_BUILD_DRAFTS building_draft_ship  */
#line 620 "src/lang.y"
                                         {
      std::string ship_code = *(yyvsp[0].sval);
      Logger::instance().info("Show draft details: " + ship_code);
      ICmd *pCmd = BuildShowDraftCommand::Builder()
                  .set_draft_code(ship_code)
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2078 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 85: /* build_cmd: TOK_BUILD TOK_SET_ATTR build_attr_spec  */
#line 632 "src/lang.y"
                                           {
      Logger::instance().info("Set attributes on current draft");
      ICmd *pCmd = g_build_set_builder->build();
      pCmd->invoke();
      SafeDelete(pCmd);
      delete g_build_set_builder;
      g_build_set_builder = new BuildSetAttributeCommand::Builder();
  }
#line 2091 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 86: /* build_cmd: TOK_BUILD_SET_ATTR build_attr_spec  */
#line 642 "src/lang.y"
                                       {
      Logger::instance().info("Set attributes on current draft");
      ICmd *pCmd = g_build_set_builder->build();
      pCmd->invoke();
      SafeDelete(pCmd);
      delete g_build_set_builder;
      g_build_set_builder = new BuildSetAttributeCommand::Builder();
  }
#line 2104 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 87: /* build_cmd: TOK_BUILD TOK_COMMIT  */
#line 652 "src/lang.y"
                         {
      Logger::instance().info("Build commit - committing current draft");
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2116 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 88: /* build_cmd: TOK_BUILD_COMMIT  */
#line 661 "src/lang.y"
                     {
      Logger::instance().info("Build commit - committing current draft");
      ICmd *pCmd = BuildCommitCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2128 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 89: /* build_cmd: TOK_BUILD TOK_CANCEL  */
#line 669 "src/lang.y"
                         {
      Logger::instance().info("Build cancel - canceling current draft");
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2140 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 90: /* build_cmd: TOK_BUILD_CANCEL  */
#line 678 "src/lang.y"
                     {
      Logger::instance().info("Build cancel - canceling current draft");
      ICmd *pCmd = BuildCancelCommand::Builder()
                  .build();
      pCmd->invoke();
      SafeDelete(pCmd);
  }
#line 2152 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 91: /* build_attr_spec: TOK_PD_ASSIGN additional_build_attr_spec  */
#line 690 "src/lang.y"
                                           {
      g_build_set_builder->set_pd((yyvsp[-1].ival));
  }
#line 2160 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 92: /* build_attr_spec: TOK_B_ASSIGN additional_build_attr_spec  */
#line 693 "src/lang.y"
                                            {
      g_build_set_builder->set_b((yyvsp[-1].ival));
  }
#line 2168 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 93: /* build_attr_spec: TOK_S_ASSIGN additional_build_attr_spec  */
#line 696 "src/lang.y"
                                            {
      g_build_set_builder->set_s((yyvsp[-1].ival));
  }
#line 2176 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 94: /* build_attr_spec: TOK_T_ASSIGN additional_build_attr_spec  */
#line 699 "src/lang.y"
                                            {
      g_build_set_builder->set_t((yyvsp[-1].ival));
  }
#line 2184 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 95: /* build_attr_spec: TOK_M_ASSIGN additional_build_attr_spec  */
#line 702 "src/lang.y"
                                            {
      g_build_set_builder->set_m((yyvsp[-1].ival));
  }
#line 2192 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 96: /* build_attr_spec: TOK_SR_ASSIGN additional_build_attr_spec  */
#line 705 "src/lang.y"
                                             {
      g_build_set_builder->set_sr((yyvsp[-1].ival));
  }
#line 2200 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 98: /* additional_build_attr_spec: TOK_PD_ASSIGN additional_build_attr_spec  */
#line 712 "src/lang.y"
                                             {
      g_build_set_builder->set_pd((yyvsp[-1].ival));
  }
#line 2208 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 99: /* additional_build_attr_spec: TOK_B_ASSIGN additional_build_attr_spec  */
#line 715 "src/lang.y"
                                            {
      g_build_set_builder->set_b((yyvsp[-1].ival));
  }
#line 2216 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 100: /* additional_build_attr_spec: TOK_S_ASSIGN additional_build_attr_spec  */
#line 718 "src/lang.y"
                                            {
      g_build_set_builder->set_s((yyvsp[-1].ival));
  }
#line 2224 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 101: /* additional_build_attr_spec: TOK_T_ASSIGN additional_build_attr_spec  */
#line 721 "src/lang.y"
                                            {
      g_build_set_builder->set_t((yyvsp[-1].ival));
  }
#line 2232 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 102: /* additional_build_attr_spec: TOK_M_ASSIGN additional_build_attr_spec  */
#line 724 "src/lang.y"
                                            {
      g_build_set_builder->set_m((yyvsp[-1].ival));
  }
#line 2240 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 103: /* additional_build_attr_spec: TOK_SR_ASSIGN additional_build_attr_spec  */
#line 727 "src/lang.y"
                                             {
      g_build_set_builder->set_sr((yyvsp[-1].ival));
  }
#line 2248 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 104: /* deployable_ship: TOK_STRING  */
#line 735 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Deployable ship: "
                              ">" + ship_id + "<" );
      (yyval.sval) = (yyvsp[0].sval);
  }
#line 2259 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 105: /* deploy_cmd: TOK_DEPLOY  */
#line 746 "src/lang.y"
              {
       Logger::instance().info("Show current self deployment");
       Logger::instance().info("Show opponent deployment");
   }
#line 2268 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 106: /* deploy_cmd: TOK_DEPLOY deployable_ship TOK_STRING  */
#line 750 "src/lang.y"
                                           {
       std::string ship(*(yyvsp[-1].sval));
       std::string destination(*(yyvsp[0].sval));
       Logger::instance().info("Attempt to deploy ship "
                               ">" + ship + "<" 
                               " at destination: "
                               ">" + destination + "<");

       ICmd* pCmd = DeployCommand::Builder()
                    .ship_code(ship)
                    .system_name(destination)
                    .build();

       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       delete (yyvsp[-1].sval);
       delete (yyvsp[0].sval);
   }
#line 2291 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 107: /* target_systemship: TOK_STRING  */
#line 771 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Target SystemShip: "
                              ">" + ship_id + "<");
  }
#line 2301 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 108: /* chain_move_location: %empty  */
#line 781 "src/lang.y"
   {
       (yyval.vec_sval) = new std::vector<std::string>;
   }
#line 2309 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 109: /* chain_move_location: TOK_STRING chain_move_location  */
#line 784 "src/lang.y"
                                    {
       std::string waypoint(*(yyvsp[-1].sval));
       Logger::instance().info("Additional waypoint on chain: >"
                                + waypoint + "<");
       (yyval.vec_sval) = (yyvsp[0].vec_sval);
       (yyval.vec_sval)->insert((yyval.vec_sval)->begin(), waypoint);
       delete (yyvsp[-1].sval);
   }
#line 2322 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 110: /* move_cmd: TOK_MOVE  */
#line 798 "src/lang.y"
           {
       Logger::instance().info("Show move status");
  }
#line 2330 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 111: /* move_cmd: TOK_MOVE TOK_STRING TOK_STRING chain_move_location  */
#line 804 "src/lang.y"
                                                       {
       std::string ship(*(yyvsp[-2].sval));
       std::string first_dest(*(yyvsp[-1].sval));
       std::vector<std::string>* waypoints = (yyvsp[0].vec_sval);
       
       Logger::instance().info("Moving ship >" 
                               + ship 
                               + "< to >" 
                               + first_dest + "<");
       
       // Build move command with all destinations
       ICmd* pCmd = MoveCommand::Builder()
            .ship_code(ship)
            .add_destination(first_dest)
            .add_waypoints(waypoints)
            .build();
       
       if (pCmd && pCmd->invoke()) { /* success */ }
       SafeDelete(pCmd);
       
       delete (yyvsp[-2].sval);
       delete (yyvsp[-1].sval);
       delete waypoints;
  }
#line 2359 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 112: /* warpship_pick_destination: TOK_STRING  */
#line 835 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Pick destination warpship: "
                              ">" + ship_id + "<");
  }
#line 2369 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 113: /* warpship_drop_source: TOK_STRING  */
#line 842 "src/lang.y"
             {
      std::string ship_id(*(yyvsp[0].sval));
      Logger::instance().info("Drop source warpship: "
                              ">" + ship_id + "<");
  }
#line 2379 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 114: /* pickdrop_cmd: TOK_PICK TOK_STRING  */
#line 851 "src/lang.y"
                      {
     std::string location(*(yyvsp[0].sval));
     Logger::instance().info("Status of what can be picked up at hex "
                 ">" + location + "<");
  }
#line 2389 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 115: /* pickdrop_cmd: TOK_DROP warpship_drop_source  */
#line 859 "src/lang.y"
                                  {
     Logger::instance().info("Only list what may be dropped from "
                             "target WarpShip");
  }
#line 2398 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 116: /* pickdrop_cmd: TOK_PICK target_systemship warpship_pick_destination  */
#line 866 "src/lang.y"
                                                         {
     Logger::instance().info("Picking up SystemShip to rack "
                             "in target WarpShip");

  }
#line 2408 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 117: /* pickdrop_cmd: TOK_DROP target_systemship warpship_drop_source  */
#line 873 "src/lang.y"
                                                    {
     Logger::instance().info("Dropping target SystemShipship from "
                             "target WarpShip");
  }
#line 2417 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 118: /* rep_cmd: TOK_REPAIR  */
#line 883 "src/lang.y"
             {
     ICmd* pCmd = RepairCommand::Builder().build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
#line 2427 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 119: /* rep_cmd: TOK_REPAIR TOK_STRING repair_attr_spec  */
#line 890 "src/lang.y"
                                           {
     // g_repair_builder populated by repair_attr_spec
     g_repair_builder->set_ship_code(*(yyvsp[-1].sval));
     ICmd* pCmd = g_repair_builder->build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
     delete g_repair_builder;
     g_repair_builder = new RepairCommand::Builder();
  }
#line 2441 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 120: /* rep_cmd: TOK_RESUPPLY  */
#line 901 "src/lang.y"
                 {
     ICmd* pCmd = ResupplyCommand::Builder().build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
#line 2451 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 121: /* rep_cmd: TOK_RESUPPLY TOK_STRING TOK_INT  */
#line 908 "src/lang.y"
                                    {
     std::string ship(*(yyvsp[-1].sval));
     int qty = (int) (yyvsp[0].ival);
     ICmd* pCmd = ResupplyCommand::Builder().set_ship_code(ship).set_missiles(qty).build();
     if (pCmd && pCmd->invoke()) { /* success */ }
     SafeDelete(pCmd);
  }
#line 2463 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 122: /* repair_attr_spec: TOK_PD_ASSIGN  */
#line 918 "src/lang.y"
                {
      g_repair_builder->set_attribute("pd");
      g_repair_builder->set_amount((yyvsp[0].ival));
  }
#line 2472 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 123: /* repair_attr_spec: TOK_B_ASSIGN  */
#line 922 "src/lang.y"
                 {
      g_repair_builder->set_attribute("b");
      g_repair_builder->set_amount((yyvsp[0].ival));
  }
#line 2481 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 124: /* repair_attr_spec: TOK_S_ASSIGN  */
#line 926 "src/lang.y"
                 {
      g_repair_builder->set_attribute("s");
      g_repair_builder->set_amount((yyvsp[0].ival));
  }
#line 2490 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 125: /* repair_attr_spec: TOK_T_ASSIGN  */
#line 930 "src/lang.y"
                 {
      g_repair_builder->set_attribute("t");
      g_repair_builder->set_amount((yyvsp[0].ival));
  }
#line 2499 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 126: /* help_cmd: TOK_HELP  */
#line 944 "src/lang.y"
  {
       // show help screen
       // show all topics
      Logger::instance().info("Show help screen");
      Logger::instance().info("List all help topics");
  }
#line 2510 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 128: /* help_composite: TOK_HELP TOK_STRING  */
#line 955 "src/lang.y"
  {
      Logger::instance().info("Help about topic");
      std::string topic(*(yyvsp[0].sval));

      Logger::instance().info(topic);
  }
#line 2521 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 130: /* help_command: TOK_HELP TOK_SET_ATTR  */
#line 966 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_SET_ATTR");
    }
#line 2529 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 131: /* help_command: TOK_HELP TOK_APPLY  */
#line 970 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_APPLY");
    }
#line 2537 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 132: /* help_command: TOK_HELP TOK_BEAM  */
#line 974 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_BEAM");
    }
#line 2545 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 133: /* help_command: TOK_HELP TOK_BUILD  */
#line 978 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_BUILD");
    }
#line 2553 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 134: /* help_command: TOK_HELP TOK_CANCEL  */
#line 982 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_CANCEL");
    }
#line 2561 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 135: /* help_command: TOK_HELP TOK_COMBAT  */
#line 986 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_COMBAT");
    }
#line 2569 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 136: /* help_command: TOK_HELP TOK_COMMIT  */
#line 990 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_COMMIT");
    }
#line 2577 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 137: /* help_command: TOK_HELP TOK_DEPLOY  */
#line 994 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_DEPLOY");
    }
#line 2585 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 138: /* help_command: TOK_HELP TOK_DONE  */
#line 998 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_DONE");
    }
#line 2593 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 139: /* help_command: TOK_HELP TOK_DRAFTS  */
#line 1002 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_DRAFTS");
    }
#line 2601 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 140: /* help_command: TOK_HELP TOK_DROP  */
#line 1006 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_DROP");
    }
#line 2609 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 141: /* help_command: TOK_HELP TOK_FLEET  */
#line 1010 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_FLEET");
    }
#line 2617 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 142: /* help_command: TOK_HELP TOK_GALAXY  */
#line 1014 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_GALAXY");
    }
#line 2625 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 143: /* help_command: TOK_HELP TOK_HELP  */
#line 1018 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_HELP");
    }
#line 2633 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 144: /* help_command: TOK_HELP TOK_HEX  */
#line 1022 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_HEX");
    }
#line 2641 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 145: /* help_command: TOK_HELP TOK_LIST  */
#line 1026 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_LIST");
    }
#line 2649 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 146: /* help_command: TOK_HELP TOK_MISSILE  */
#line 1030 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_MISSILE");
    }
#line 2657 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 147: /* help_command: TOK_HELP TOK_MOVE  */
#line 1034 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_MOVE");
    }
#line 2665 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 148: /* help_command: TOK_HELP TOK_NEW  */
#line 1038 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_NEW");
    }
#line 2673 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 149: /* help_command: TOK_HELP TOK_NEXT  */
#line 1042 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_NEXT");
    }
#line 2681 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 150: /* help_command: TOK_HELP TOK_NEXT_SHORT  */
#line 1046 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_NEXT_SHORT");
    }
#line 2689 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 151: /* help_command: TOK_HELP TOK_ONLINE  */
#line 1050 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_ONLINE");
    }
#line 2697 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 152: /* help_command: TOK_HELP TOK_ORDER  */
#line 1054 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_ORDER");
    }
#line 2705 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 153: /* help_command: TOK_HELP TOK_PICK  */
#line 1058 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_PICK");
    }
#line 2713 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 154: /* help_command: TOK_HELP TOK_POWER_DRIVE  */
#line 1062 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_POWER_DRIVE");
    }
#line 2721 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 155: /* help_command: TOK_HELP TOK_RESET  */
#line 1066 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_RESET");
    }
#line 2729 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 156: /* help_command: TOK_HELP TOK_SCREEN  */
#line 1070 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_SCREEN");
    }
#line 2737 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 157: /* help_command: TOK_HELP TOK_START_GAME  */
#line 1074 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_START_GAME");
    }
#line 2745 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 158: /* help_command: TOK_HELP TOK_STATS  */
#line 1078 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_STATS");
    }
#line 2753 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 159: /* help_command: TOK_HELP TOK_STATUS  */
#line 1082 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_STATUS");
    }
#line 2761 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 160: /* help_command: TOK_HELP TOK_SYSTEM  */
#line 1086 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_SYSTEM");
    }
#line 2769 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 161: /* help_command: TOK_HELP TOK_TUBE  */
#line 1090 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_TUBE");
    }
#line 2777 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;

  case 162: /* help_command: TOK_HELP TOK_UNKNOWN  */
#line 1094 "src/lang.y"
    {
        Logger::instance().info("Help about TOK_UNKNOWN");
    }
#line 2785 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"
    break;


#line 2789 "/home/jdw/gh/KeplersHorizon/site/server/b22/lang.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1102 "src/lang.y"

void yyerror(const char* s)
{
   Logger::instance().error("Parse error: " + std::string(s));
}
