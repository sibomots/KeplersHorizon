///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "ecl_bridge.h"

#include <cctype>
#include <format>

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
                          std::vector<std::string>& commands_out,
                          std::vector<std::pair<std::string, double>>& metrics_out)
{
    commands_out.clear();
    metrics_out.clear();

    if (!s_booted)
    {
        return false;
    }

    // Marshal slate to Lisp alist
    cl_object slate_alist = marshal_slate(slate);

    // Get the aa-calculate function symbol
    cl_object fn_sym = ecl_make_symbol("AA-CALCULATE", "CL-USER");

    // Redirect *standard-output* to a string stream so (format t ...)
    // output is captured instead of going to stdout.
    cl_object capture_stream = ecl_make_string_output_stream(128, 1);
    cl_object stdout_sym = ecl_make_symbol("*STANDARD-OUTPUT*", "COMMON-LISP");
    ecl_bds_bind(ecl_process_env(), stdout_sym, capture_stream);

    // Call (aa-calculate slate)
    cl_object result = ECL_NIL;
    bool ok = true;
    ECL_HANDLER_CASE_BEGIN(ecl_process_env(), ecl_list1(ECL_T))
    {
        result = cl_funcall(2, fn_sym, slate_alist);
    }
    ECL_HANDLER_CASE(1, condition)
    {
        // Capture condition text before unbinding
        cl_object cond_str = cl_princ_to_string(condition);
        std::string cond_msg = extract_string(cond_str);
        if (!cond_msg.empty())
        {
            Logger::instance().error(std::format("[AI] {}", cond_msg));
        }
        ok = false;
    }
    ECL_HANDLER_CASE_END;

    // Extract captured Lisp output and restore *standard-output*
    cl_object captured = cl_get_output_stream_string(capture_stream);
    ecl_bds_unwind1(ecl_process_env());

    std::string lisp_output = extract_string(captured);
    if (!lisp_output.empty())
    {
        Logger::instance().debug(std::format("[AI] {}", lisp_output));
    }

    if (!ok)
    {
        return false;
    }

    // Unmarshal result to commands (metrics intercepted here)
    return unmarshal_commands(result, commands_out, metrics_out);
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

    // Cross-turn metrics
    alist = make_cons(
        make_cons(make_keyword("metrics"),
                  marshal_metric_list(slate.persisted_metrics)),
        alist);

    // BFS distance matrix (topology-aware)
    alist = make_cons(
        make_cons(make_keyword("distances"),
                  marshal_hex_distance_list(slate.hex_distances)),
        alist);

    // Warpline hexes
    alist = make_cons(
        make_cons(make_keyword("warpline-hexes"),
                  marshal_string_list(slate.warpline_hexes)),
        alist);

    // Hex adjacency (geometric + warpline neighbors)
    alist = make_cons(
        make_cons(make_keyword("adjacency"),
                  marshal_adjacency_list(slate.hex_adjacency)),
        alist);

    // Economic layer
    alist = make_cons(
        make_cons(make_keyword("codex"), marshal_codex_list(slate.codex)),
        alist);
    alist = make_cons(make_cons(make_keyword("resources"),
                                marshal_resource_list(slate.resources)),
                      alist);
    alist = make_cons(make_cons(make_keyword("facilities"),
                                marshal_facility_list(slate.facilities)),
                      alist);
    alist = make_cons(make_cons(make_keyword("market-prices"),
                                marshal_market_list(slate.market_prices)),
                      alist);
    alist = make_cons(make_cons(make_keyword("salvageables"),
                                marshal_salvageable_list(slate.salvageables)),
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

        // Racked systemships (for warpships with H > 0)
        plist = make_cons(marshal_string_list(s.carried_systemships), plist);
        plist = make_cons(make_keyword("racked"), plist);

        // Which warpship this systemship is racked in (empty if not racked)
        plist = make_cons(make_string(s.racked_in), plist);
        plist = make_cons(make_keyword("racked-in"), plist);

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

        plist = make_cons(make_int(s.last_phasic), plist);
        plist = make_cons(make_keyword("last-phasic"), plist);

        plist = make_cons(make_int(s.last_shield), plist);
        plist = make_cons(make_keyword("last-shield"), plist);

        plist = make_cons(make_int(s.last_launcher), plist);
        plist = make_cons(make_keyword("last-launcher"), plist);

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

        plist = make_cons(make_int(s.hangar), plist);
        plist = make_cons(make_keyword("hangar"), plist);

        plist = make_cons(make_int(s.torpedo), plist);
        plist = make_cons(make_keyword("m"), plist);

        plist = make_cons(make_int(s.launcher), plist);
        plist = make_cons(make_keyword("t"), plist);

        plist = make_cons(make_int(s.shield), plist);
        plist = make_cons(make_keyword("s"), plist);

        plist = make_cons(make_int(s.phasic), plist);
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

        // Economic layer: cargo
        plist = make_cons(make_int(s.cargo_ferrous), plist);
        plist = make_cons(make_keyword("cargo-ferrous"), plist);

        plist = make_cons(make_int(s.cargo_rare_earth), plist);
        plist = make_cons(make_keyword("cargo-rare-earth"), plist);

        plist = make_cons(make_int(s.cargo_radioactive), plist);
        plist = make_cons(make_keyword("cargo-radioactive"), plist);

        plist = make_cons(make_int(s.cargo_crystalline), plist);
        plist = make_cons(make_keyword("cargo-crystalline"), plist);

        plist = make_cons(make_int(s.cargo_volatile), plist);
        plist = make_cons(make_keyword("cargo-volatile"), plist);

        plist = make_cons(make_int(s.cargo_water), plist);
        plist = make_cons(make_keyword("cargo-water"), plist);

        plist = make_cons(make_int(s.cargo_organic), plist);
        plist = make_cons(make_keyword("cargo-organic"), plist);

        plist = make_cons(make_int(s.cargo_exotic), plist);
        plist = make_cons(make_keyword("cargo-exotic"), plist);

        plist = make_cons(make_int(s.cargo_torpedoes), plist);
        plist = make_cons(make_keyword("cargo-torpedoes"), plist);

        plist = make_cons(make_int(s.cargo_capacity), plist);
        plist = make_cons(make_keyword("cargo-capacity"), plist);

        plist = make_cons(make_int(s.torpedoes_max), plist);
        plist = make_cons(make_keyword("torpedoes-max"), plist);

        plist = make_cons(make_string(s.at_system), plist);
        plist = make_cons(make_keyword("at-system"), plist);

        // Max values for repair decisions
        plist = make_cons(make_int(s.pd_max), plist);
        plist = make_cons(make_keyword("pd-max"), plist);

        plist = make_cons(make_int(s.phasic_max), plist);
        plist = make_cons(make_keyword("phasic-max"), plist);

        plist = make_cons(make_int(s.shield_max), plist);
        plist = make_cons(make_keyword("shield-max"), plist);

        plist = make_cons(make_int(s.launcher_max), plist);
        plist = make_cons(make_keyword("launcher-max"), plist);

        // Equipment
        plist = make_cons(make_int(s.lrs), plist);
        plist = make_cons(make_keyword("lrs"), plist);

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

cl_object EclBridge::marshal_codex_list(const std::vector<AACodexEntry>& codex)
{
    cl_object list = ECL_NIL;

    for (auto it = codex.rbegin(); it != codex.rend(); ++it)
    {
        const AACodexEntry& ce = *it;

        cl_object plist = ECL_NIL;

        plist = make_cons(make_string(ce.level), plist);
        plist = make_cons(make_keyword("level"), plist);

        plist = make_cons(make_string(ce.system_name), plist);
        plist = make_cons(make_keyword("system"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object
EclBridge::marshal_resource_list(const std::vector<AAResourceInfo>& resources)
{
    cl_object list = ECL_NIL;

    for (auto it = resources.rbegin(); it != resources.rend(); ++it)
    {
        const AAResourceInfo& ri = *it;

        cl_object plist = ECL_NIL;

        plist = make_cons(make_int(ri.yield), plist);
        plist = make_cons(make_keyword("yield"), plist);

        plist = make_cons(make_string(ri.abundance), plist);
        plist = make_cons(make_keyword("abundance"), plist);

        plist = make_cons(make_string(ri.type), plist);
        plist = make_cons(make_keyword("type"), plist);

        plist = make_cons(make_string(ri.system), plist);
        plist = make_cons(make_keyword("system"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object
EclBridge::marshal_facility_list(const std::vector<AAFacilityInfo>& facilities)
{
    cl_object list = ECL_NIL;

    for (auto it = facilities.rbegin(); it != facilities.rend(); ++it)
    {
        const AAFacilityInfo& fi = *it;

        cl_object plist = ECL_NIL;

        if (fi.controller != '\0')
        {
            plist = make_cons(ECL_CODE_CHAR(fi.controller), plist);
        }
        else
        {
            plist = make_cons(ECL_NIL, plist);
        }
        plist = make_cons(make_keyword("controller"), plist);

        plist = make_cons(make_string(fi.type), plist);
        plist = make_cons(make_keyword("type"), plist);

        plist = make_cons(make_string(fi.system), plist);
        plist = make_cons(make_keyword("system"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object
EclBridge::marshal_market_list(const std::vector<AAMarketPrice>& prices)
{
    cl_object list = ECL_NIL;

    for (auto it = prices.rbegin(); it != prices.rend(); ++it)
    {
        const AAMarketPrice& mp = *it;

        cl_object plist = ECL_NIL;

        plist = make_cons(make_int(mp.base_price), plist);
        plist = make_cons(make_keyword("base-price"), plist);

        plist = make_cons(make_int(mp.current_price), plist);
        plist = make_cons(make_keyword("price"), plist);

        plist = make_cons(make_string(mp.resource_type), plist);
        plist = make_cons(make_keyword("type"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object EclBridge::marshal_salvageable_list(
    const std::vector<AASalvageableInfo>& salvageables)
{
    cl_object list = ECL_NIL;

    for (auto it = salvageables.rbegin(); it != salvageables.rend(); ++it)
    {
        const AASalvageableInfo& si = *it;

        cl_object plist = ECL_NIL;

        plist = make_cons(make_int(si.salvage_value), plist);
        plist = make_cons(make_keyword("value"), plist);

        plist = make_cons(make_string(si.state), plist);
        plist = make_cons(make_keyword("state"), plist);

        plist = make_cons(make_string(si.object_type), plist);
        plist = make_cons(make_keyword("object-type"), plist);

        plist = make_cons(make_string(si.hex_id), plist);
        plist = make_cons(make_keyword("hex"), plist);

        list = make_cons(plist, list);
    }

    return list;
}

cl_object EclBridge::marshal_metric_list(const std::vector<AAMetric>& metrics)
{
    // Build alist of (name . value) pairs
    cl_object list = ECL_NIL;

    for (auto it = metrics.rbegin(); it != metrics.rend(); ++it)
    {
        const AAMetric& met = *it;
        cl_object pair = make_cons(make_string(met.name),
                                   ecl_make_double_float(met.value));
        list = make_cons(pair, list);
    }

    return list;
}

cl_object
EclBridge::marshal_hex_distance_list(const std::vector<AAHexDistance>& dists)
{
    // Build alist of ((fromHex . toHex) . cost)
    cl_object list = ECL_NIL;

    for (auto it = dists.rbegin(); it != dists.rend(); ++it)
    {
        const AAHexDistance& hd = *it;
        cl_object key = make_cons(make_string(hd.fromHex),
                                  make_string(hd.toHex));
        cl_object entry = make_cons(key, make_int(hd.cost));
        list = make_cons(entry, list);
    }

    return list;
}

cl_object
EclBridge::marshal_adjacency_list(const std::vector<AAHexAdjacency>& adjacency)
{
    // Build alist of (hex-id . (neighbor1 neighbor2 ...))
    cl_object list = ECL_NIL;

    for (auto it = adjacency.rbegin(); it != adjacency.rend(); ++it)
    {
        const AAHexAdjacency& ha = *it;
        cl_object nbrs = marshal_string_list(ha.neighbors);
        cl_object entry = make_cons(make_string(ha.hexId), nbrs);
        list = make_cons(entry, list);
    }

    return list;
}

// ----------------------------------------------------------------------------
// Unmarshal Lisp -> C++
// ----------------------------------------------------------------------------

bool EclBridge::unmarshal_commands(
    cl_object result,
    std::vector<std::string>& commands_out,
    std::vector<std::pair<std::string, double>>& metrics_out)
{
    // Result should be a list of plists: ((:cmd "X" :args "Y") ...)
    if (KH_EQU(result, ECL_NIL))
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

            // Intercept __metric pseudo-commands
            if (KH_EQU(cmd, "__metric") && !args.empty())
            {
                // Parse "name value" from args
                size_t space_pos = args.find(' ');
                if (space_pos != std::string::npos)
                {
                    std::string mname = args.substr(0, space_pos);
                    double mval = std::atof(args.substr(space_pos + 1).c_str());
                    metrics_out.push_back({mname, mval});
                }
            }
            else
            {
                // Normal command
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
        }

        result = cl_cdr(result);
    }

    return true;
}

std::string EclBridge::extract_string(cl_object obj)
{
    if (KH_EQU(obj, ECL_NIL) || KH_EQU(obj, nullptr))
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
