//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_ECL_BRIDGE_H__
#define __KH_ECL_BRIDGE_H__

#include <ecl/ecl.h>
#include <string>
#include <vector>

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
    static bool calculate(const AASlate& slate,
                          std::vector<std::string>& commands_out);

  private:
    // Marshal C++ -> Lisp
    static cl_object marshal_slate(const AASlate& slate);
    static cl_object
    marshal_ship_list(const std::vector<struct AAShipInfo>& ships);
    static cl_object
    marshal_string_list(const std::vector<std::string>& strings);
    static cl_object
    marshal_combat_list(const std::vector<struct AACombatHex>& combats);

    // Unmarshal Lisp -> C++
    static bool unmarshal_commands(cl_object result,
                                   std::vector<std::string>& commands_out);
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
