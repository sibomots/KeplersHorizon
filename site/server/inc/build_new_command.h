#ifndef __BUILD_NEW_COMMAND_H__
#define __BUILD_NEW_COMMAND_H__

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

        ICmd *build()
        {
            return new BuildNewCommand(*this);
        }
    };

  private:
    BuildNewCommand(Builder &builder)
        : m_db(builder._db), m_game_id(builder._game_id),
          m_ship_code(std::move(builder._ship_code)),
          m_ship_name(std::move(builder._ship_name))
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
