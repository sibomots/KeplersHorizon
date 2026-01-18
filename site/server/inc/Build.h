//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __BUILD_ALL_COMMANDS_H__
#define __BUILD_ALL_COMMANDS_H__

#include <string>
#include "typedefs.h"
#include "attributemap.h"
#include "icmd.h"
#include "statemachine.h"

class BuildListDraftsCommand : public ICmd
{
  public:
    std::string m_target;
    bool        m_btargeted;
    class Builder
    {
      public:
        std::string _target;
        bool _btargeted;

        Builder()
        {
            _btargeted = false;
        }

        Builder& set_draft_target(std::string target) {
             _target = target;
             _btargeted = true;
             return *this;
        }
     
        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildListDraftsCommand(*this));
        }
    };

    virtual bool invoke(void);

  private:
    BuildListDraftsCommand(Builder& builder)
    {
       if (builder._btargeted) 
       {
          m_target = std::move(builder._target);
          m_btargeted = builder._btargeted;
       }
    }
};

class BuildNewCommand : public ICmd
{
  public:
    std::string m_ship_code;
    std::string m_ship_name;

    class Builder
    {
      public:
        std::string _ship_code;
        std::string _ship_name;

        Builder& set_ship_code(const std::string& code)
        {
            _ship_code = code;
            return *this;
        }
        Builder& set_ship_name(const std::string& name)
        {
            _ship_name = name;
            return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildNewCommand(*this));
        }
    };

    virtual bool invoke(void);

  private:
    BuildNewCommand(Builder& builder)
        : m_ship_code(std::move(builder._ship_code)),
          m_ship_name(std::move(builder._ship_name))
    {
    }
};


class BuildSetAttributeCommand : public ICmd
{
  public:
    AttributeMap m_attributes;
    std::string m_target;
    bool m_btargeted;

    class Builder
    {
      public:
        AttributeMap _attributes;
        std::string _target;
        bool _btargeted;

        Builder()
        {
            _btargeted = false;
        }

        Builder& set_pd(int val)
        {
            _attributes[AttributeID::POWER_DRIVE] = val;
            return *this;
        }

        Builder& set_b(int val)
        {
            _attributes[AttributeID::BEAM] = val;
            return *this;
        }

        Builder& set_s(int val)
        {
            _attributes[AttributeID::SCREEN] = val;
            return *this;
        }

        Builder& set_t(int val)
        {
            _attributes[AttributeID::TUBE] = val;
            return *this;
        }

        Builder& set_m(int val)
        {
            _attributes[AttributeID::MISSILE] = val;
            return *this;
        }

        Builder& set_sr(int val)
        {
            _attributes[AttributeID::SYSTEM_RACK] = val;
            return *this;
        }

        Builder& set_attributes(AttributeMap* pmap)
        {
            if (pmap)
            {
               _attributes = std::move(*pmap); 
            }
            return *this;
        }
        Builder& set_draft_target(std::string* ptarget) 
        {
            if (ptarget) {
               _btargeted = true;
               _target = std::string(*ptarget);
            }
            return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildSetAttributeCommand(*this));
        }
    };

    virtual bool invoke(void);

  private:
    BuildSetAttributeCommand(Builder& builder)
        : m_attributes(std::move(builder._attributes)),
          m_btargeted(builder._btargeted),
          m_target(std::move(builder._target))
    {
    }
};

class BuildCommitCommand : public ICmd
{
  public:
    std::string m_target;
    bool        m_btargeted;

    class Builder
    {
      public:
        std::string _target;
        bool _btargeted;

        Builder()
        {
            _btargeted = false;
        }

        Builder& set_draft_target(std::string* ptarget) 
        {
           if (ptarget)
           {
              _target = std::string(*ptarget);
              _btargeted = true;
           }
           return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildCommitCommand(*this));
        }
    };
    virtual bool invoke(void);

  private:
    BuildCommitCommand(Builder& builder)
       : m_target(std::move(builder._target)), m_btargeted(builder._btargeted)
    {
    }

};


class BuildCancelCommand : public ICmd
{
  public:
    std::string m_target;
    bool        m_btargeted;

    class Builder
    {
      public:
        std::string _target;
        bool _btargeted;

        Builder()
        {
            _btargeted = false;
        }

        Builder& set_draft_target(std::string* ptarget) 
        {
           if (ptarget)
           {
              _target = std::string(*ptarget);
              _btargeted = true;
           }
           return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildCancelCommand(*this));
        }
    };
    virtual bool invoke(void);

  private:
    BuildCancelCommand(Builder& builder)
       : m_target(std::move(builder._target)), m_btargeted(builder._btargeted)
    {
    }

};

class BuildCommand : public ICmd
{
  public:
    std::string m_draft_code;

    class Builder
    {
      public:
        std::string _draft_code;

        Builder& set_draft_code(const std::string& code)
        {
            _draft_code = code;
            return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildCommand(*this));
        }
    };

    virtual bool invoke(void);

  private:
    BuildCommand(Builder& builder)
        : m_draft_code(std::move(builder._draft_code))
    {
    }
};


class BuildShowDraftCommand : public ICmd
{
  public:
    std::string m_target;
    bool m_btargeted;
    class Builder
    {
      public:
        std::string _target;
        bool _btargeted;

        Builder()
        {
           _btargeted = false;
        }
        Builder& set_draft_target(const std::string& target)
        {
            _target = target;
            _btargeted = true;
            return *this;
        }

        ICmd* build()
        {
            return dynamic_cast<ICmd*>(new BuildShowDraftCommand(*this));
        }
    };

    virtual bool invoke(void);

  private:
    BuildShowDraftCommand(Builder& builder)
        : m_target(std::move(builder._target)), m_btargeted(builder._btargeted)
    {
    }
};

#endif
