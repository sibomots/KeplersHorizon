//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MOVE_COMMAND_H__
#define __MOVE_COMMAND_H__

#include <string>
#include <vector>

#include "db.h"
#include "icmd.h"

class MoveCommand : public ICmd
{
  private:
    std::string ship_code;
    std::vector<std::string> destinations;

  public:
    class Builder
    {
      public:
        std::string _ship_code;
        std::vector<std::string> _destinations;

        Builder() {}
        Builder& ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }
        Builder& add_destination(const std::string& dest)
        {
            _destinations.push_back(dest);
            return *this;
        }
        Builder& add_waypoints(std::vector<std::string>* waypoints)
        {
            if (waypoints != NULL && waypoints->size() > 0) 
            {
                // Add chained waypoints
                for (std::vector<std::string>::const_iterator 
                        itr = waypoints->begin();
                        itr != waypoints->end();
                        ++itr)
                {
                    _destinations.push_back(*itr);
                }
            }
            return *this;
        }

        ICmd* build()
        {
            return new MoveCommand(*this);
        }
    };

    bool invoke(void) override;

  private:
    MoveCommand(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_destinations(std::move(builder._destinations))
    {
    }

    std::string m_ship_code;
    std::vector<std::string> m_destinations;
};

#endif
