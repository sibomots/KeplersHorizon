///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_ECL_BRIDGE_H__
#define __KH_ECL_BRIDGE_H__

#include <ecl/ecl.h>
#include <string>
#include <utility>
#include <vector>
#include "typedefs.h"
#include "autonomy_agency.h"

// ----------------------------------------------------------------------------
// EclBridge: C++ <-> ECL marshaling
// ----------------------------------------------------------------------------

class EclBridge
{
  public:
    // Lifecycle
    static bool boot();
    static void shutdown();
    static bool is_booted();

    // Load Lisp files
    static bool load_file(const std::string& path);

    // Marshal AASlate to Lisp alist, call aa-calculate, unmarshal result
    // Metrics with "__metric" prefix are intercepted and placed in metrics_out
    static bool calculate(const AASlate& slate,
                          std::vector<std::string>& commands_out,
                          std::vector<std::pair<std::string, double>>& metrics_out);

  private:
    // Marshal C++ -> Lisp
    static cl_object marshal_slate(const AASlate& slate);
    static cl_object
    marshal_ship_list(const std::vector<struct AAShipInfo>& ships);
    static cl_object
    marshal_string_list(const std::vector<std::string>& strings);
    static cl_object
    marshal_combat_list(const std::vector<struct AACombatHex>& combats);
    static cl_object
    marshal_codex_list(const std::vector<struct AACodexEntry>& codex);
    static cl_object
    marshal_resource_list(const std::vector<struct AAResourceInfo>& resources);
    static cl_object
    marshal_facility_list(const std::vector<struct AAFacilityInfo>& facilities);
    static cl_object
    marshal_market_list(const std::vector<struct AAMarketPrice>& prices);
    static cl_object marshal_salvageable_list(
        const std::vector<struct AASalvageableInfo>& salvageables);
    static cl_object
    marshal_metric_list(const std::vector<struct AAMetric>& metrics);
    static cl_object
    marshal_hex_distance_list(const std::vector<struct AAHexDistance>& dists);
    static cl_object
    marshal_adjacency_list(const std::vector<struct AAHexAdjacency>& adjacency);

    // Unmarshal Lisp -> C++
    static bool unmarshal_commands(
        cl_object result,
        std::vector<std::string>& commands_out,
        std::vector<std::pair<std::string, double>>& metrics_out);
    static std::string extract_string(cl_object obj);

    // Helpers
    static cl_object make_keyword(const char* name);
    static cl_object make_cons(cl_object car, cl_object cdr);
    static cl_object make_string(const std::string& s);
    static cl_object make_int(int val);
    static cl_object make_bool(bool val);

    static bool s_booted;
};

#endif
