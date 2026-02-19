///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SHIP_MANAGER_H__
#define __KH_SHIP_MANAGER_H__

#include "telemetry.h"
#include "shipdata.h"

class ShipManager
{
  public:
    static ShipManager& instance()
    {
        static ShipManager instance;
        return instance;
    }

    bool load_ship_by_code_or_name(ShipRow& shiprow, int game_id, char owner,
                                   const std::string& code);
    bool load_ship_draft_by_spec(DraftRow& row, int did, int game_id,
                                 char owner, const std::string& code);

    void update_ship_location(int game_id, char owner, const std::string& code,
                              const std::string& at_system,
                              const std::string& at_hex,
                              const std::string& racked_in = "");
    bool ship_exists_by_code_or_name(int game_id, char owner,
                                     const std::string& code);
    bool ship_exists(int game_id, char owner, const std::string& code);
    bool ship_code_taken(int game_id, char owner, const std::string& code);

    void delete_draft(int did, int game_id, char owner,
                      const std::string& code);

    void append_draft_header(std::ostringstream& out,
                             size_t vessels_in_production = 0);
    void append_draft_row(std::ostringstream& out, const DraftRow& r);
    void append_fleet_header(std::ostringstream& out, int vessels_operational);
    void append_fleet_row(std::ostringstream& out, const ShipRow& r);
    void build_drafts_report(std::ostringstream& out,
                             std::vector<DraftRow>& drafts);

    void insert_draft(int game_id, char owner, const DraftRow& d);
    void update_draft_attrs(int did, int game_id, char owner,
                            const std::string& code, const DraftRow& d);
    bool insert_ship(int game_id, char owner, const ShipRow& ship);
    bool is_ship_draft_valid(DraftRow& drow, std::vector<std::string>& report);

    bool load_drafts_by_owner(std::vector<DraftRow>& drafts, int game_id,
                              char owner);

  private:
    ShipManager()
    {
    }

    ShipManager(const ShipManager&) = delete;
    ShipManager& operator=(const ShipManager&) = delete;
    ShipManager(ShipManager&&) = delete;
    ShipManager& operator=(ShipManager&&) = delete;

    ShipRow prepare_ship_row_from_row(std::vector<std::string> row);

    // Formatting utilities
    void pstxt(std::ostringstream& out, const std::string& s, int w);
    void pstxt_right(std::ostringstream& out, const std::string& s, int w);
    void psnum(std::ostringstream& out, const int& n, int w);
    std::string hangar_na_glyph();
    std::string itos(int v);
    void put_title(std::ostringstream& out, const std::string& label,
                   const std::string& bracket_text, int total_width);
};
#endif
