```c++
#include "build_list_drafts_command.h"
#include "game.h"
#include "logger.h"
#include "typs.h"
#include <sstream>

bool BuildListDraftsCommand::invoke(void)
{
    GameState s = load_game(m_db, m_game_id);
    char active_player = (s.active_player.empty() ? 'A' : s.active_player[0]);
    std::vector<DraftRow> drafts = load_drafts(m_db, m_game_id, active_player);
    
    if (drafts.empty()) {
        Logger::instance().info("No drafts found");
    } else {
        std::ostringstream msg;
        msg << "Drafts (" << drafts.size() << "):";
        for (const auto& d : drafts) {
            msg << "\n  " << d.code << " - " << d.name;
        }
        Logger::instance().info(msg.str());
    }
    
    return true;
}

```
