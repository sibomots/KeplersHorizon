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

/* CP */ std::string get_current_draft(int game_id, char owner)
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

/* CP */ void set_current_draft(int game_id, char owner,
                                const std::string& code_or_null)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string col = (owner == 'A') ? "current_draft_A" : "current_draft_B";
    std::string val =
        code_or_null.empty() ? "NULL" : ("'" + db.esc(code_or_null) + "'");
    db.exec("UPDATE games SET " + col + "=" + val +
            " WHERE id=" + std::to_string(game_id));
}

/* CP */ bool draft_exists(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT id FROM drafts WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db.esc(code) + "' LIMIT 1");
    return !rows.empty();
}

/* CP */ bool ship_exists(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    auto rows = db.query(
        "SELECT id FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db.esc(code) + "' AND destroyed_at IS NULL LIMIT 1");
    return !rows.empty();
}

/* CP */ std::vector<DraftRow> load_drafts(int game_id, char owner)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<DraftRow> out;
    auto rows = db.query(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        "FROM drafts "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, owner) +
        "' ORDER BY ship_code");

    for (auto& r : rows)
    {
        DraftRow d;
        d.code = r[0];
        d.name = r[1];
        d.attr.type = r[2].empty() ? 'W' : r[2][0];
        d.attr.PD = std::atoi(r[3].c_str());
        d.attr.B = std::atoi(r[4].c_str());
        d.attr.S = std::atoi(r[5].c_str());
        d.attr.T = std::atoi(r[6].c_str());
        d.attr.M = std::atoi(r[7].c_str());
        d.attr.SR = std::atoi(r[8].c_str());
        out.push_back(d);
    }
    return out;
}

/* CP */ DraftRow load_draft(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string qry(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        "FROM drafts "
        "WHERE game_id=");
    qry.append(std::to_string(game_id));
    qry.append(" AND owner='");
    qry.append(std::string(1, owner));
    qry.append("' AND ship_code='");
    qry.append(db.esc(code));
    qry.append("' LIMIT 1");

    auto rows = db.query(qry.c_str());

    if (rows.empty())
    {
        throw std::runtime_error("draft not found");
    }
    DraftRow d;
    auto& r = rows[0];
    d.code = r[0];
    d.name = r[1];
    d.attr.type = r[2].empty() ? 'W' : r[2][0];
    d.attr.PD = std::atoi(r[3].c_str());
    d.attr.B = std::atoi(r[4].c_str());
    d.attr.S = std::atoi(r[5].c_str());
    d.attr.T = std::atoi(r[6].c_str());
    d.attr.M = std::atoi(r[7].c_str());
    d.attr.SR = std::atoi(r[8].c_str());
    return d;
}

/* CP */ void insert_draft(int game_id, char owner, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q =
        "INSERT INTO "
        "drafts(game_id,owner,ship_code,ship_name,ship_type,pd,beam,"
        "screen,tube,missiles,sr) VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db.esc(d.code) + "','" + db.esc(d.name) + "','" +
        std::string(1, d.attr.type) + "'," + std::to_string(d.attr.PD) + "," +
        std::to_string(d.attr.B) + "," + std::to_string(d.attr.S) + "," +
        std::to_string(d.attr.T) + "," + std::to_string(d.attr.M) + "," +
        std::to_string(d.attr.SR) + ")";
    db.exec(q);
}

/* CP */ void update_draft_attrs(int game_id, char owner,
                                 const std::string& code, const DraftRow& d)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q = "UPDATE drafts SET pd=" + std::to_string(d.attr.PD) +
                    ",beam=" + std::to_string(d.attr.B) +
                    ",screen=" + std::to_string(d.attr.S) +
                    ",tube=" + std::to_string(d.attr.T) +
                    ",missiles=" + std::to_string(d.attr.M) +
                    ",sr=" + std::to_string(d.attr.SR) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND owner='" + std::string(1, owner) +
                    "' AND ship_code='" + db.esc(code) + "'";
    db.exec(q);
}

/* CP */ void delete_draft(int game_id, char owner, const std::string& code)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    db.exec("DELETE FROM drafts WHERE game_id=" + std::to_string(game_id) +
            " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
            db.esc(code) + "'");
}

/* CP */ std::vector<ShipRow> load_ships(int game_id, char owner)
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
        s.attr.type = r[2].empty() ? 'W' : r[2][0];
        s.attr.tech = std::atoi(r[3].c_str());
        s.built_turn = r[4];
        s.attr.PD = std::atoi(r[5].c_str());
        s.attr.B = std::atoi(r[6].c_str());
        s.attr.S = std::atoi(r[7].c_str());
        s.attr.T = std::atoi(r[8].c_str());
        s.attr.M = std::atoi(r[9].c_str());
        s.attr.SR = std::atoi(r[10].c_str());
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
    s.attr.type = r[2].empty() ? 'W' : r[2][0];
    s.attr.tech = std::atoi(r[3].c_str());
    s.built_turn = r[4];
    s.attr.PD = std::atoi(r[5].c_str());
    s.attr.B = std::atoi(r[6].c_str());
    s.attr.S = std::atoi(r[7].c_str());
    s.attr.T = std::atoi(r[8].c_str());
    s.attr.M = std::atoi(r[9].c_str());
    s.attr.SR = std::atoi(r[10].c_str());
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

/* CP */ void insert_ship(int game_id, char owner, const ShipRow& s)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    std::string q =
        "INSERT INTO "
        "ships(game_id,owner,ship_code,ship_name,ship_type,tech_level,built_"
        "turn,"
        "pd,beam,screen,tube,missiles,sr,at_system,at_hex,racked_in,pd_spent) "
        "VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db.esc(s.code) + "','" + db.esc(s.name) + "','" +
        std::string(1, s.attr.type) + "'," + std::to_string(s.attr.tech) +
        ",'" + db.esc(s.built_turn) + "'," + std::to_string(s.attr.PD) + "," +
        std::to_string(s.attr.B) + "," + std::to_string(s.attr.S) + "," +
        std::to_string(s.attr.T) + "," + std::to_string(s.attr.M) + "," +
        std::to_string(s.attr.SR) + "," +
        (s.at_system.empty() ? "NULL" : ("'" + db.esc(s.at_system) + "'")) +
        "," + (s.at_hex.empty() ? "NULL" : ("'" + db.esc(s.at_hex) + "'")) +
        "," +
        (s.racked_in.empty() ? "NULL" : ("'" + db.esc(s.racked_in) + "'")) +
        ",0)"; // pd_spent=0 on insert
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
