//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "ecl_bridge.h"

#include <cctype>

#include "logger.h"

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------

bool EclBridge::s_booted = false;

// ----------------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------------

bool EclBridge::boot()
{
    if (s_booted)
    {
        return true;
    }

    // Boot ECL with no command-line args
    char* argv[] = {(char*)"kh", NULL};
    cl_boot(1, argv);

    s_booted = true;
    return true;
}

void EclBridge::shutdown()
{
    if (s_booted)
    {
        cl_shutdown();
        s_booted = false;
    }
}

bool EclBridge::is_booted()
{
    return s_booted;
}

// ----------------------------------------------------------------------------
// Load Lisp Files
// ----------------------------------------------------------------------------

bool EclBridge::load_file(const std::string& path)
{
    if (!s_booted)
    {
        return false;
    }

    cl_object lisp_path = make_string(path);
    cl_object result = cl_load(1, lisp_path);

    return (result != ECL_NIL);
}

// ----------------------------------------------------------------------------
// Calculate: Marshal -> Call -> Unmarshal
// ----------------------------------------------------------------------------

bool EclBridge::calculate(const AASlate& slate,
                          std::vector<std::string>& commands_out)
{
    commands_out.clear();

    if (!s_booted)
    {
        return false;
    }

    // Marshal slate to Lisp alist
    cl_object slate_alist = marshal_slate(slate);

    // Get the aa-calculate function symbol
    cl_object fn_sym = ecl_make_symbol("AA-CALCULATE", "CL-USER");

    // Call (aa-calculate slate)
    cl_object result = ECL_NIL;
    ECL_HANDLER_CASE_BEGIN(ecl_process_env(), ecl_list1(ECL_T))
    {
        result = cl_funcall(2, fn_sym, slate_alist);
    }
    ECL_HANDLER_CASE(1, condition)
    {
        (void)condition; // Lisp error - ECL logs to stdout
        return false;
    }
    ECL_HANDLER_CASE_END;

    // Unmarshal result to commands
    return unmarshal_commands(result, commands_out);
}

// ----------------------------------------------------------------------------
// Marshal C++ -> Lisp
// ----------------------------------------------------------------------------

cl_object EclBridge::marshal_slate(const AASlate& slate)
{
    // Build alist: ((:key . value) ...)
    cl_object alist = ECL_NIL;

    // Add fields in reverse order (cons prepends)
    alist = make_cons(
        make_cons(make_keyword("in-combat"), make_bool(slate.in_combat)),
        alist);
    alist = make_cons(make_cons(make_keyword("contested-hexes"),
                                marshal_string_list(slate.contested_hexes)),
                      alist);
    alist = make_cons(make_cons(make_keyword("active-combats"),
                                marshal_combat_list(slate.active_combats)),
                      alist);
    alist = make_cons(
        make_cons(make_keyword("drafts"), marshal_ship_list(slate.draft_ships)),
        alist);
    alist = make_cons(make_cons(make_keyword("enemy-ships"),
                                marshal_ship_list(slate.enemy_ships)),
                      alist);
    alist = make_cons(make_cons(make_keyword("own-ships"),
                                marshal_ship_list(slate.own_ships)),
                      alist);
    alist = make_cons(make_cons(make_keyword("own-bases"),
                                marshal_string_list(slate.own_base_hexes)),
                      alist);
    alist = make_cons(make_cons(make_keyword("enemy-bases"),
                                marshal_string_list(slate.enemy_base_hexes)),
                      alist);
    alist = make_cons(
        make_cons(make_keyword("home-side"), make_string(slate.home_side)),
        alist);
    alist = make_cons(
        make_cons(make_keyword("enemy-vp"), make_int(slate.enemy_vp)), alist);
    alist = make_cons(make_cons(make_keyword("vp"), make_int(slate.vp)), alist);
    alist = make_cons(
        make_cons(make_keyword("tech-level"), make_int(slate.tech_level)),
        alist);
    alist = make_cons(
        make_cons(make_keyword("credits"), make_int(slate.credits)), alist);
    alist = make_cons(
        make_cons(make_keyword("game-over"), make_bool(slate.game_over)),
        alist);
    alist = make_cons(
        make_cons(make_keyword("is-aa-turn"), make_bool(slate.is_aa_turn)),
        alist);
    alist = make_cons(make_cons(make_keyword("active-player"),
                                ECL_CODE_CHAR(slate.active_player)),
                      alist);
    alist = make_cons(
        make_cons(make_keyword("phase"), make_int(slate.phase_index)), alist);
    alist = make_cons(make_cons(make_keyword("round"), make_int(slate.round)),
                      alist);
    alist = make_cons(
        make_cons(make_keyword("aa-player"), ECL_CODE_CHAR(slate.aa_player)),
        alist);
    alist = make_cons(
        make_cons(make_keyword("game-id"), make_int(slate.game_id)), alist);

    return alist;
}

cl_object EclBridge::marshal_ship_list(const std::vector<AAShipInfo>& ships)
{
    cl_object list = ECL_NIL;

    // Build in reverse, then reverse at end
    for (auto it = ships.rbegin(); it != ships.rend(); ++it)
    {
        const AAShipInfo& s = *it;

        // Ship is a plist: (:code "X" :name "Y" :hex "Z" :pd N ...)
        cl_object plist = ECL_NIL;

        plist = make_cons(make_bool(s.is_warpship), plist);
        plist = make_cons(make_keyword("warpship"), plist);

        // Revealed enemy order (from prior round, public info)
        if (s.last_tactic != '\0')
        {
            plist = make_cons(ECL_CODE_CHAR(s.last_tactic), plist);
        }
        else
        {
            plist = make_cons(ECL_NIL, plist);
        }
        plist = make_cons(make_keyword("last-tactic"), plist);

        plist = make_cons(make_int(s.last_drive), plist);
        plist = make_cons(make_keyword("last-drive"), plist);

        plist = make_cons(make_int(s.last_beam), plist);
        plist = make_cons(make_keyword("last-beam"), plist);

        // AI movement suggestion (empty if no valid move)
        plist = make_cons(make_string(s.suggested_destination), plist);
        plist = make_cons(make_keyword("suggested-dest"), plist);

        // Combat state
        plist = make_cons(make_bool(s.needs_combat_order), plist);
        plist = make_cons(make_keyword("needs-order"), plist);

        plist = make_cons(make_int(s.pending_damage), plist);
        plist = make_cons(make_keyword("pending-damage"), plist);

        plist = make_cons(make_bool(s.escape_pending), plist);
        plist = make_cons(make_keyword("escape-pending"), plist);

        plist = make_cons(make_int(s.tech_level), plist);
        plist = make_cons(make_keyword("tech"), plist);

        plist = make_cons(make_int(s.sr), plist);
        plist = make_cons(make_keyword("sr"), plist);

        plist = make_cons(make_int(s.missile), plist);
        plist = make_cons(make_keyword("m"), plist);

        plist = make_cons(make_int(s.tube), plist);
        plist = make_cons(make_keyword("t"), plist);

        plist = make_cons(make_int(s.screen), plist);
        plist = make_cons(make_keyword("s"), plist);

        plist = make_cons(make_int(s.beam), plist);
        plist = make_cons(make_keyword("b"), plist);

        plist = make_cons(make_int(s.pd), plist);
        plist = make_cons(make_keyword("pd"), plist);

        plist = make_cons(make_int(s.base_pd), plist);
        plist = make_cons(make_keyword("base-pd"), plist);

        plist = make_cons(make_string(s.hex_id), plist);
        plist = make_cons(make_keyword("hex"), plist);

        plist = make_cons(make_string(s.name), plist);
        plist = make_cons(make_keyword("name"), plist);

        plist = make_cons(make_string(s.code), plist);
        plist = make_cons(make_keyword("code"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object
EclBridge::marshal_string_list(const std::vector<std::string>& strings)
{
    cl_object list = ECL_NIL;

    for (auto it = strings.rbegin(); it != strings.rend(); ++it)
    {
        list = make_cons(make_string(*it), list);
    }

    return list;
}

cl_object
EclBridge::marshal_combat_list(const std::vector<AACombatHex>& combats)
{
    cl_object list = ECL_NIL;

    for (auto it = combats.rbegin(); it != combats.rend(); ++it)
    {
        const AACombatHex& ch = *it;

        // Combat hex plist with stalemate tracking
        cl_object plist = ECL_NIL;

        plist = make_cons(make_bool(ch.ai_is_attacker), plist);
        plist = make_cons(make_keyword("ai-attacker"), plist);

        plist = make_cons(make_int(ch.stalemate_counter), plist);
        plist = make_cons(make_keyword("stalemate"), plist);

        plist = make_cons(make_bool(ch.ai_committed), plist);
        plist = make_cons(make_keyword("ai-committed"), plist);

        plist = make_cons(make_int(ch.round), plist);
        plist = make_cons(make_keyword("round"), plist);

        plist = make_cons(make_int(ch.stage), plist);
        plist = make_cons(make_keyword("stage"), plist);

        plist = make_cons(make_string(ch.hex_id), plist);
        plist = make_cons(make_keyword("hex"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

// ----------------------------------------------------------------------------
// Unmarshal Lisp -> C++
// ----------------------------------------------------------------------------

bool EclBridge::unmarshal_commands(cl_object result,
                                   std::vector<std::string>& commands_out)
{
    // Result should be a list of plists: ((:cmd "X" :args "Y") ...)
    if (result == ECL_NIL)
    {
        return true;
    }

    cl_object kw_cmd = make_keyword("cmd");
    cl_object kw_args = make_keyword("args");

    while (result != ECL_NIL && ECL_LISTP(result))
    {
        cl_object spec = cl_car(result);

        if (ECL_LISTP(spec))
        {
            // Extract :cmd and :args from plist
            cl_object cmd_val = cl_getf(2, spec, kw_cmd);
            cl_object args_val = cl_getf(2, spec, kw_args);

            std::string cmd = extract_string(cmd_val);
            std::string args = extract_string(args_val);

            // Combine cmd and args
            std::string full_cmd = cmd;
            if (!args.empty())
            {
                full_cmd += " " + args;
            }

            if (!full_cmd.empty())
            {
                commands_out.push_back(full_cmd);
            }
        }

        result = cl_cdr(result);
    }

    return true;
}

std::string EclBridge::extract_string(cl_object obj)
{
    if (obj == ECL_NIL || obj == nullptr)
    {
        return "";
    }

    // Convert to string if not already
    cl_object str_obj = obj;
    if (!ECL_STRINGP(obj))
    {
        str_obj = cl_princ_to_string(obj);
    }

    // ECL has two string types:
    // - base-string: 8-bit characters (ecl_base_char)
    // - extended-string: 32-bit characters (ecl_character)
    std::string result;
    cl_index len = str_obj->string.fillp;

    if (ECL_BASE_STRING_P(str_obj))
    {
        // Base string: 8-bit chars, direct copy
        const char* data = (const char*)str_obj->base_string.self;
        result.assign(data, len);
    }
    else
    {
        // Extended string: 32-bit chars (ecl_character)
        const ecl_character* data = str_obj->string.self;
        result.reserve(len);
        for (cl_index idx = 0; idx < len; ++idx)
        {
            result.push_back(static_cast<char>(data[idx] & 0xFF));
        }
    }

    return result;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

cl_object EclBridge::make_keyword(const char* name)
{
    // ECL keywords must be uppercase to match Lisp reader default
    std::string upper;
    for (const char* p = name; *p; ++p)
    {
        upper.push_back(static_cast<char>(std::toupper(*p)));
    }
    return ecl_make_keyword(upper.c_str());
}

cl_object EclBridge::make_cons(cl_object car, cl_object cdr)
{
    return ecl_cons(car, cdr);
}

cl_object EclBridge::make_string(const std::string& s)
{
    return ecl_make_simple_base_string(s.c_str(), s.length());
}

cl_object EclBridge::make_int(int val)
{
    return ecl_make_fixnum(val);
}

cl_object EclBridge::make_bool(bool val)
{
    return val ? ECL_T : ECL_NIL;
}
