///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// 
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////
#include "game.h"

#include "app.h"
#include "db.h"

/*
GameState gamestate;
AuthContext authcontext;
DraftRow draftrow;
ShipRow shiprow;
*/

GameState new_game_state_for_scenario(const std::string &scenario)
{
    GameState s;
    s.scenario = scenario;
    s.round = 1;
    s.active_player = "A";
    s.phase_index = PH_BUILD_SHIPS;
    s.vpA = 0;
    s.vpB = 0;

    if (scenario == "learning")
    {
        s.bpA = 40;
        s.bpB = 40;
    }
    else if (scenario == "basic")
    {
        s.bpA = 50;
        s.bpB = 50;
    }
    else if (scenario == "advanced")
    {
        s.bpA = 20;
        s.bpB = 20; // start of first turn
    }
    return s;
}

void apply_start_of_turn(Db *db, GameState &s)
{
    // Called when a player begins their player-turn (phase 0 = Build Ships).
    // 1) Count victory points automatically.
    // 2) In Advanced scenario, award BP (+10) at start of each player-turn after
    //    the very first player-turn.

    if (s.scenario.empty() || s.game_over)
        return;

    // VP: +1 for each enemy base system occupied at start of your turn.
    char me = s.active_player.empty() ? 'A' : s.active_player[0];
    char enemy = (me == 'A') ? 'B' : 'A';

    int vp_gain = 0;
    {
        std::string q =
            "SELECT COUNT(DISTINCT ss.name) "
            "FROM ships sh JOIN star_systems ss ON sh.at_system = ss.name "
            "WHERE sh.game_id=" + std::to_string(s.game_id) +
            " AND sh.owner='" + std::string(1, me) +
            "' AND sh.racked_in IS NULL "
            " AND ss.is_base=1 AND ss.base_owner='" + std::string(1, enemy) + "'";
        auto r = db->query(q);
        if (!r.empty() && !r[0].empty())
            vp_gain = std::atoi(r[0][0].c_str());
    }

    if (vp_gain > 0)
    {
        if (me == 'A')
            s.vpA += vp_gain;
        else
            s.vpB += vp_gain;
    }

    int need = 3;
    if (s.scenario == "learning")
        need = 1;
    else if (s.scenario == "basic")
        need = 2;
    else if (s.scenario == "advanced")
        need = 3;

    int my_vp = (me == 'A') ? s.vpA : s.vpB;
    if (my_vp >= need)
    {
        s.game_over = true;
        s.winner = std::string(1, me);
        return;
    }

    // Advanced scenario BP cadence.
    if (s.scenario == "advanced")
    {
        bool is_first_player_first_turn = (s.round == 1 && me == 'A');
        if (!is_first_player_first_turn)
        {
            if (me == 'A')
                s.bpA += 10;
            else
                s.bpB += 10;
        }
    }
}

void advance_next(Db *db, GameState &s)
{
    if (s.scenario.empty() || s.game_over)
        return;

    if (s.phase_index < PH_END_TURN)
    {
        s.phase_index++;
        return;
    }

    if (s.active_player == "A")
    {
        s.active_player = "B";
    }
    else
    {
        s.active_player = "A";
        s.round++;
    }
    s.phase_index = PH_BUILD_SHIPS;
    apply_start_of_turn(db, s);
}

int next_event_seq(Db *db, int game_id)
{
    auto r = db->query(
        "SELECT COALESCE(MAX(seq),0)+1 FROM game_events WHERE game_id=" +
        std::to_string(game_id));
    if (r.empty())
        return 1;
    return std::atoi(r[0][0].c_str());
}

void save_game(Db *db, const GameState &s)
{
    std::string q = "UPDATE games SET scenario=";
    if (s.scenario.empty())
    {
        q += "NULL";
    }
    else
    {
        q += "'" + db->esc(s.scenario) + "'";
    }

    q += ", state_json='" + db->esc(s.to_json()) +
         "' WHERE id=" + std::to_string(s.game_id);

    db->exec(q);
}

std::string get_current_draft(Db *db, int game_id, char owner)
{
    std::string col = (owner == 'A') ? "current_draft_A" : "current_draft_B";
    auto rows = db->query("SELECT " + col + " FROM games WHERE id=" +
                          std::to_string(game_id) + " LIMIT 1");
    if (rows.empty())
    {
        return "";
    }
    return rows[0][0];
}

void set_current_draft(Db *db, int game_id, char owner,
                       const std::string &code_or_null)
{
    std::string col = (owner == 'A') ? "current_draft_A" : "current_draft_B";
    std::string val =
        code_or_null.empty() ? "NULL" : ("'" + db->esc(code_or_null) + "'");
    db->exec("UPDATE games SET " + col + "=" + val +
             " WHERE id=" + std::to_string(game_id));
}

bool draft_exists(Db *db, int game_id, char owner, const std::string &code)
{
    auto rows = db->query(
        "SELECT id FROM drafts WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db->esc(code) + "' LIMIT 1");
    return !rows.empty();
}

bool ship_exists(Db *db, int game_id, char owner, const std::string &code)
{
    auto rows = db->query(
        "SELECT id FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
        db->esc(code) + "' LIMIT 1");
    return !rows.empty();
}

std::vector<DraftRow> load_drafts(Db *db, int game_id, char owner)
{
    std::vector<DraftRow> out;
    auto rows = db->query(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        "FROM drafts "
        "WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, owner) +
        "' ORDER BY ship_code");

    for (auto &r : rows)
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

DraftRow load_draft(Db *db, int game_id, char owner, const std::string &code)
{
    std::string qry(
        "SELECT ship_code,ship_name,ship_type,pd,beam,screen,tube,missiles,sr "
        "FROM drafts "
        "WHERE game_id=");
    qry.append(std::to_string(game_id));
    qry.append(" AND owner='");
    qry.append(std::string(1, owner));
    qry.append("' AND ship_code='");
    qry.append(db->esc(code));
    qry.append("' LIMIT 1");

    auto rows = db->query(qry.c_str());

    if (rows.empty())
    {
        throw std::runtime_error("draft not found");
    }
    DraftRow d;
    auto &r = rows[0];
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

void insert_draft(Db *db, int game_id, char owner, const DraftRow &d)
{
    std::string q =
        "INSERT INTO "
        "drafts(game_id,owner,ship_code,ship_name,ship_type,pd,beam,"
        "screen,tube,missiles,sr) VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db->esc(d.code) + "','" + db->esc(d.name) + "','" +
        std::string(1, d.attr.type) + "'," + std::to_string(d.attr.PD) + "," +
        std::to_string(d.attr.B) + "," + std::to_string(d.attr.S) + "," +
        std::to_string(d.attr.T) + "," + std::to_string(d.attr.M) + "," +
        std::to_string(d.attr.SR) + ")";
    db->exec(q);
}

void update_draft_attrs(Db *db, int game_id, char owner,
                        const std::string &code, const DraftRow &d)
{
    std::string q = "UPDATE drafts SET pd=" + std::to_string(d.attr.PD) +
                    ",beam=" + std::to_string(d.attr.B) +
                    ",screen=" + std::to_string(d.attr.S) +
                    ",tube=" + std::to_string(d.attr.T) +
                    ",missiles=" + std::to_string(d.attr.M) +
                    ",sr=" + std::to_string(d.attr.SR) +
                    " WHERE game_id=" + std::to_string(game_id) +
                    " AND owner='" + std::string(1, owner) +
                    "' AND ship_code='" + db->esc(code) + "'";
    db->exec(q);
}

void delete_draft(Db *db, int game_id, char owner, const std::string &code)
{
    db->exec("DELETE FROM drafts WHERE game_id=" + std::to_string(game_id) +
             " AND owner='" + std::string(1, owner) + "' AND ship_code='" +
             db->esc(code) + "'");
}

std::vector<ShipRow> load_ships(Db *db, int game_id, char owner)
{
    std::vector<ShipRow> out;
    auto rows =
        db->query("SELECT "
                  "ship_code,ship_name,ship_type,tech_level,built_turn,pd,"
                  "beam,screen,tube,missiles,sr,at_system,racked_in "
                  "FROM ships WHERE game_id=" +
                  std::to_string(game_id) + " AND owner='" +
                  std::string(1, owner) + "' ORDER BY ship_code");
    for (auto &r : rows)
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
        s.racked_in = r[12];
        out.push_back(s);
    }
    return out;
}

ShipRow load_ship(Db *db, int game_id, char owner, const std::string &code)
{
    auto rows = db->query(
        "SELECT "
        "ship_code,ship_name,ship_type,tech_level,built_turn,pd,"
        "beam,screen,tube,missiles,sr,at_system,racked_in "
        "FROM ships WHERE game_id=" +
        std::to_string(game_id) + " AND owner='" + std::string(1, owner) +
        "' AND ship_code='" + db->esc(code) + "' LIMIT 1");
    if (rows.empty())
        throw std::runtime_error("ship not found");
    auto &r = rows[0];
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
    s.racked_in = r[12];
    return s;
}

int count_racked_in(Db *db, int game_id, char owner,
                    const std::string &warpship_code)
{
    auto rows = db->query(
        "SELECT COUNT(*) FROM ships WHERE game_id=" + std::to_string(game_id) +
        " AND owner='" + std::string(1, owner) + "' AND racked_in='" +
        db->esc(warpship_code) + "'");
    if (rows.empty())
        return 0;
    return std::atoi(rows[0][0].c_str());
}

void insert_ship(Db *db, int game_id, char owner, const ShipRow &s)
{
    std::string q =
        "INSERT INTO "
        "ships(game_id,owner,ship_code,ship_name,ship_type,tech_level,built_"
        "turn,"
        "pd,beam,screen,tube,missiles,sr,at_system,racked_in) VALUES(" +
        std::to_string(game_id) + ",'" + std::string(1, owner) + "','" +
        db->esc(s.code) + "','" + db->esc(s.name) + "','" +
        std::string(1, s.attr.type) + "'," + std::to_string(s.attr.tech) +
        ",'" + db->esc(s.built_turn) + "'," + std::to_string(s.attr.PD) + "," +
        std::to_string(s.attr.B) + "," + std::to_string(s.attr.S) + "," +
        std::to_string(s.attr.T) + "," + std::to_string(s.attr.M) + "," +
        std::to_string(s.attr.SR) + "," +
        (s.at_system.empty() ? "NULL" : ("'" + db->esc(s.at_system) + "'")) +
        "," +
        (s.racked_in.empty() ? "NULL" : ("'" + db->esc(s.racked_in) + "'")) +
        ")";
    db->exec(q);
}

void update_ship_location(Db *db, int game_id, char owner,
                          const std::string &code, const std::string &at_system,
                          const std::string &racked_in)
{
    std::string q =
        "UPDATE ships SET at_system=" +
        (at_system.empty() ? "NULL" : ("'" + db->esc(at_system) + "'")) +
        ",racked_in=" +
        (racked_in.empty() ? "NULL" : ("'" + db->esc(racked_in) + "'")) +
        " WHERE game_id=" + std::to_string(game_id) + " AND owner='" +
        std::string(1, owner) + "' AND ship_code='" + db->esc(code) + "'";
    db->exec(q);
}
