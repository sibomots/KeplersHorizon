///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_SHIP_DATA_H__
#define __KH_SHIP_DATA_H__

#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

class ShipAttributes
{
  public:
    char type;
    int tech;
    int PD;
    int Phasic;
    int Shield;
    int Launcher;
    int Torpedo;
    int Hangar;

    int LS;

  public:
    ShipAttributes()
    {
        type = 'W';
        tech = 0;
        PD = 0;
        Phasic = 0;
        Shield = 0;
        Launcher = 0;
        Torpedo = 0;
        Hangar = 0;
        LS = 0;
    }
    void deep_copy(ShipAttributes& dest, const ShipAttributes& src)
    {
        dest.tech = src.tech;
        dest.type = src.type;
        dest.PD = src.PD;
        dest.Phasic = src.Phasic;
        dest.Shield = src.Shield;
        dest.Launcher = src.Launcher;
        dest.Torpedo = src.Torpedo;
        dest.Hangar = src.Hangar;
        dest.LS = src.LS;
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
        os << "Code: " << ob.get_code() << " Name: " << ob.get_name()
           << " Technology Level: " << std::to_string(ob.get_tech()) << "\n"
           << "Cost: " << ob.get_cost() << "\n"
           << "    Attributes: "
           << "[PD=" << ob.get_PD() << " "
           << "P=" << ob.get_Phasic() << " "
           << "S=" << ob.get_Shield() << " "
           << "L=" << ob.get_Launcher() << " "
           << "T=" << ob.get_Torpedo() << " "
           << "H=" << ob.get_Hanger() << "]\n";
        return os;
    }

    // clang-format off
    void set_code(std::string c) { code = c; }
    void set_name(std::string n) { name = n; }
    void set_tech(int t) { attr.tech = t; }
    void set_type(char t) { attr.type = t; }
    void set_PD(int pd) { attr.PD = pd; }
    void set_Phasic(int b) { attr.Phasic = b; }
    void set_Shield(int s) { attr.Shield = s; }
    void set_Launcher(int t) { attr.Launcher = t; }
    void set_Torpedo(int m) { attr.Torpedo = m; }
    void set_Hanger(int hangar) { attr.Hangar = hangar; }

    std::string get_code() const { return std::string(code); }
    std::string get_name() const { return std::string(name); }
    int get_tech() const { return attr.tech; }
    char get_type() const { return attr.type; }
    int get_PD() const { return attr.PD; }
    int get_Phasic() const { return attr.Phasic; }
    int get_Shield() const { return attr.Shield; }
    int get_Launcher() const { return attr.Launcher; }
    int get_Torpedo() const { return attr.Torpedo; }
    int get_Hanger() const { return attr.Hangar; }
    int get_cost() const { return total_cost; }
    // clang-format on

    void update_cost()
    {
        int subtotal = 0;
        switch (attr.type)
        {
        case 'W':
            subtotal += 5; // Warp Drive Cost
        default:
            subtotal += 0; // No Warp Drive
        }
        subtotal += attr.PD;
        subtotal += attr.Phasic;
        subtotal += attr.Shield;
        subtotal += attr.Launcher;
        subtotal += attr.Torpedo;
        subtotal += attr.Hangar;
        total_cost = subtotal;
    }

    bool value_ranges_invalid()
    {
        return (attr.PD < 0) || (attr.Phasic < 0) || (attr.Shield < 0) || (attr.Launcher < 0) ||
               (attr.Torpedo < 0) || (attr.Hangar < 0);
    }
    bool system_ship_fitment_invalid()
    {
        return ((attr.type != 'W') && (attr.Hangar > 0));
    }
    bool torpedo_count_invalid()
    {
        return ((attr.Torpedo > 0) && ((attr.Torpedo % 3) != 0));
    }
    bool launcher_torpedo_match_invalid()
    {
        return ((attr.Launcher < 1) && torpedo_count_invalid());
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
        os << ob.name << " - " << ob.code << " (Tech Level "
           << std::to_string(ob.get_tech()) << ")\n"
           << "Cost: " << std::to_string(ob.get_cost()) << " BP";
        return os;
    }

    // clang-format off
    void set_code(std::string c) { code = c; }
    void set_name(std::string n) { name = n; }
    void set_tech(int t) { attr.tech = t; }
    void set_type(char t) { attr.type = t; }
    void set_PD(int pd) { attr.PD = pd; }
    void set_Phasic(int b) { attr.Phasic = b; }
    void set_Shield(int s) { attr.Shield = s; }
    void set_Launcher(int t) { attr.Launcher = t; }
    void set_Torpedo(int m) { attr.Torpedo = m; }
    void set_Hanger(int hangar) { attr.Hangar = hangar; }
    void set_LS(int lrs) { attr.LS = lrs; }

    std::string get_code() const { return std::string(code); }
    std::string get_name() const { return std::string(name); }
    int get_tech() const { return attr.tech; }
    char get_type() const { return attr.type; }
    int get_PD() const { return attr.PD; }
    int get_Phasic() const { return attr.Phasic; }
    int get_Shield() const { return attr.Shield; }
    int get_Launcher() const { return attr.Launcher; }
    int get_Torpedo() const { return attr.Torpedo; }
    int get_Hanger() const { return attr.Hangar; }
    int get_LS() const { return attr.LS; }
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

#endif
