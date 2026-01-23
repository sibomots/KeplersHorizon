//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_COMBAT_AGENT_H__
#define __KH_COMBAT_AGENT_H__

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "typedefs.h"
#include "attributemap.h"
#include "util.h"
#include "combattypes.h"

class CombatParam
{
  public:
    class Builder
    {
        friend class CombatOrderParam;
        friend class CombatApplyParam;
        friend class CombatRetreatParam;
        friend class CombatCancelParam;
        friend class CombatCommitParam;
        friend class CombatStatusParam;
        friend class CombatDraftsParam;

      public:
        Builder& set_module_id(int _mid)
        {
            mid = _mid;
            return *this;
        }
        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }
        Builder& set_player(char user)
        {
            player = user;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        CombatParam build() const
        {
            return CombatParam(gid, mid, player);
        }
      protected:
        int mid;
        int gid;
        char player;
    };

    CombatParam(int gid, int mid, char user)
        : m_gid(gid), m_mid(mid), m_user(user)
    {
    }

    CombatParam() : m_gid(0), m_mid(0), m_user(0)
    {
    }

  protected:
    // Game ID
    int m_gid;
    // Module ID
    int m_mid;
    // the owner of the attacker ship is always this 'player'
    char m_user; // 'A' or 'B'
};

class CombatCommitParam : public CombatParam
{
  public:
    CombatCommitParam(int gid, int mid, char user) : CombatParam(gid, mid, user)
    {
    }
    CombatCommitParam() : CombatParam()
    {
    }
    class Builder
    {
      public:
        Builder& set_module_id(int _mid)
        {
            mid = _mid;
            return *this;
        }
        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }
        Builder& set_player(char user)
        {
            player = user;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        CombatCommitParam build() const
        {
            return CombatCommitParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };

    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }
};

class CombatCancelParam : public CombatParam
{
  public:
    CombatCancelParam(int gid, int mid, char user) : CombatParam(gid, mid, user)
    {
    }
    CombatCancelParam() : CombatParam()
    {
    }
    class Builder
    {
      public:
        Builder& set_module_id(int _mid)
        {
            mid = _mid;
            return *this;
        }
        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }
        Builder& set_player(char user)
        {
            player = user;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        CombatCancelParam build() const
        {
            return CombatCancelParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };
    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }
};

class CombatDraftsParam : public CombatParam
{
  public:
    CombatDraftsParam(int gid, int mid, char user) : CombatParam(gid, mid, user)
    {
    }
    CombatDraftsParam() : CombatParam()
    {
    }
    class Builder
    {
      public:
        Builder& set_module_id(int _mid)
        {
            mid = _mid;
            return *this;
        }
        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }
        Builder& set_player(char user)
        {
            player = user;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        CombatDraftsParam build() const
        {
            return CombatDraftsParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };
    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }
};

class CombatStatusParam : public CombatParam
{
  public:
    CombatStatusParam(int gid, int mid, char user) : CombatParam(gid, mid, user)
    {
    }
    CombatStatusParam() : CombatParam()
    {
    }
    class Builder
    {
      public:
        Builder& set_module_id(int _mid)
        {
            mid = _mid;
            return *this;
        }
        Builder& set_game_id(int _gid)
        {
            gid = _gid;
            return *this;
        }
        Builder& set_player(char user)
        {
            player = user;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        CombatStatusParam build() const
        {
            return CombatStatusParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };
    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }
};

class CombatOrderParam : public CombatParam
{
  public:
    CombatOrderParam(int gid, int mid, char user, std::string attacker,
                     std::string attackee, char tactic,
                     AttributeMap attr, MissileSet missiles)
        : CombatParam(gid, mid, user), m_tactic(tactic), m_attacker(attacker),
          m_attackee(attackee), m_attr(attr), m_missiles(missiles)
    {
    }

    CombatOrderParam() : CombatParam(), m_tactic(0)
    {
    }

    class Builder
    {
      public:
        // Base class CombatParam builder methods
        Builder& set_module_id(int _mid)
        {
            base_builder.set_module_id(_mid);
            return *this;
        }

        Builder& set_game_id(int _gid)
        {
            base_builder.set_game_id(_gid);
            return *this;
        }

        Builder& set_player(char _user)
        {
            base_builder.set_player(_user);
            return *this;
        }
        Builder& set_attacker(std::string _attacker)
        {
            attacker = _attacker;
            return *this;
        }
        Builder& set_attackee(std::string _attackee)
        {
            attackee = _attackee;
            return *this;
        }
        Builder& set_tactic(char _tactic)
        {
            tactic = _tactic;
            return *this;
        }
        Builder& set_attr(AttributeMap _attr)
        {
            attr = _attr;
            return *this;
        }
        Builder& set_missiles(MissileSet _missiles)
        {
            missiles = _missiles;
            return *this;
        }

        CombatOrderParam build() const
        {
            return CombatOrderParam(base_builder.gid, base_builder.mid,
                                    base_builder.player, attacker, attackee,
                                    tactic, attr, missiles);
        }

      protected:
        CombatParam::Builder base_builder;
        std::string attacker;
        std::string attackee;
        char tactic;
        AttributeMap attr;
        MissileSet missiles;
    };

    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }

    std::string get_attacker()
    {
        return m_attacker;
    }
    std::string get_attackee()
    {
        return m_attackee;
    }
    char get_tactic()
    {
        return m_tactic;
    }
    AttributeMap get_attr()
    {
        return m_attr;
    }
    MissileSet get_missiles()
    {
        return m_missiles;
    }

  private:
    std::string m_attacker;
    std::string m_attackee;
    char m_tactic;
    AttributeMap m_attr;
    MissileSet m_missiles;
};

class CombatApplyParam : public CombatParam
{
  public:
    CombatApplyParam(int gid, int mid, char user, std::string target_ship,
                     AttributeMap attr)
        : CombatParam(gid, mid, user), m_target_ship(target_ship), m_assignments(attr)
    {
    }

    CombatApplyParam() : CombatParam()
    {
    }

    class Builder
    {
      public:
        // Base class CombatParam builder methods
        Builder& set_module_id(int _mid)
        {
            base_builder.set_module_id(_mid);
            return *this;
        }

        Builder& set_game_id(int _gid)
        {
            base_builder.set_game_id(_gid);
            return *this;
        }

        Builder& set_player(char _user)
        {
            base_builder.set_player(_user);
            return *this;
        }
        Builder& set_target_ship(std::string _target_ship)
        {
            target_ship = _target_ship;
            return *this;
        }
        Builder& set_assignments(AttributeMap _attr)
        {
            attr = _attr;
            return *this;
        }
        CombatApplyParam build() const
        {
            return CombatApplyParam(base_builder.gid, base_builder.mid,
                                    base_builder.player, target_ship, attr);
        }

      protected:
        CombatParam::Builder base_builder;
        std::string target_ship;
        AttributeMap attr;
    };

    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }

    std::string get_target_ship()
    {
        return m_target_ship;
    }

    AttributeMap get_assignments()
    {
        return m_assignments;
    }

  private:
    std::string m_target_ship;
    AttributeMap m_assignments;
};

class CombatRetreatParam : public CombatParam
{
  public:
    CombatRetreatParam(int gid, int mid, char user, std::string target_ship,
                       std::string destination)
        : CombatParam(gid, mid, user), m_target_ship(target_ship),
          m_destination(destination)
    {
    }

    CombatRetreatParam() : CombatParam()
    {
    }

    class Builder
    {
      public:
        // Base class CombatParam builder methods
        Builder& set_module_id(int _mid)
        {
            base_builder.set_module_id(_mid);
            return *this;
        }

        Builder& set_game_id(int _gid)
        {
            base_builder.set_game_id(_gid);
            return *this;
        }

        Builder& set_player(char _user)
        {
            base_builder.set_player(_user);
            return *this;
        }
        Builder& set_target_ship(std::string _target_ship)
        {
            target_ship = _target_ship;
            return *this;
        }
        Builder& set_destination(std::string _destintion)
        {
            destination = _destintion;
            return *this;
        }
        CombatRetreatParam build() const
        {
            return CombatRetreatParam(base_builder.gid, base_builder.mid,
                                      base_builder.player, target_ship,
                                      destination);
        }

      protected:
        CombatParam::Builder base_builder;
        std::string target_ship;
        std::string destination;
    };

    int get_game_id()
    {
        return m_gid;
    }
    int get_module_id()
    {
        return m_mid;
    }
    char get_player()
    {
        return m_user;
    }
    std::string get_target_ship()
    {
        return m_target_ship;
    }
    std::string get_destination()
    {
        return m_destination;
    }

  private:
    std::string m_target_ship;
    std::string m_destination;
};

//////////////////////////////////////////////
//////////////////////////////////////////////

typedef std::variant<CombatStatusParam, CombatCancelParam, CombatCommitParam,
                     CombatDraftsParam, CombatRetreatParam, CombatApplyParam,
                     CombatOrderParam, CombatParam> CombatAgentPayload;

class CombatAgentParam
{
  public:
    CombatAgentPayload m_payload;

    CombatOpCode m_opcode;
    CombatAgentParam() : m_opcode(CombatOpCode::KH_CNOP)
    {
    }

    explicit CombatAgentParam(const CombatParam& param)
        : m_opcode(CombatOpCode::KH_CNOP), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatCommitParam& param)
        : m_opcode(CombatOpCode::KH_CCOMMIT), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatCancelParam& param)
        : m_opcode(CombatOpCode::KH_CCANCEL), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatStatusParam& param)
        : m_opcode(CombatOpCode::KH_CSTATUS), m_payload(param)
    {
    }

    explicit CombatAgentParam(const CombatRetreatParam& param)
        : m_opcode(CombatOpCode::KH_CRETR), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatDraftsParam& param)
        : m_opcode(CombatOpCode::KH_CDRAFTS), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatApplyParam& param)
        : m_opcode(CombatOpCode::KH_CAPPLY), m_payload(param)
    {
    }
    explicit CombatAgentParam(const CombatOrderParam& param)
        : m_opcode(CombatOpCode::KH_CORDER), m_payload(param)
    {
    }

    // Query what it currently holds
    bool isCombatParam() const
    {
        return std::holds_alternative<CombatParam>(m_payload);
    }
    bool isCombatOrderParam() const
    {
        return std::holds_alternative<CombatOrderParam>(m_payload);
    }
    bool isCombatApplyParam() const
    {
        return std::holds_alternative<CombatApplyParam>(m_payload);
    }
    bool isCombatRetreatParam() const
    {
        return std::holds_alternative<CombatRetreatParam>(m_payload);
    }
    bool isCombatCommitParam() const
    {
        return std::holds_alternative<CombatCommitParam>(m_payload);
    }
    bool isCombatCancelParam() const
    {
        return std::holds_alternative<CombatCancelParam>(m_payload);
    }
    bool isCombatDraftsParam() const
    {
        return std::holds_alternative<CombatDraftsParam>(m_payload);
    }
    bool isCombatStatusParam() const
    {
        return std::holds_alternative<CombatStatusParam>(m_payload);
    }

    // Accessors (throws std::bad_variant_access if wrong type)
    CombatParam& asCombatParam()
    {
        return std::get<CombatParam>(m_payload);
    }
    CombatOrderParam& asCombatOrderParam()
    {
        return std::get<CombatOrderParam>(m_payload);
    }
    CombatApplyParam& asCombatApplyParam()
    {
        return std::get<CombatApplyParam>(m_payload);
    }
    CombatRetreatParam& asCombatRetreatParam()
    {
        return std::get<CombatRetreatParam>(m_payload);
    }
    CombatCommitParam& asCombatCommitParam()
    {
        return std::get<CombatCommitParam>(m_payload);
    }
    CombatCancelParam& asCombatCancelParam()
    {
        return std::get<CombatCancelParam>(m_payload);
    }
    CombatDraftsParam& asCombatDraftsParam()
    {
        return std::get<CombatDraftsParam>(m_payload);
    }
    CombatStatusParam& asCombatStatusParam()
    {
        return std::get<CombatStatusParam>(m_payload);
    }

    const CombatParam& asCombatParam() const
    {
        return std::get<CombatParam>(m_payload);
    }
    const CombatOrderParam& asCombatOrderParam() const
    {
        return std::get<CombatOrderParam>(m_payload);
    }
    const CombatApplyParam& asCombatApplyParam() const
    {
        return std::get<CombatApplyParam>(m_payload);
    }
    const CombatRetreatParam& asCombatRetreatParam() const
    {
        return std::get<CombatRetreatParam>(m_payload);
    }
    const CombatCommitParam& asCombatCommitParam() const
    {
        return std::get<CombatCommitParam>(m_payload);
    }
    const CombatCancelParam& asCombatCancelParam() const
    {
        return std::get<CombatCancelParam>(m_payload);
    }
    const CombatStatusParam& asCombatStatusParam() const
    {
        return std::get<CombatStatusParam>(m_payload);
    }
    const CombatDraftsParam& asCombatDraftsParam() const
    {
        return std::get<CombatDraftsParam>(m_payload);
    }

    friend std::ostream& operator<<(std::ostream& os, CombatAgentParam& param)
    {
        std::visit(
            [&](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, CombatParam>)
                {
                    os << "holds CombatParam:"
                       << "\n";
                }
                else if constexpr (std::is_same_v<T, CombatOrderParam>)
                {
                    os << "holds CombatOrderParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatApplyParam>)
                {
                    os << "holds CombatApplyParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatRetreatParam>)
                {
                    os << "holds CombatRetreatParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatCommitParam>)
                {
                    os << "holds CombatCommitParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatCancelParam>)
                {
                    os << "holds CombatCancelParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatDraftsParam>)
                {
                    os << "holds CombatDraftsParam:\n";
                }
                else if constexpr (std::is_same_v<T, CombatStatusParam>)
                {
                    os << "holds CombatStatusParam:\n";
                }
                else
                {
                    os << "not holding CombatParam:\n";
                }
            },
            param.m_payload);
        return os;
    }
};

class CombatAgent
{
  public:
    static CombatAgent& getInstance()
    {
        static CombatAgent instance;
        return instance;
    }


    bool apply(CombatAgentParam& caparam);

    CombatAgent(const CombatAgent&) = delete;
    CombatAgent& operator=(const CombatAgent&) = delete;
    CombatAgent(CombatAgent&&) = delete;
    CombatAgent& operator=(CombatAgent&&) = delete;

  private:
    CombatAgent() = default;
    ~CombatAgent() = default;


    bool apply(CombatParam& param);
    bool apply(CombatOrderParam& param);
    bool apply(CombatApplyParam& param);
    bool apply(CombatRetreatParam& param);
    bool apply(CombatCommitParam& param);
    bool apply(CombatCancelParam& param);
    bool apply(CombatDraftsParam& param);
    bool apply(CombatStatusParam& param);


    CombatSessionState get_combat_state_at_hex(const int gid, const std::string& hex_id);
    bool is_param_valid(const CombatAgentPayload& param) const;
};

#endif
