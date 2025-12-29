#ifndef __BUILD_SET_ATTRIBUTE_COMMAND_H__
#define __BUILD_SET_ATTRIBUTE_COMMAND_H__

#include <memory>
#include "icmd.h"
#include "typedefs.h"
#include "db.h"

class BuildSetAttributeCommand : public ICmd
{
public:
    class Builder {
    public:
        Db* _db = nullptr;
        int _game_id = 0;
        AttributeMap _attributes;
        
        Builder& set_db(Db* db) { _db = db; return *this; }
        Builder& set_game_id(int id) { _game_id = id; return *this; }
        
        Builder& set_pd(int val) { 
            _attributes[AttributeID::POWER_DRIVE] = val; 
            return *this; 
        }
        
        Builder& set_b(int val) { 
            _attributes[AttributeID::BEAM] = val; 
            return *this; 
        }
        
        Builder& set_s(int val) { 
            _attributes[AttributeID::SCREEN] = val; 
            return *this; 
        }
        
        Builder& set_t(int val) { 
            _attributes[AttributeID::TUBE] = val; 
            return *this; 
        }
        
        Builder& set_m(int val) { 
            _attributes[AttributeID::MISSILE] = val; 
            return *this; 
        }
        
        Builder& set_sr(int val) { 
            _attributes[AttributeID::SYSTEM_RACK] = val; 
            return *this; 
        }
        
        std::unique_ptr<ICmd> build() {
            return std::unique_ptr<BuildSetAttributeCommand>(
                new BuildSetAttributeCommand(_db, _game_id, _attributes));
        }
    };
   
private:
    BuildSetAttributeCommand(Db* db, int game_id, const AttributeMap& attrs) 
        : m_db(db), m_game_id(game_id), m_attributes(attrs) {}
    
    Db* m_db;
    int m_game_id;
    AttributeMap m_attributes;

public:
    virtual bool invoke(void);
};

#endif
