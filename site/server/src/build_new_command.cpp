#include "build_new_command.h"

#include <cctype>
#include <sstream>

#include "game.h"
#include "logger.h"
#include "typs.h"

bool BuildNewCommand::invoke(void)
{
    GameState s = load_game(m_db, m_game_id);
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    // Check BP availability
    int &bp = (s.active_player == "A") ? s.bpA : s.bpB;
    if (bp <= 0)
    {
        Logger::instance().error("No Build Points available");
        return false;
    }

    // Validate ship code format
    if (m_ship_code.empty())
    {
        Logger::instance().error("Bad ship code");
        return false;
    }

    char stype = std::toupper(m_ship_code[0]);
    if (stype != 'W' && stype != 'S')
    {
        Logger::instance().error("Ship type must start with W or S");
        return false;
    }

    // Parse or auto-assign ship number
    auto parse_num = [](const std::string &s) -> int {
        if (s.size() <= 1)
            return -1;
        int n = 0;
        for (size_t i = 1; i < s.size(); ++i)
        {
            if (!std::isdigit(s[i]))
                return -1;
            n = n * 10 + (s[i] - '0');
        }
        return n;
    };

    int n = parse_num(m_ship_code);
    if (n == -1)
    {
        // Auto-assign next available number
        int maxn = 0;
        auto rows = m_db->query("SELECT ship_code FROM ships WHERE game_id=" +
                                std::to_string(m_game_id) +
                                " AND ship_type='" + std::string(1, stype) +
                                "'");
        for (auto &r : rows)
        {
            int nn = parse_num(r[0]);
            if (nn > maxn)
                maxn = nn;
        }
        auto rows2 =
            m_db->query("SELECT ship_code FROM drafts WHERE game_id=" +
                        std::to_string(m_game_id) + " AND ship_type='" +
                        std::string(1, stype) + "'");
        for (auto &r : rows2)
        {
            int nn = parse_num(r[0]);
            if (nn > maxn)
                maxn = nn;
        }
        n = maxn + 1;
    }

    if (n <= 0 || n >= 100)
    {
        Logger::instance().error("Ship id must be 1..99");
        return false;
    }

    // Build final code
    std::ostringstream c;
    c << stype << n;
    std::string code = c.str();

    // Check for duplicates
    auto r_check = m_db->query("SELECT 1 FROM ships WHERE game_id=" +
                               std::to_string(m_game_id) +
                               " AND ship_code='" + m_db->esc(code) + "'");
    auto d_check = m_db->query("SELECT 1 FROM drafts WHERE game_id=" +
                               std::to_string(m_game_id) +
                               " AND ship_code='" + m_db->esc(code) + "'");

    if (!r_check.empty() || !d_check.empty())
    {
        Logger::instance().error("Ship code already in use: " + code);
        return false;
    }

    // Prepare name
    std::string name = m_ship_name;
    if (name.empty())
        name = (stype == 'W' ? "WarpShip" : "SystemShip");
    if (name.size() > 32)
        name.resize(32);

    // Create draft
    DraftRow d;
    d.code = code;
    d.name = name;
    d.attr.type = stype;
    insert_draft(m_db, m_game_id, active_player, d);
    set_current_draft(m_db, m_game_id, active_player, code);

    std::ostringstream msg;
    msg << "Draft created: " << name << " - " << code
        << " (current). Use: build set PD|B|S|T|M|SR <n>";
    Logger::instance().info(msg.str());

    return true;
}
