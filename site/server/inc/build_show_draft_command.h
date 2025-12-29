#ifndef __BUILD_SHOW_DRAFT_COMMAND_H__
#define __BUILD_SHOW_DRAFT_COMMAND_H__

#include <memory>
#include <string>

#include "db.h"
#include "icmd.h"

class BuildShowDraftCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Db *_db = nullptr;
        int _game_id = 0;
        std::string _draft_code;

        Builder &set_db(Db *db)
        {
            _db = db;
            return *this;
        }
        Builder &set_game_id(int id)
        {
            _game_id = id;
            return *this;
        }
        Builder &set_draft_code(const std::string &code)
        {
            _draft_code = code;
            return *this;
        }

        std::unique_ptr<ICmd> build()
        {
            return std::unique_ptr<BuildShowDraftCommand>(
                new BuildShowDraftCommand(_db, _game_id, _draft_code));
        }
    };

  private:
    BuildShowDraftCommand(Db *db, int game_id, const std::string &draft_code)
        : m_db(db), m_game_id(game_id), m_draft_code(draft_code)
    {
    }

    Db *m_db;
    int m_game_id;
    std::string m_draft_code;

  public:
    virtual bool invoke(void);
};

#endif
