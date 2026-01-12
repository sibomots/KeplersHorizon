//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "ships.h"

#include "app.h"
#include "combat.h"
#include "db.h"

std::string get_current_draft(int game_id, char owner)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string col = (owner == 'A') ? "current_draft_A" : "current_draft_B";
    auto rows = db.query("SELECT " + col + " FROM games WHERE id=" +
                         std::to_string(game_id) + " LIMIT 1");
    if (rows.empty())
    {
        return "";
    }
    return rows[0][0];
}

bool get_draft_by_spec(int& did, int gid, char owner, std::string target)
{
  // jdw
  bool result = false;
  DatabaseManager& db = DatabaseManager::getInstance();
  std::string qry =
            "SELECT DISTINCT "
            "d.id, d.game_id, d.owner, d.ship_code, d.ship_name, "
            "d.ship_type "
            " FROM drafts d "
            " WHERE d.owner = '" +
             std::string(1, owner) +
            "'  AND d.game_id = " +
             std::to_string(gid) +
            " AND ( d.ship_code = '" +
            db.esc(target) +
            "' OR d.ship_name = '" +
            db.esc(target) +
            "')";
  auto rows = db.query(qry.c_str());

  fprintf(stderr, "getting draft by spec:\n%s\n", qry.c_str());

  size_t sz = rows.size(); 
  if (sz == 0) 
  {
         // we cannot find the draft. 
  }
  else 
  {
         // we found the draft(s) 
         // BUGBUG how many??
         std::vector<std::string> r = rows[0];
         did = std::atoi(r[0].c_str());
         result = true;  
  }
  return result; 
}

void set_current_draft(int game_id, char owner,
                                const std::string& code_or_null)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string col = (owner == 'A') ? "current_draft_A" : "current_draft_B";
    std::string val =
        code_or_null.empty() ? "NULL" : ("'" + db.esc(code_or_null) + "'");
    db.exec("UPDATE games SET " + col + "=" + val +
            " WHERE id=" + std::to_string(game_id));
}

bool draft_exists(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT id FROM drafts WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db.esc(code) + "' LIMIT 1");
    return !rows.empty();
}

bool ship_exists(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT id FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db.esc(code) + "' AND destroyed_at IS NULL LIMIT 1");
    return !rows.empty();
}

std::vector<DraftRow> load_drafts_by_owner(int game_id, char owner)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<DraftRow> drafts;
    auto rows = db.query(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        " FROM drafts "
        " WHERE "
        " game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' "  +
        " ORDER BY ship_code");

    for (auto& r : rows)
    {
        DraftRow d;
        d.code = r[0];
        d.name = r[1];
        d.set_type(r[2].empty() ? 'W' : r[2][0]);
        d.set_PD(std::atoi(r[3].c_str()));
        d.set_B(std::atoi(r[4].c_str()));
        d.set_S(std::atoi(r[5].c_str()));
        d.set_T(std::atoi(r[6].c_str()));
        d.set_M(std::atoi(r[7].c_str()));
        d.set_SR(std::atoi(r[8].c_str()));
        drafts.push_back(d);
    }
    return drafts;
}

bool load_ship_draft_by_spec(DraftRow& row, int did, int game_id, char owner, const std::string& code)
{
    bool result = false;
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string qry(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        "FROM drafts "
        "WHERE game_id=");
    qry.append(std::to_string(game_id));
    qry.append(" AND id =");
    qry.append(std::to_string(did));
    qry.append(" AND owner='");
    qry.append(std::string(1, owner));
    qry.append("' AND ( ship_code='");
    qry.append(db.esc(code));
    qry.append("' OR ship_name ='");
    qry.append(db.esc(code));
    qry.append("') LIMIT 1");

    auto rows = db.query(qry.c_str());

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
        // BUGBUG - we cannot let this persist!
        row.set_type(r[2].empty() ? 'W' : r[2][0]);
        row.set_PD(std::atoi(r[3].c_str()));
        row.set_B(std::atoi(r[4].c_str()));
        row.set_S(std::atoi(r[5].c_str()));
        row.set_T(std::atoi(r[6].c_str()));
        row.set_M(std::atoi(r[7].c_str()));
        row.set_SR(std::atoi(r[8].c_str()));
    }
    return result;
}

void insert_draft(int game_id, char owner, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q =
        "INSERT INTO "
        "drafts(game_id,owner,ship_code,ship_name,ship_type,pd,beam,"
        "screen,tube,missiles,sr) VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db.esc(d.code) + "','" + db.esc(d.name) + "','" +
        std::string(1, d.get_type()) + "'," + std::to_string(d.get_PD()) + "," +
        std::to_string(d.get_B()) + "," + std::to_string(d.get_S()) + "," +
        std::to_string(d.get_T()) + "," + std::to_string(d.get_M()) + "," +
        std::to_string(d.get_SR()) + ")";
    fprintf(stderr, "Insert Draft:\n%s\n",
       q.c_str());
    db.exec(q);
}

void update_draft_attrs(int did, int game_id, char owner,
                                 const std::string& code, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q = "UPDATE drafts SET pd=" + std::to_string(d.get_PD()) +
                    ",beam=" + std::to_string(d.get_B()) +
                    ",screen=" + std::to_string(d.get_S()) +
                    ",tube=" + std::to_string(d.get_T()) +
                    ",missiles=" + std::to_string(d.get_M()) +
                    ",sr=" + std::to_string(d.get_SR()) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND ID="+std::to_string(did) +
                    " AND owner='" + std::string(1, owner) +
                    "' AND ( ship_code='" + db.esc(code) + "' OR " +
                    " ship_name='" + db.esc(code) + "')";
    db.exec(q);
}

void delete_draft(int did, int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    db.exec("DELETE FROM drafts "
            " WHERE id ="+ std::to_string(did) +
            " AND game_id=" + std::to_string(game_id) +
            " AND id=" + std::to_string(did) +
            " AND owner='" + std::string(1, owner) + "' "
            " AND ( ship_code='" + db.esc(code) + "' " 
            "       OR ship_name='" + db.esc(code) + "')");
}

std::vector<ShipRow> load_ships(int game_id, char owner)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<ShipRow> out;
    auto rows = db.query(
        "SELECT "
        "ship_code,ship_name,ship_type,tech_level,built_turn,pd,"
        "beam,screen,tube,missiles,sr,at_system,at_hex,racked_in,pd_spent "
        "FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, owner) +
        "' AND destroyed_at IS NULL ORDER BY ship_code");
    for (auto& r : rows)
    {
        ShipRow s;
        s.code = r[0];
        s.name = r[1];
        s.set_type(r[2].empty() ? 'W' : r[2][0]);
        s.set_tech(std::atoi(r[3].c_str()));
        s.built_turn = r[4];
        s.set_PD(std::atoi(r[5].c_str()));
        s.set_B(std::atoi(r[6].c_str()));
        s.set_S(std::atoi(r[7].c_str()));
        s.set_T(std::atoi(r[8].c_str()));
        s.set_M(std::atoi(r[9].c_str()));
        s.set_SR(std::atoi(r[10].c_str()));
        s.at_system = r[11];
        s.at_hex = r[12];
        s.racked_in = r[13];
        s.pd_spent = std::atoi(r[14].c_str());
        out.push_back(s);
    }
    return out;
}

/* CP */ ShipRow load_ship(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT "
        "ship_code,ship_name,ship_type,tech_level,built_turn,pd,"
        "beam,screen,tube,missiles,sr,at_system,at_hex,racked_in,pd_spent "
        "FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, owner) +
        "' AND ship_code='" + db.esc(code) +
        "' AND destroyed_at IS NULL LIMIT 1");
    if (rows.empty())
        throw std::runtime_error("ship not found");
    auto& r = rows[0];
    ShipRow s;
    s.code = r[0];
    s.name = r[1];
    s.set_type(r[2].empty() ? 'W' : r[2][0]);
    s.set_tech(std::atoi(r[3].c_str()));
    s.built_turn = r[4];
    s.set_PD(std::atoi(r[5].c_str()));
    s.set_B(std::atoi(r[6].c_str()));
    s.set_S(std::atoi(r[7].c_str()));
    s.set_T(std::atoi(r[8].c_str()));
    s.set_M(std::atoi(r[9].c_str()));
    s.set_SR(std::atoi(r[10].c_str()));
    s.at_system = r[11];
    s.at_hex = r[12];
    s.racked_in = r[13];
    s.pd_spent = std::atoi(r[14].c_str());
    return s;
}

/* CP */ int count_racked_in(int game_id, char owner,
                             const std::string& warpship_code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT COUNT(*) FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND racked_in='" +
        db.esc(warpship_code) + "' AND destroyed_at IS NULL");
    if (rows.empty())
        return 0;
    return std::atoi(rows[0][0].c_str());
}

void insert_ship(int game_id, char owner, const ShipRow& ship)
{
    ShipRow s(ship);
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q =
        "INSERT INTO "
        "ships(game_id,owner,ship_code,ship_name,ship_type,tech_level,"
        "built_turn,pd,beam,screen,tube,missiles,sr,at_system,at_hex,racked_in,"
        "pd_spent) "
        "VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db.esc(s.code) + "','" + db.esc(s.name) + "','" +
        std::string(1, s.get_type()) + "'," + std::to_string(s.get_tech()) +
        ",'" + db.esc(s.built_turn) + "'," + std::to_string(s.get_PD()) + "," +
        std::to_string(s.get_B()) + "," + std::to_string(s.get_S()) + "," +
        std::to_string(s.get_T()) + "," + std::to_string(s.get_M()) + "," +
        std::to_string(s.get_SR()) + "," +
        (s.at_system.empty() ? "NULL" : ("'" + db.esc(s.at_system) + "'")) +
        "," + (s.at_hex.empty() ? "NULL" : ("'" + db.esc(s.at_hex) + "'")) +
        "," +
        (s.racked_in.empty() ? "NULL" : ("'" + db.esc(s.racked_in) + "'")) +
        ", 0)";
    db.exec(q);
}

/* CP */ void update_ship_location(int game_id, char owner,
                                   const std::string& code,
                                   const std::string& at_system,
                                   const std::string& at_hex,
                                   const std::string& racked_in)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q =
        "UPDATE ships SET at_system=" +
        (at_system.empty() ? "NULL" : ("'" + db.esc(at_system) + "'")) +
        ",at_hex=" + (at_hex.empty() ? "NULL" : ("'" + db.esc(at_hex) + "'")) +
        ",racked_in=" +
        (racked_in.empty() ? "NULL" : ("'" + db.esc(racked_in) + "'")) +
        " WHERE game_id=" + std::to_string(game_id) + " AND owner='" +
        std::string(1, owner) + "' AND ship_code='" + db.esc(code) + "'";
    db.exec(q);
}

bool test_ship_draft_candidate(DraftRow& drow , std::vector<std::string>& report)
{
    int result = 0;

    // sanity check
    if (drow.value_ranges_invalid())
    {
        report.push_back(LOC_STR_NEG_ATTR);
        result != E_NEG_SHIP_ATTR;
    }

    // system ships cannot have system rack
    if (drow.system_ship_fitment_invalid())
    {
        report.push_back(LOC_STR_SS_SR);
        result |= E_SR_ON_SYSTEM_SHIP;
    }

    // if missiles, then they come in multiples of 3
    if (drow.missile_count_invalid())
    {
        report.push_back(LOC_STR_MISSILE_BY_THREE);
        result |= E_MISSILE_BY_THREE;
    }

    // tube and missle match is wrong
    if (drow.tube_missile_match_invalid()) 
    {
        report.push_back(LOC_STR_MISSILE_TUBE_MISMATCH);
        result |= E_MISSILE_TUBE_MISMATCH;
    }
    if (KH_SUCCEEDED(result))
    {
        drow.update_cost();
    }   

    return KH_SUCCEEDED(result);
}
