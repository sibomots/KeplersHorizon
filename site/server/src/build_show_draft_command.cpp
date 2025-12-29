#include "build_show_draft_command.h"

#include <sstream>

#include "game.h"
#include "logger.h"
#include "typs.h"

bool BuildShowDraftCommand::invoke(void)
{
    GameState s = load_game(m_db, m_game_id);
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);

    if (!draft_exists(m_db, m_game_id, active_player, m_draft_code))
    {
        Logger::instance().error("Draft not found: " + m_draft_code);
        return false;
    }

    DraftRow d = load_draft(m_db, m_game_id, active_player, m_draft_code);
    std::ostringstream msg;
    msg << "Draft: " << d.name << " - " << d.code << "\n"
        << "  Type: " << d.attr.type << "\n"
        << "  PD=" << d.attr.PD << ", B=" << d.attr.B << ", S=" << d.attr.S
        << ", T=" << d.attr.T << ", M=" << d.attr.M << ", SR=" << d.attr.SR;
    Logger::instance().info(msg.str());

    set_current_draft(m_db, m_game_id, active_player, m_draft_code);

    return true;
}
