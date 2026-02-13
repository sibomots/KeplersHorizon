///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SAVE_COMMAND_H__
#define __KH_SAVE_COMMAND_H__

#include <string>

#include "icmd.h"
#include "typedefs.h"

// SaveCommand - saves current game as a named bookmark
class SaveCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_name(const std::string& name)
        {
            m_name = name;
            return *this;
        }

        Builder& set_show_usage()
        {
            m_show_usage = true;
            return *this;
        }

        ICmd* build()
        {
            SaveCommand* cmd = new SaveCommand();
            cmd->m_name = m_name;
            cmd->m_show_usage = m_show_usage;
            return cmd;
        }

      private:
        std::string m_name;
        bool m_show_usage = false;
    };

    bool invoke() override;
    CommandID id()
    {
        return CommandID::SAVE;
    }

  private:
    std::string m_name;
    bool m_show_usage = false;
};

// LoadCommand - lists saves or initiates two-factor load
class LoadCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_name(const std::string& name)
        {
            m_name = name;
            return *this;
        }

        Builder& set_list_saves()
        {
            m_list_saves = true;
            return *this;
        }

        ICmd* build()
        {
            LoadCommand* cmd = new LoadCommand();
            cmd->m_name = m_name;
            cmd->m_list_saves = m_list_saves;
            return cmd;
        }

      private:
        std::string m_name;
        bool m_list_saves = false;
    };

    bool invoke() override;
    CommandID id()
    {
        return CommandID::LOAD;
    }

  private:
    std::string m_name;
    bool m_list_saves = false;
};

// AcceptCommand - confirms a pending load request
class AcceptCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_name(const std::string& name)
        {
            m_name = name;
            return *this;
        }

        ICmd* build()
        {
            AcceptCommand* cmd = new AcceptCommand();
            cmd->m_name = m_name;
            return cmd;
        }

      private:
        std::string m_name;
    };

    bool invoke() override;
    CommandID id()
    {
        return CommandID::ACCEPT;
    }

  private:
    std::string m_name;
};

// RejectCommand - declines a pending load request
class RejectCommand : public ICmd
{
  public:
    class Builder
    {
      public:
        Builder& set_name(const std::string& name)
        {
            m_name = name;
            return *this;
        }

        ICmd* build()
        {
            RejectCommand* cmd = new RejectCommand();
            cmd->m_name = m_name;
            return cmd;
        }

      private:
        std::string m_name;
    };

    bool invoke() override;
    CommandID id()
    {
        return CommandID::REJECT;
    }

  private:
    std::string m_name;
};

#endif
