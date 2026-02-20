///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_REPRES_AGENT_H__
#define __KH_REPRES_AGENT_H__

#include <string>
#include <variant>

#include "attributemap.h"
#include "repair_modes.h"
#include "resupply_modes.h"

class RepresParam
{
  public:
    RepresParam(int gid, int mid, char player)
        : m_gid(gid), m_mid(mid), m_player(player)
    {
    }

    RepresParam() : m_gid(0), m_mid(0), m_player(0)
    {
    }

    int get_game_id() const
    {
        return m_gid;
    }

    int get_module_id() const
    {
        return m_mid;
    }

    char get_player() const
    {
        return m_player;
    }

  protected:
    int m_gid;
    int m_mid;
    char m_player;
};

//////////////////////////////////////////////////////////////////////////
// RepairParam
//
///////////////

class RepairParam : public RepresParam
{
  public:
    RepairParam(int gid, int mid, char player, RepairMode mode,
                const std::string& ship_code, AttributeID attribute, int amount)
        : RepresParam(gid, mid, player), m_mode(mode),
          m_ship_code(ship_code), m_attribute(attribute), m_amount(amount)
    {
    }

    RepairParam()
        : RepresParam(), m_mode(RepairMode::REPAIR_APPLY),
          m_attribute(AttributeID::UNKNOWN), m_amount(0)
    {
    }

    class Builder
    {
      public:
        Builder()
        {
            mode = RepairMode::REPAIR_APPLY;
            attribute = AttributeID::UNKNOWN;
            amount = 0;
            gid = 0;
            player = 0;
        }

        Builder& set_list_mode()
        {
            mode = RepairMode::REPAIR_LIST;
            return *this;
        }

        Builder& set_ship_code(const std::string& code)
        {
            ship_code = code;
            return *this;
        }

        Builder& set_attribute(AttributeID attr)
        {
            attribute = attr;
            return *this;
        }

        Builder& set_amount(int amt)
        {
            amount = amt;
            return *this;
        }

        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }

        Builder& set_player(char _player)
        {
            player = _player;
            return *this;
        }

        RepairParam build() const
        {
            return RepairParam(gid, 0, player, mode, ship_code, attribute,
                               amount);
        }

      protected:
        RepairMode mode;
        std::string ship_code;
        AttributeID attribute;
        int amount;
        int gid;
        char player;
    };

    RepairMode get_mode() const
    {
        return m_mode;
    }

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    AttributeID get_attribute() const
    {
        return m_attribute;
    }

    int get_amount() const
    {
        return m_amount;
    }

  protected:
    RepairMode m_mode;
    std::string m_ship_code;
    AttributeID m_attribute;
    int m_amount;
};

//////////////////////////////////////////////////////////////////////////
// ResupplyParam
//
///////////////

class ResupplyParam : public RepresParam
{
  public:
    ResupplyParam(int gid, int mid, char player, ResupplyMode mode,
                  const std::string& ship_code, int torpedoes)
        : RepresParam(gid, mid, player), m_mode(mode),
          m_ship_code(ship_code), m_torpedoes(torpedoes)
    {
    }

    ResupplyParam()
        : RepresParam(), m_mode(ResupplyMode::RESUPPLY_LIST), m_torpedoes(0)
    {
    }

    class Builder
    {
      public:
        Builder()
        {
            mode = ResupplyMode::RESUPPLY_LIST;
            torpedoes = 0;
            gid = 0;
            player = 0;
        }

        Builder& set_list_mode()
        {
            mode = ResupplyMode::RESUPPLY_LIST;
            return *this;
        }

        Builder& set_torpedoes_mode()
        {
            mode = ResupplyMode::RESUPPLY_TORPEDOES;
            return *this;
        }

        Builder& set_ship_code(const std::string& code)
        {
            ship_code = code;
            return *this;
        }

        Builder& set_torpedoes(int n)
        {
            torpedoes = n;
            return *this;
        }

        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }

        Builder& set_player(char _player)
        {
            player = _player;
            return *this;
        }

        ResupplyParam build() const
        {
            return ResupplyParam(gid, 0, player, mode, ship_code, torpedoes);
        }

      protected:
        ResupplyMode mode;
        std::string ship_code;
        int torpedoes;
        int gid;
        char player;
    };

    ResupplyMode get_mode() const
    {
        return m_mode;
    }

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    int get_torpedoes() const
    {
        return m_torpedoes;
    }

  protected:
    ResupplyMode m_mode;
    std::string m_ship_code;
    int m_torpedoes;
};

using RepresAgentParam = std::variant<RepairParam, ResupplyParam>;

class RepresAgent
{
  public:
    static RepresAgent& instance()
    {
        static RepresAgent _instance;
        return _instance;
    }

    RepresAgent(const RepresAgent&) = delete;
    RepresAgent& operator=(const RepresAgent&) = delete;

    bool apply(RepresAgentParam& param);
    bool apply(RepairParam& param);
    bool apply(ResupplyParam& param);

  private:
    RepresAgent() = default;
    ~RepresAgent() = default;
};

#endif
