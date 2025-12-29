#ifndef __BUILD_CANCEL_COMMAND_H__
#define __BUILD_CANCEL_COMMAND_H__

#include <memory>

#include "db.h"
#include "icmd.h"

class BuildCancelCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Db *_db = nullptr;
        int _game_id = 0;

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

        std::unique_ptr<ICmd> build()
        {
            return std::unique_ptr<BuildCancelCommand>(
                new BuildCancelCommand(_db, _game_id));
        }
    };

  private:
    BuildCancelCommand(Db *db, int game_id) : m_db(db), m_game_id(game_id)
    {
    }

    Db *m_db;
    int m_game_id;

  public:
    virtual bool invoke(void);
};

#endif
