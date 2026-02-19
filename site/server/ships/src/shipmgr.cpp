///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include "shipmgr.h"

#include "app.h"
#include "ce.h"
#include "db.h"

bool ShipManager::ship_exists(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "SELECT id FROM ships "
                    " WHERE game_id=? AND owner=? AND ship_code=? "
                    " AND destroyed_at IS NULL LIMIT 1";
    auto rows = db.Query(q, {game_id, owner, code});
    return !rows.empty();
}

bool ShipManager::ship_code_taken(int game_id, char owner,
                                  const std::string& code)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "SELECT id FROM ships "
                    " WHERE game_id=? AND owner=? AND ship_code=? "
                    " LIMIT 1";
    auto rows = db.Query(q, {game_id, owner, code});
    return !rows.empty();
}

bool ShipManager::ship_exists_by_code_or_name(int game_id, char owner,
                                              const std::string& code)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "SELECT id FROM ships "
                    " WHERE game_id=? AND destroyed_at IS NULL "
                    " AND owner=? AND ( ship_code=? OR ship_name=? ) LIMIT 1";

    auto rows = db.Query(q, {game_id, owner, code, code});
    return (KH_NEQ(rows.size(), 0));
}

bool ShipManager::load_drafts_by_owner(std::vector<DraftRow>& drafts,
                                       int game_id, char owner)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::instance();
    std::vector<DraftRow> candidate_drafts;

    std::string q =
        "SELECT ship_code,ship_name,ship_type,pd,phasic,shield,launcher,torpedoes,hangar "
        " FROM drafts "
        " WHERE "
        " game_id=? AND owner=? ORDER BY ship_code";

    auto rows = db.Query(q, {game_id, owner});

    for (auto& r : rows)
    {
        DraftRow d;
        d.code = r[0];
        d.name = r[1];
        d.set_type(r[2].empty() ? 'W' : r[2][0]);
        d.set_PD(std::atoi(r[3].c_str()));
        d.set_Phasic(std::atoi(r[4].c_str()));
        d.set_Shield(std::atoi(r[5].c_str()));
        d.set_Launcher(std::atoi(r[6].c_str()));
        d.set_Torpedo(std::atoi(r[7].c_str()));
        d.set_Hanger(std::atoi(r[8].c_str()));
        candidate_drafts.push_back(d);
    }

    if (candidate_drafts.size() > 0)
    {
        drafts = std::move(candidate_drafts);
        result = true;
    }
    return result;
}

bool ShipManager::load_ship_draft_by_spec(DraftRow& row, int did, int game_id,
                                          char owner, const std::string& code)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::instance();

    std::string q =
        "SELECT ship_code,ship_name,ship_type,pd,phasic,shield,launcher,torpedoes,hangar "
        "FROM drafts "
        "WHERE game_id=? "
        " AND id =? "
        " AND owner=? "
        " AND ( ship_code=? OR ship_name=? )"
        " LIMIT 1";

    auto rows = db.Query(q, {game_id, did, owner, code, code});

    if (rows.empty())
    {
        result = false;
    }
    else
    {
        result = true;
        auto& r = rows[0];
        row.code = r[0];
        row.name = r[1];
        // TINYBUG - Don't like how we have to call setters this way...
        row.set_type(r[2].empty() ? 'W' : r[2][0]);
        row.set_PD(std::atoi(r[3].c_str()));
        row.set_Phasic(std::atoi(r[4].c_str()));
        row.set_Shield(std::atoi(r[5].c_str()));
        row.set_Launcher(std::atoi(r[6].c_str()));
        row.set_Torpedo(std::atoi(r[7].c_str()));
        row.set_Hanger(std::atoi(r[8].c_str()));
    }
    return result;
}

void ShipManager::insert_draft(int game_id, char owner, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q =
        "INSERT INTO "
        "drafts(game_id,owner,ship_code,ship_name,ship_type,pd,phasic,"
        "shield,launcher,torpedoes,hangar) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    db.Exec(q, {game_id, owner, d.code, d.name, d.get_type(), d.get_PD(),
                d.get_Phasic(), d.get_Shield(), d.get_Launcher(), d.get_Torpedo(), d.get_Hanger()});
}

void ShipManager::update_draft_attrs(int did, int game_id, char owner,
                                     const std::string& code, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "UPDATE drafts SET pd=?, "
                    "phasic=?, "
                    "shield=?, "
                    "launcher=?, "
                    "torpedoes=?, "
                    "hangar=? "
                    " WHERE game_id=? "
                    " AND ID=? "
                    " AND owner=? AND ( ship_code=? OR ship_name= ?)";
    db.Exec(q, {d.get_PD(), d.get_Phasic(), d.get_Shield(), d.get_Launcher(), d.get_Torpedo(),
                d.get_Hanger(), game_id, did, owner, code, code});
}

void ShipManager::delete_draft(int did, int game_id, char owner,
                               const std::string& code)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q =
        "DELETE FROM drafts WHERE id=? AND game_id=? AND id=? AND owner=? "
        " AND (ship_code=? OR ship_name=?)";
    db.Exec(q, {did, game_id, did, owner, code, code});
}

ShipRow ShipManager::prepare_ship_row_from_row(std::vector<std::string> row)
{
    auto& r = row;
    ShipRow s;
    s.code = r[0];
    s.name = r[1];
    s.set_type(r[2].empty() ? 'W' : r[2][0]);
    s.set_tech(std::atoi(r[3].c_str()));
    s.built_turn = r[4];
    s.set_PD(std::atoi(r[5].c_str()));
    s.set_Phasic(std::atoi(r[6].c_str()));
    s.set_Shield(std::atoi(r[7].c_str()));
    s.set_Launcher(std::atoi(r[8].c_str()));
    s.set_Torpedo(std::atoi(r[9].c_str()));
    s.set_Hanger(std::atoi(r[10].c_str()));
    s.at_system = r[11];
    s.at_hex = r[12];
    s.racked_in = r[13];
    s.pd_spent = std::atoi(r[14].c_str());
    return s;
}

bool ShipManager::load_ship_by_code_or_name(ShipRow& shiprow, int game_id,
                                            char owner, const std::string& code)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::instance();
    std::string q =
        "SELECT "
        "ship_code,ship_name,ship_type,tech_level,built_turn,pd,"
        "phasic,shield,launcher,torpedoes,hangar,at_system,at_hex,racked_in,pd_spent "
        "FROM ships "
        " WHERE game_id=? AND owner=? "
        " AND ( ship_code=? OR ship_name=? ) "
        " AND destroyed_at IS NULL "
        " LIMIT 1";

    auto rows = db.Query(q, {game_id, owner, code, code});

    if (rows.empty())
    {
        result = false;
    }
    else
    {
        // TINYBUG - We assume it's in the first row..?
        shiprow = prepare_ship_row_from_row(rows[0]);
        result = true;
    }
    return result;
}

bool ShipManager::insert_ship(int game_id, char owner, const ShipRow& ship)
{
    ShipRow s(ship);
    DatabaseManager& db = DatabaseManager::instance();

    // Get the attribute values (used for both current and max)
    int pd = s.get_PD();
    int phasic = s.get_Phasic();
    int shield = s.get_Shield();
    int launcher = s.get_Launcher();
    int torpedoes = s.get_Torpedo();
    int hangar = s.get_Hanger();

    std::string q =
        "INSERT INTO "
        "ships(game_id,owner,ship_code,ship_name,ship_type,tech_level,"
        "built_turn,"
        "pd,pd_max,"
        "phasic,phasic_max,"
        "shield,shield_max,"
        "launcher,launcher_max,"
        "torpedoes,torpedoes_max,"
        "hangar,hangar_max,"
        "at_system,at_hex,racked_in,pd_spent) "
        "VALUES ( "
        "  ? ,?, ?, ?, ?, ?, "
        "  ?, "
        "  ?, ?, "
        "  ?, ?, "
        "  ?, ?, "
        "  ?, ?, "
        "  ?, ?, "
        "  ?, ?, "
        "  ?, ?, ?, ? "
        " )";

    bool ok = db.Exec(q, {game_id,      owner,        s.code, s.name,
                          s.get_type(), s.get_tech(), s.built_turn, pd,
                          pd,           phasic,         phasic,         shield,
                          shield,       launcher,         launcher,         torpedoes,
                          torpedoes,     hangar,           hangar,           s.at_system,
                          s.at_hex,     s.racked_in,  0});
    return ok;
}

void ShipManager::update_ship_location(int game_id, char owner,
                                       const std::string& code,
                                       const std::string& at_system,
                                       const std::string& at_hex,
                                       const std::string& racked_in)
{
    DatabaseManager& db = DatabaseManager::instance();
    std::string q = "UPDATE ships SET at_system=?, at_hex=?, racked_in=? "
                    " WHERE game_id=? AND owner=? "
                    " AND ( ship_code=? OR ship_name=? )";
    db.Exec(q, {at_system, at_hex, racked_in, game_id, owner, code, code});
}

bool ShipManager::is_ship_draft_valid(DraftRow& drow,
                                      std::vector<std::string>& report)
{

   static const int E_NEG_SHIP_ATTR  = (1 << 0);
   static const int E_H_ON_SYSTEM_SHIP = (1 << 1);
   static const int E_TORPEDO_BY_THREE = (1 << 2);
   static const int E_TORPEDO_LAUNCHER_MISMATCH = (1 << 3);
    int result = 0;

    // sanity check
    if (drow.value_ranges_invalid())
    {
        report.push_back(LC_NEG_ATTR);
        result |= E_NEG_SHIP_ATTR;
    }

    // system ships cannot have system rack
    if (drow.system_ship_fitment_invalid())
    {
        report.push_back(LC_SS_H);
        result |= E_H_ON_SYSTEM_SHIP;
    }

    // if torpedoes, then they come in multiples of 3
    if (drow.torpedo_count_invalid())
    {
        report.push_back(LC_TORPEDO_BY_THREE);
        result |= E_TORPEDO_BY_THREE;
    }

    // launcher and missle match is wrong
    if (drow.launcher_torpedo_match_invalid())
    {
        report.push_back(LC_TORPEDO_LAUNCHER_MISMATCH);
        result |= E_TORPEDO_LAUNCHER_MISMATCH;
    }
    if (KH_EQU(0, result))
    {
        drow.update_cost();
    }

    return KH_EQU(0, result);
}

void ShipManager::pstxt(std::ostringstream& out, const std::string& s, int w)
{
    out << std::left << std::setw(w) << s;
}

void ShipManager::pstxt_right(std::ostringstream& out, const std::string& s, int w)
{
    out << std::right << std::setw(w) << s;
}

void ShipManager::psnum(std::ostringstream& out, const int& n, int w)
{
    out << std::right << std::setw(w) << std::to_string(n);
}

std::string ShipManager::hangar_na_glyph()
{
    // U+274C CROSS MARK (often rendered as a red X glyph)
    // return u8"❌";
    return std::string(reinterpret_cast<const char*>(u8"❌"));
}

std::string ShipManager::itos(int v)
{
    std::ostringstream tmp;
    tmp << v;
    return tmp.str();
}

// Title helper: left label + right bracket phrase on same line.
// total_width is the line width you want to target for alignment.
// If you don't care, keep it at 60-ish like the sample; tweak as desired.
void ShipManager::put_title(std::ostringstream& out, const std::string& label,
                            const std::string& bracket_text, int total_width)
{
    // Example result:
    // SHIPYARD                [ 3 vessels in production ]
    const std::string rhs = "[ " + bracket_text + " ]";

    // If label is already long, just print with a space.
    if ((int)label.size() + 1 + (int)rhs.size() >= total_width)
    {
        out << label << ' ' << rhs << '\n';
        return;
    }

    out << label;
    const int spaces = total_width - (int)label.size() - (int)rhs.size();
    for (int i = 0; i < spaces; ++i)
        out << ' ';
    out << rhs << '\n';
}

// DRAFT HEADER (Shipyard)
void ShipManager::append_draft_header(std::ostringstream& out,
                                      size_t vessels_in_production)
{

    if (vessels_in_production > 0)
    {
        put_title(out, "SHIPYARD",
                  itos(vessels_in_production) + " vessels in production", 50);
    }
    else
    {
        pstxt(out, "SHIPYARD", 8);
        out << "\n";
    }

    pstxt(out, "HULL", 4);
    out << ' ';
    pstxt(out, "DESIGNATION", 17);
    out << ' ';
    pstxt(out, "TECH", 4);
    out << ' ';
    pstxt(out, "COST", 4);
    out << ' ';
    pstxt(out, "PD", 2);
    out << ' ';
    pstxt_right(out, "P", 2);
    out << ' ';
    pstxt_right(out, "S", 2);
    out << ' ';
    pstxt_right(out, "L", 2);
    out << ' ';
    pstxt_right(out, "T", 2);
    out << ' ';
    pstxt_right(out, "H", 2);
    out << '\n';
    pstxt(out, "────", 4);
    out << ' ';
    pstxt(out, "─────────────────", 17);
    out << ' ';
    pstxt(out, "────", 4);
    out << ' ';
    pstxt(out, "────", 4);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << '\n';
}

// DRAFT ROW (1-line)
void ShipManager::append_draft_row(std::ostringstream& out, const DraftRow& r)
{
    pstxt(out, r.code, 4);
    out << ' ';
    pstxt(out, r.name, 17);
    out << ' ';

    psnum(out, r.get_tech(), 4);
    out << ' ';
    psnum(out, r.get_cost(), 4);
    out << " ";

    psnum(out, r.get_PD(), 2);
    out << ' ';
    psnum(out, r.get_Phasic(), 2);
    out << ' ';
    psnum(out, r.get_Shield(), 2);
    out << ' ';
    psnum(out, r.get_Launcher(), 2);
    out << ' ';
    psnum(out, r.get_Torpedo(), 2);
    out << ' ';
    psnum(out, r.get_Hanger(), 2);

    out << '\n';
}

// FLEET HEADER (Registry)
void ShipManager::append_fleet_header(std::ostringstream& out,
                                      int vessels_operational)
{
    put_title(out, "FLEET REGISTRY",
              itos(vessels_operational) + " vessels operational", 78);

    pstxt(out, "HULL", 5);
    pstxt(out, "DESIGNATION", 17);
    out << ' ';
    pstxt(out, "SECTOR", 10);
    out << ' ';
    pstxt(out, "TECH", 4);
    out << ' ';
    pstxt(out, "PD", 2);
    out << ' ';
    pstxt_right(out, "P", 2);
    out << ' ';
    pstxt_right(out, "S", 2);
    out << ' ';
    pstxt_right(out, "L", 2);
    out << ' ';
    pstxt_right(out, "T", 2);
    out << ' ';
    pstxt_right(out, "H", 2);
    out << ' ';
    pstxt(out, "LS", 2);
    out << '\n';

    pstxt(out, "────", 5);
    pstxt(out, "─────────────────", 17);
    out << ' ';
    pstxt(out, "──────────", 10);
    out << ' ';
    pstxt(out, "────", 4);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << ' ';
    pstxt(out, "──", 2);
    out << '\n';
}

// FLEET ROW (1-line)
void ShipManager::append_fleet_row(std::ostringstream& out, const ShipRow& r)
{
    pstxt(out, r.code, 5);
    pstxt(out, r.name, 17);
    out << ' ';
    pstxt(out, r.get_sector(), 10);
    out << ' ';

    psnum(out, r.get_tech(), 4);
    out << ' ';
    psnum(out, r.get_PD(), 2);
    out << ' ';
    psnum(out, r.get_Phasic(), 2);
    out << ' ';
    psnum(out, r.get_Shield(), 2);
    out << ' ';
    psnum(out, r.get_Launcher(), 2);
    out << ' ';
    psnum(out, r.get_Torpedo(), 2);
    out << ' ';

    const bool is_system_ship =
          ((KH_EQU(r.code[0], 'S')) || (KH_EQU(r.code[0], 's')));

    if (is_system_ship)
    {
        pstxt(out, hangar_na_glyph(), 2);
    }
    else
    {
        psnum(out, r.get_Hanger(), 2);
    }
    out << ' ';

    psnum(out, r.get_LS(), 2);

    out << '\n';
}

// Builds the whole report string. You can add rows by calling append_row.
void ShipManager::build_drafts_report(std::ostringstream& out,
                                      std::vector<DraftRow>& drafts)
{
    append_draft_header(out, drafts.size());
    for (auto& d : drafts)
    {
        d.update_cost();
        append_draft_row(out, d);
    }
}
