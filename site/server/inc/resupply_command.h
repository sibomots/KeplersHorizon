#ifndef __RESUPPLY_COMMAND_H__
#define __RESUPPLY_COMMAND_H__

#include <memory>
#include <string>

#include "icmd.h"
#include "typedefs.h"

class ResupplyCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        std::string _ship_code;
        int _missiles = 0;

        Builder &set_ship_code(const std::string &code)
        {
            _ship_code = code;
            return *this;
        }

        Builder &set_missiles(int m)
        {
            _missiles = m;
            return *this;
        }

        std::unique_ptr<ICmd> build()
        {
            return std::unique_ptr<ResupplyCommand>(
                new ResupplyCommand(_ship_code, _missiles));
        }
    };

  private:
    ResupplyCommand(const std::string &ship_code, int missiles)
        : m_ship_code(ship_code), m_missiles(missiles)
    {
    }

    std::string m_ship_code;
    int m_missiles;

  public:
    virtual bool invoke(void);
};

#endif
