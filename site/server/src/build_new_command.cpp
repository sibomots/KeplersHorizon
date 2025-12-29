#include "build_new_command.h"

#include <cctype>
#include <sstream>

#include "game.h"
#include "logger.h"
#include "telemetry.h"
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
    if (m_ship_code.empty() || m_ship_code.length() > 10)
    {
        Logger::instance().error("Invalid ship code format");
        Telemetry::write("Error: Invalid ship code format");
        return false;
    }

    // Auto-assign ship number if not provided
    std::string ship_code = m_ship_code;
    if (ship_code.find_first_of("0123456789") == std::string::npos)
    {
        int next_num = 1;
        std::string candidate;
        do
        {
            candidate = ship_code + std::to_string(next_num);
            next_num++;
        } while (ship_exists(m_db, s.game_id, active_player, candidate) ||
                 draft_exists(m_db, s.game_id, active_player, candidate));
        ship_code = candidate;
    }

    // Check for duplicates
    if (draft_exists(m_db, s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Draft already exists: " + ship_code);
        Telemetry::write("Error: Draft already exists: " + ship_code);
        return false;
    }

    if (ship_exists(m_db, s.game_id, active_player, ship_code))
    {
        Logger::instance().error("Ship already exists: " + ship_code);
        Telemetry::write("Error: Ship already exists: " + ship_code);
        return false;
    }

    // Create draft
    DraftRow draft;
    draft.code = ship_code;
    draft.name = m_ship_name;
    draft.attr.type = 'W'; // Default to warship

    insert_draft(m_db, s.game_id, active_player, draft);
    set_current_draft(m_db, s.game_id, active_player, ship_code);

    Logger::instance().info("Draft created: " + m_ship_name + " - " +
                            ship_code);
    Telemetry::write("Draft created: " + m_ship_name + " - " + ship_code +
                     " (current)");
    Telemetry::write("Use: build set PD|B|S|T|M|SR <n>");

    return true;
}
