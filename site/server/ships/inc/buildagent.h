//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __KH_BUILD_AGENT_H__
#define __KH_BUILD_AGENT_H__

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "attributemap.h"
#include "typedefs.h"
#include "util.h"

// Base parameter class for all Build operations
class BuildParam
{
  public:
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
        BuildParam build() const
        {
            return BuildParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };

    BuildParam(int gid, int mid, char user)
        : m_gid(gid), m_mid(mid), m_user(user)
    {
    }

    BuildParam() : m_gid(0), m_mid(0), m_user(0)
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
        return m_user;
    }

  protected:
    int m_gid;   // Game ID
    int m_mid;   // Module ID
    char m_user; // 'A' or 'B'
};

// Parameter class for BuildNew operation
class BuildNewParam : public BuildParam
{
  public:
    BuildNewParam(int gid, int mid, char user, char ship_type,
                  const std::string& ship_code, const std::string& ship_name)
        : BuildParam(gid, mid, user), m_ship_type(ship_type),
          m_ship_code(ship_code), m_ship_name(ship_name)
    {
    }

    BuildNewParam() : BuildParam(), m_ship_type('W')
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
        Builder& set_ship_type(char type)
        {
            ship_type = type;
            return *this;
        }
        Builder& set_ship_code(const std::string& code)
        {
            ship_code = code;
            return *this;
        }
        Builder& set_ship_name(const std::string& name)
        {
            ship_name = name;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            ship_type = 'W';
        }
        BuildNewParam build() const
        {
            return BuildNewParam(gid, mid, player, ship_type, ship_code,
                                 ship_name);
        }

      protected:
        int mid;
        int gid;
        char player;
        char ship_type;
        std::string ship_code;
        std::string ship_name;
    };

    char get_ship_type() const
    {
        return m_ship_type;
    }
    std::string get_ship_code() const
    {
        return m_ship_code;
    }
    std::string get_ship_name() const
    {
        return m_ship_name;
    }

  protected:
    char m_ship_type;
    std::string m_ship_code;
    std::string m_ship_name;
};

// Parameter class for BuildSet operation
class BuildSetParam : public BuildParam
{
  public:
    BuildSetParam(int gid, int mid, char user, const std::string& target,
                  const AttributeMap& attributes)
        : BuildParam(gid, mid, user), m_target(target), m_attributes(attributes)
    {
    }

    BuildSetParam() : BuildParam()
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
        Builder& set_target(const std::string& tgt)
        {
            target = tgt;
            return *this;
        }
        Builder& set_attributes(const AttributeMap& attrs)
        {
            attributes = attrs;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        BuildSetParam build() const
        {
            return BuildSetParam(gid, mid, player, target, attributes);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string target;
        AttributeMap attributes;
    };

    std::string get_target() const
    {
        return m_target;
    }
    AttributeMap get_attributes() const
    {
        return m_attributes;
    }

  protected:
    std::string m_target;
    AttributeMap m_attributes;
};

// Parameter class for BuildCommit operation
class BuildCommitParam : public BuildParam
{
  public:
    BuildCommitParam(int gid, int mid, char user, const std::string& target)
        : BuildParam(gid, mid, user), m_target(target)
    {
    }

    BuildCommitParam() : BuildParam()
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
        Builder& set_target(const std::string& tgt)
        {
            target = tgt;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        BuildCommitParam build() const
        {
            return BuildCommitParam(gid, mid, player, target);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string target;
    };

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_target;
};

// Parameter class for BuildDrafts operation (list all drafts)
class BuildDraftsParam : public BuildParam
{
  public:
    BuildDraftsParam(int gid, int mid, char user) : BuildParam(gid, mid, user)
    {
    }

    BuildDraftsParam() : BuildParam()
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
        BuildDraftsParam build() const
        {
            return BuildDraftsParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };
};

// Parameter class for BuildShowDraft operation (detail on this ship)
class BuildShowDraftParam : public BuildParam
{
  public:
    BuildShowDraftParam(int gid, int mid, char user, std::string target)
        : BuildParam(gid, mid, user), m_target(target)
    {
    }

    BuildShowDraftParam() : BuildParam()
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
        Builder& set_target(const std::string& tgt)
        {
            target = tgt;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        BuildShowDraftParam build() const
        {
            return BuildShowDraftParam(gid, mid, player, target);
        }

      protected:
        int mid;
        int gid;
        std::string target;
        char player;
    };

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_target;
};

// Parameter class for BuildCancel operation
class BuildCancelParam : public BuildParam
{
  public:
    BuildCancelParam(int gid, int mid, char user, const std::string& target)
        : BuildParam(gid, mid, user), m_target(target)
    {
    }

    BuildCancelParam() : BuildParam()
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
        Builder& set_target(const std::string& tgt)
        {
            target = tgt;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }
        BuildCancelParam build() const
        {
            return BuildCancelParam(gid, mid, player, target);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string target;
    };

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_target;
};

// Variant wrapper for all Build parameter types
using BuildAgentParam =
    std::variant<BuildParam, BuildNewParam, BuildSetParam, BuildCommitParam,
                 BuildDraftsParam, BuildCancelParam, BuildShowDraftParam>;

// BuildAgent - Singleton that handles all build operations
class BuildAgent
{
  public:
    static BuildAgent& instance()
    {
        static BuildAgent instance;
        return instance;
    }

    // Deleted copy constructor and assignment operator
    BuildAgent(const BuildAgent&) = delete;
    BuildAgent& operator=(const BuildAgent&) = delete;

    // Apply methods for each parameter type
    bool apply(BuildAgentParam& param);
    bool apply(BuildNewParam& param);
    bool apply(BuildSetParam& param);
    bool apply(BuildCommitParam& param);
    bool apply(BuildDraftsParam& param);
    bool apply(BuildShowDraftParam& param);
    bool apply(BuildCancelParam& param);

  private:
    // return value is false if the spec is not found.
    // else output is the draft id (did)
    bool get_draft_by_spec(int& did, int gid, char owner, std::string target);

    BuildAgent() = default;
    ~BuildAgent() = default;
};

#endif
