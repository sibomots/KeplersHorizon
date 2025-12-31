#ifndef __SHIP_ATTR_H__
#define __SHIP_ATTR_H__
#include <map>
#include <string>

#include "typedefs.h"
#include "app.h"
#include "db.h"
#include "combat.h"

class ShipAttributes
{
  public:
    char type;
    int tech;
    int PD;
    int B;
    int S;
    int T;
    int M;
    int SR;

  public:
    ShipAttributes()
    {
        type = 'W';
        tech = 0;
        PD = 0;
        B = 0;
        S = 0;
        T = 0;
        M = 0;
        SR = 0;
    }
};

class DraftRow
{
  public:
    std::string code;
    std::string name;
    ShipAttributes attr;
    DraftRow()
    {
        attr.type = 'W';
    }
};

class ShipRow
{
  public:
    std::string code;
    std::string name;
    std::string built_turn;
    std::string at_system;
    std::string at_hex;
    std::string racked_in;
    int pd_spent = 0;
    ShipAttributes attr;

  public:
    // Default constructor
    ShipRow()
    {
    }

    // Constructor from DraftRow with tech level and turn info
    ShipRow(const DraftRow& draft, int tech_level,
            const std::string& turn_built)
        : code(draft.code), name(draft.name), built_turn(turn_built),
          pd_spent(0)
    {
        attr = draft.attr;
        attr.tech = tech_level;
    }

    // Factory method for creating from draft
    static ShipRow from_draft(const DraftRow& draft, int tech_level, int round,
                              const std::string& active_player)
    {
        std::string turn_built = "R" + std::to_string(round) + active_player;
        return ShipRow(draft, tech_level, turn_built);
    }
};

std::string get_current_draft(int game_id, char owner);
void set_current_draft(int game_id, char owner, const std::string& code_or_null);
bool draft_exists(int game_id, char owner, const std::string& code);
bool ship_exists(int game_id, char owner, const std::string& code);
std::vector<DraftRow> load_drafts(int game_id, char owner);
DraftRow load_draft(int game_id, char owner, const std::string& code);
void insert_draft(int game_id, char owner, const DraftRow& d);
void update_draft_attrs(int game_id, char owner, const std::string& code, const DraftRow& d);
void delete_draft(int game_id, char owner, const std::string& code);
std::vector<ShipRow> load_ships(int game_id, char owner);
ShipRow load_ship(int game_id, char owner, const std::string& code);
int count_racked_in(int game_id, char owner, const std::string& warpship_code);
void insert_ship(int game_id, char owner, const ShipRow& s);
void update_ship_location(int game_id, char owner, const std::string& code, const std::string& at_system, const std::string& at_hex, const std::string& racked_in);

#endif
