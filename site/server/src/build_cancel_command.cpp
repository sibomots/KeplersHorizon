#include "build_cancel_command.h"

#include "game.h"
#include "logger.h"

bool BuildCancelCommand::invoke(void)
{
    GameState s = load_game(m_db, m_game_id);
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::string draft_code = get_current_draft(m_db, m_game_id, active_player);

    if (draft_code.empty())
    {
        Logger::instance().error("No current draft to cancel");
        return false;
    }

    delete_draft(m_db, m_game_id, active_player, draft_code);
    set_current_draft(m_db, m_game_id, active_player, "");
    Logger::instance().info("Canceled draft: " + draft_code);

    return true;
}
