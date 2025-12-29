#ifndef __START_COMMAND_H__
#define __START_COMMAND_H__

#include "db.h"
#include "icmd.h"
#include "typedefs.h"

class StartCommand : public ICmd
{
  private:
    Db *m_db;
    int m_game_id;
    ScenarioType m_scenario;

  public:
    class Builder
    {
      public:
        Db *_db = nullptr;
        int _game_id = 0;
        ScenarioType _scenario = ScenarioType::UNDEFINED;

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
        Builder &set_scenario(ScenarioType typ)
        {
            _scenario = std::move(typ);
            return *this;
        }

        ICmd *build()
        {
            return new StartCommand(*this);
        }
    };

  private:
    StartCommand(Builder &builder)
        : m_db(builder._db), m_game_id(builder._game_id),
          m_scenario(std::move(builder._scenario))
    {
    }

  public:
    virtual bool invoke(void);
};

#endif
