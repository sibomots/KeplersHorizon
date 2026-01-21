#ifndef __SHIP_ATTR_H__
#define __SHIP_ATTR_H__
#include <map>
#include <string>
#include <ostream>
#include <sstream>
#include <vector>
#include <iomanip>

#include "app.h"
#include "combat.h"
#include "db.h"
#include "typedefs.h"
#include "kherr.h"
#include "localization.h"

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

    int LS;
    int TB;
    int DN;

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
        LS = 0;
        TB = 0;
        DN = 0;
    }
    void deep_copy(ShipAttributes& dest, const ShipAttributes& src)
    {
        dest.tech = src.tech;
        dest.type = src.type;
        dest.PD = src.PD;
        dest.B = src.B;
        dest.S = src.S;
        dest.T = src.T;
        dest.M = src.M;
        dest.SR = src.SR;
        dest.LS = src.LS;
        dest.TB = src.TB;
        dest.DN = src.DN;
    }

    ShipAttributes& operator=(const ShipAttributes& rhs)
    {
        if (this != &rhs)
        {
           deep_copy(*this, rhs);
        }
        return *this;
    }

    ShipAttributes(const ShipAttributes& rhs)
    {
         deep_copy(*this, rhs);
    }
};

class ShipRow;

class DraftRow
{
  private:
  ShipAttributes attr;

  public:

    friend class ShipRow;

    int total_cost;
    std::string code;
    std::string name;
    DraftRow()
    {
        attr.type = 'W';
        total_cost = 0;
    }

    friend std::ostream& operator<<(std::ostream& os, DraftRow& ob)
    {
        os << "Code: " << ob.get_code()
           << " Name: " << ob.get_name()
           << " Technology Level: " << std::to_string(ob.get_tech()) << "\n"
           << "Cost: " << ob.get_cost() << "\n"
           << "    Attributes: "
           << "[PD=" << ob.get_PD() << " "
           << "B=" << ob.get_B() << " "
           << "S=" << ob.get_S() << " "
           << "T=" << ob.get_T() << " "
           << "M=" << ob.get_M() << " "
           << "SR=" << ob.get_SR() << "]\n";
        return os;
    }
            
    // clang-format off
    void set_code(std::string c) { code = c; }
    void set_name(std::string n) { name = n; }
    void set_tech(int t) { attr.tech = t; }
    void set_type(char t) { attr.type = t; }
    void set_PD(int pd) { attr.PD = pd; }
    void set_B(int b) { attr.B = b; }
    void set_S(int s) { attr.S = s; }
    void set_T(int t) { attr.T = t; }
    void set_M(int m) { attr.M = m; }
    void set_SR(int sr) { attr.SR = sr; }

    std::string get_code() const { return std::string(code); }
    std::string get_name() const { return std::string(name); }
    int get_tech() const { return attr.tech; }
    char get_type() const { return attr.type; }
    int get_PD() const { return attr.PD; }
    int get_B() const { return attr.B; }
    int get_S() const { return attr.S; }
    int get_T() const { return attr.T; }
    int get_M() const { return attr.M; }
    int get_SR() const { return attr.SR; }
    int get_cost() const { return total_cost; }
    // clang-format on
    
    void update_cost()
    {
       int subtotal = 0;
       switch(attr.type)
       {
         case 'W':
             subtotal += 5; // Warp Drive Cost
         default:
             subtotal += 0; // No Warp Drive
       }
       subtotal += attr.PD;
       subtotal += attr.B;
       subtotal += attr.S;
       subtotal += attr.T;
       subtotal += attr.M;
       subtotal += attr.SR;
       total_cost = subtotal;
    }

    bool value_ranges_invalid() {
      return (attr.PD < 0) ||
             (attr.B < 0) ||
             (attr.S < 0) ||
             (attr.T < 0) ||
             (attr.M < 0) ||
             (attr.SR < 0);
    }
    bool system_ship_fitment_invalid() {
      return ((attr.type != 'W') && (attr.SR > 0));
    }
    bool missile_count_invalid() {
      return ((attr.M > 0) && (( attr.M % 3) != 0));
    }
    bool tube_missile_match_invalid() {
      return ((attr.T < 1) && missile_count_invalid());
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
    int total_cost;
    ShipAttributes attr;

  public:
    // Default constructor
    ShipRow()
    {
      total_cost = 0;
    }

    ShipRow(const ShipRow& rhs)
    {
          code = rhs.code;
          name = rhs.name;
          built_turn = rhs.built_turn;
          at_system = rhs.at_system;
          at_hex = rhs.at_hex;
          racked_in = rhs.racked_in;
          pd_spent = rhs.pd_spent;
          total_cost = rhs.total_cost;
          attr = rhs.attr;
    }

    ShipRow(const DraftRow& draft, const std::string& turn_built)
        : code(draft.code), name(draft.name), built_turn(turn_built),
          pd_spent(0), total_cost(draft.total_cost)
    {
        attr = draft.attr;
    }

    // Factory method for creating from draft
    static ShipRow from_draft(const DraftRow& draft, int round,
                              const std::string& active_player)
    {
        std::string turn_built = "R" + std::to_string(round) + active_player;
        return ShipRow(draft, turn_built);
    }

    friend std::ostream& operator<<(std::ostream& os, ShipRow& ob)
    {
        os << ob.name << " - " << ob.code
           << " (Tech Level " << std::to_string(ob.get_tech()) << ")\n"
           << "Cost: " << std::to_string(ob.get_cost()) << " BP";
        return os;
    }

    // clang-format off
    void set_code(std::string c) { code = c; }
    void set_name(std::string n) { name = n; }
    void set_tech(int t) { attr.tech = t; }
    void set_type(char t) { attr.type = t; }
    void set_PD(int pd) { attr.PD = pd; }
    void set_B(int b) { attr.B = b; }
    void set_S(int s) { attr.S = s; }
    void set_T(int t) { attr.T = t; }
    void set_M(int m) { attr.M = m; }
    void set_SR(int sr) { attr.SR = sr; }
    void set_LS(int lrs) { attr.LS = lrs; }
    void set_TB(int tb) { attr.TB = tb; }
    void set_DN(int dn) { attr.DN = dn; }

    std::string get_code() const { return std::string(code); }
    std::string get_name() const { return std::string(name); }
    int get_tech() const { return attr.tech; }
    char get_type() const { return attr.type; }
    int get_PD() const { return attr.PD; }
    int get_B() const { return attr.B; }
    int get_S() const { return attr.S; }
    int get_T() const { return attr.T; }
    int get_M() const { return attr.M; }
    int get_SR() const { return attr.SR; }
    int get_LS() const { return attr.LS; }
    int get_TB() const { return attr.TB; }
    int get_DN() const { return attr.DN; }
    int get_cost() const { return total_cost; }
    std::string get_sector() const {
       if (at_system.empty()) {
           return std::string(at_hex);
       }
       else {
           return std::string( at_system + " H" + at_hex);
       }
    }
      
    // clang-format on
};

// jdw std::string get_current_draft(int game_id, char owner);
// jdw  void set_current_draft(int game_id, char owner, const std::string& code_or_null);
bool draft_exists(int game_id, char owner, const std::string& code);
bool ship_exists(int game_id, char owner, const std::string& code);
std::vector<DraftRow> load_drafts_by_owner(int game_id, char owner);
DraftRow load_draft(int game_id, char owner, const std::string& code);
void insert_draft(int game_id, char owner, const DraftRow& d);
void update_draft_attrs(int did, int game_id, char owner, const std::string& code,
                        const DraftRow& d);
void delete_draft(int did, int game_id, char owner, const std::string& code);
std::vector<ShipRow> load_ships(int game_id, char owner);
ShipRow load_ship(int game_id, char owner, const std::string& code);
ShipRow load_ship_by_code_or_name(int game_id, char owner, const std::string& code);
int count_racked_in(int game_id, char owner, const std::string& warpship_code);
void insert_ship(int game_id, char owner, const ShipRow& s);
void update_ship_location(int game_id, char owner, const std::string& code,
                          const std::string& at_system,
                          const std::string& at_hex,
                          const std::string& racked_in = "");

void update_ship_location_by_code_or_name(int game_id, char owner,
                          const std::string& code,
                          const std::string& at_system,
                          const std::string& at_hex,
                          const std::string& racked_in = "");

bool get_draft_by_spec(int& did, int gid, char owner, std::string target);
bool load_ship_draft_by_spec(DraftRow& row, int did, int game_id, char owner, const std::string& code);
bool test_ship_draft_candidate(DraftRow& drow , std::vector<std::string>& report);
bool ship_exists_by_code_or_name(int game_id, char owner, const std::string& code);

void append_draft_header(std::ostringstream& out, size_t num_production = 0);
void append_draft_row(std::ostringstream& out, const DraftRow& row);
void append_fleet_header(std::ostringstream& out, int vessels_operational);
void append_fleet_row(std::ostringstream& out, const ShipRow& row);
void build_drafts_report(std::ostringstream& out, std::vector<DraftRow>& drafts);

#endif

