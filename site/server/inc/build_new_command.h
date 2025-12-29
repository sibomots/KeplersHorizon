#ifndef __BUILD_NEW_COMMAND_H__
#define __BUILD_NEW_COMMAND_H__

#include <memory>
#include <string>

#include "db.h"
#include "icmd.h"

class BuildNewCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Db *_db = nullptr;
        int _game_id = 0;
        std::string _ship_code;
        std::string _ship_name;

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
        Builder &set_ship_code(const std::string &code)
        {
            _ship_code = code;
            return *this;
        }
        Builder &set_ship_name(const std::string &name)
        {
            _ship_name = name;
            return *this;
        }

        std::unique_ptr<ICmd> build()
        {
            return std::unique_ptr<BuildNewCommand>(
                new BuildNewCommand(_db, _game_id, _ship_code, _ship_name));
        }
    };

  private:
    BuildNewCommand(Db *db, int game_id, const std::string &ship_code,
                    const std::string &ship_name)
        : m_db(db), m_game_id(game_id), m_ship_code(ship_code),
          m_ship_name(ship_name)
    {
    }

    Db *m_db;
    int m_game_id;
    std::string m_ship_code;
    std::string m_ship_name;

  public:
    virtual bool invoke(void);
};

#endif
