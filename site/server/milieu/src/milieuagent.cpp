///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "milieuagent.h"

#include <iomanip>
#include <sstream>

#include "db.h"
#include "extract_strategy.h"
#include "fabricate_strategy.h"
#include "logger.h"
#include "market_strategy.h"
#include "moduleutil.h"
#include "outfit_modes.h"
#include "outfit_strategy.h"
#include "statemachine.h"
#include "survey_strategy.h"
#include "system_strategy.h"
#include "telemetry.h"
#include "trade_strategy.h"

// Main dispatch apply - routes to specific apply methods
bool MilieuAgent::apply(MilieuAgentParam& param)
{
    return std::visit(
        [this](auto&& arg) -> bool
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, MilieuParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, TradeParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, CargoParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, ExtractParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, FabricateParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, FacilitiesParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, GalaxyParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, MarketParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, OutfitParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, SalvageParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, SurveyParam>)
            {
                return this->apply(arg);
            }
            else if constexpr (std::is_same_v<T, SystemParam>)
            {
                return this->apply(arg);
            }
            return false;
        },
        param);
}

bool MilieuAgent::apply(TradeParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();
    int game_id = param.get_game_id();

    // Mode is list, buy, sell, transfer
    TradeMode mode = param.get_mode();

    // TradeActor - Buy/sell resources at trade hubs
    // Usage: trade list
    //        trade buy <resource> <qty>
    //        trade sell <resource> <qty>
    //        trade transfer <ship1> <ship2> <resource> <qty>

    // Check inhibits
    std::string inhibit_error;
    if (!StateMachine::instance().check_inhibits(CommandID::TRADE,
                                                 inhibit_error))
    {
        Telemetry::instance().write("Error: " + inhibit_error);
        bres = false;
    }
    else
    {
        int qty = 0;
        if (mode > TradeMode::TRADE_LIST && mode < TradeMode::LAST_TRADE_MODE)
        {
            qty = param.get_qty();
        }

        switch (mode)
        {
        case TradeMode::TRADE_LIST:
        {
            // only need mode 'list'
            bres = TradeStrategy::show_prices();
            break;
        }
        case TradeMode::TRADE_BUY:
        {
            // need 'resource' and 'qty'
            std::string resource = param.get_resource();
            bres = TradeStrategy::do_buy(resource, qty);
            break;
        }
        case TradeMode::TRADE_SELL:
        {
            // need 'resource' and 'qty'
            std::string resource = param.get_resource();
            bres = TradeStrategy::do_sell(resource, qty);
            break;
        }
        case TradeMode::TRADE_TRANSFER:
            // need ship1, ship2, resource, qty
            std::string srcship = param.get_src_ship();
            std::string destship = param.get_dest_ship();
            std::string resource = param.get_resource();
            bres = TradeStrategy::do_transfer(resource, qty, srcship, destship);
            break;
        }
    }

    return bres;
}

bool MilieuAgent::apply(CargoParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    char owner = StateMachine::instance().get_current_player();

    int game_id = param.get_game_id();
    char player = param.get_player();

    std::string ship_code = param.get_ship_code();
    std::string target = param.get_target();

    // Query ship with cargo info
    std::string q =
        "SELECT ship_name, pd, hangar, "
        " cargo_ferrous, cargo_rare_earth, cargo_radioactive, "
        "cargo_crystalline, "
        " cargo_volatile, cargo_water, cargo_organic, cargo_exotic, "
        "cargo_torpedoes "
        " FROM ships WHERE game_id=?  AND owner=? "
        " AND ( ship_code=? OR ship_name=?) AND destroyed_at IS NULL";

    auto rows = db.Query(q, {s.game_id, owner, ship_code, ship_code});

    if (rows.empty())
    {
        Telemetry::instance().write(
            std::format(LC_TARGET_SHIP_NOT_FOUND, ship_code));
        bres = false;
    }
    else
    {
        std::string name = rows[0][0];
        int pd = std::atoi(rows[0][1].c_str());
        int hangar = std::atoi(rows[0][2].c_str());

        // Cargo values
        int cargo[9];
        for (int i = 0; i < 9; i++)
        {
            cargo[i] = std::atoi(rows[0][3 + i].c_str());
        }

        // Capacity formula: 2*PD + H (minimum 1)
        int capacity = 2 * pd + hangar;
        if (capacity < 1)
        {
            capacity = 1;
        }
        int total_cargo = 0;
        for (int i = 0; i < 8; i++)
        {
            total_cargo += cargo[i]; // torpedoes don't count against capacity
        }

        std::ostringstream out;
        out << "=== " << name << " (" << ship_code << ") CARGO MANIFEST ===\n"
            << "Capacity: " << total_cargo << "/" << capacity;

        if (total_cargo >= capacity)
        {
            out << " (FULL)";
        }
        out << "\n";

        const char* names[] = {"Ferrous",     "Rare Earth", "Radioactive",
                               "Crystalline", "Volatile",   "Water",
                               "Organic",     "Exotic",     "Torpedoes"};

        for (int i = 0; i < 9; i++)
        {
            if (cargo[i] > 0)
            {
                out << "  " << names[i] << ": " << cargo[i] << "\n";
            }
        }

        if (KH_EQU(total_cargo, 0) && KH_EQU(cargo[8], 0))
        {
            out << "  (empty)\n";
        }

        Telemetry::instance().write(out.str());
    }
    return bres;
}

bool MilieuAgent::apply(ExtractParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();

    if (param.get_scan_mode())
    {
        bres = ExtractStrategy::do_scan();
    }
    else if (param.get_extract_mode())
    {
        bres = ExtractStrategy::do_extract(param.get_ship_code(),
                                           param.get_resource());
    }
    return bres;
}

bool MilieuAgent::apply(FabricateParam& param)
{
    bool bres = false;
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    int module_id = param.get_module_id();
    char owner = param.get_player();
    FabricateMode mode = param.get_mode();
    int qty = param.get_qty();

    switch (mode)
    {
    case FabricateMode::LIST_PLANS:
        bres = FabricateStrategy::show_plans(game_id, module_id);
        break;
    case FabricateMode::FABRICATE_TORPEDO:
        bres = FabricateStrategy::fabricate_torpedo(game_id, owner, qty);
        break;
    case FabricateMode::FABRICATE_LAUNCHER:
        bres = FabricateStrategy::fabricate_launcher(game_id, owner, qty);
        break;
    case FabricateMode::FABRICATE_PHASIC:
        bres = FabricateStrategy::fabricate_phasic(game_id, owner, qty);
        break;
    case FabricateMode::FABRICATE_SHIELD:
        bres = FabricateStrategy::fabricate_shield(game_id, owner, qty);
        break;
    case FabricateMode::FABRICATE_TECH:
        bres = FabricateStrategy::fabricate_tech(game_id, owner, qty);
        break;
    default:
        Telemetry::instance().write(LC_MILIEU_UNKNOWN_FABRICATE_MODE);
        break;
    }

    return bres;
}

bool MilieuAgent::apply(FacilitiesParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string ship_code = param.get_ship_code();
    std::string target = param.get_target();

    return bres;
}

bool MilieuAgent::apply(GalaxyParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    char owner = param.get_player();

    std::ostringstream out;
    out << "         GALAXY OVERVIEW\n";
    out << "=========================================================\n";
    out << " System          Hex     Base   Ships A  Ships B\n";
    out << "---------------------------------------------------------\n";

    // Get all systems
    std::string q =
        "SELECT name, hex_id, is_base, base_owner FROM star_systems "
        "WHERE module_id=1 ORDER BY name";

    auto systems = db.Query(q, {});

    for (const auto& sys : systems)
    {
        std::string name = sys[0];
        std::string hex = sys[1];
        bool isBase = KH_EQU(sys[2], "1");
        std::string owner = sys[3];

        // Count ships at this hex
        std::string q = "SELECT COUNT(*) FROM ships WHERE game_id=? "
                        " AND at_hex=? AND owner=? AND destroyed_at IS NULL";

        auto countA = db.Query(q, {game_id, hex, 'A'});
        auto countB = db.Query(q, {game_id, hex, 'B'});

        int shipsA = countA.empty() ? 0 : std::atoi(countA[0][0].c_str());
        int shipsB = countB.empty() ? 0 : std::atoi(countB[0][0].c_str());

        // Pad name to 15 chars
        std::string paddedName = name;
        if (paddedName.length() < 15)
        {
            paddedName.append(15 - paddedName.length(), ' ');
        }
        else
        {
            paddedName = paddedName.substr(0, 15);
        }

        out << " " << paddedName << " " << hex;
        if (isBase)
        {
            out << "   [" << owner << "]  ";
        }
        else
        {
            out << "        ";
        }
        out << std::setw(8) << shipsA << " " << std::setw(8) << shipsB << "\n";
    }

    out << "=========================================================\n";
    Telemetry::instance().write(out.str());
    bres = true;

    return bres;
}

bool MilieuAgent::apply(MarketParam& param)
{
    bool bres = false;
    int game_id = param.get_game_id();
    MarketMode mode = param.get_mode();

    switch (mode)
    {
    case MarketMode::MARKET_LIST_PRICES:
        bres = MarketStrategy::show_all_prices(game_id);
        break;
    case MarketMode::MARKET_PRICE_HISTORY:
        bres =
            MarketStrategy::show_price_history(game_id, param.get_resource());
        break;
    }

    return bres;
}

bool MilieuAgent::apply(OutfitParam& param)
{
    bool bres = false;
    int game_id = param.get_game_id();
    char owner = param.get_player();
    OutfitMode mode = param.get_mode();
    std::string ship_code = param.get_ship_code();

    switch (mode)
    {
    case OutfitMode::OUTFIT_LIST:
        bres = OutfitStrategy::show_equipment();
        break;
    case OutfitMode::OUTFIT_LRS:
        bres = OutfitStrategy::outfit_lrs(game_id, owner, ship_code);
        break;
    }

    return bres;
}

bool MilieuAgent::apply(SurveyParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string target_system = param.get_system_name();
    SurveyMode survey_mode = param.get_mode();

    // BIGBUG All of the caseimpl needs to go into the strategy
    switch (survey_mode)
    {
    // BIGBUG no difference yet??
    case SurveyMode::SURV_NONE:
    case SurveyMode::SURV_BASIC:
    case SurveyMode::SURV_ENHANCED:
    {
        if (target_system.empty())
        {
            // Find first system where player has a ship
            auto loc_rows =
                db.Query("SELECT DISTINCT ss.name FROM ships s "
                         "JOIN star_systems ss ON s.at_hex = ss.hex_id "
                         "WHERE s.game_id=? AND s.owner=? "
                         "AND s.destroyed_at IS NULL LIMIT 1",
                         {s.game_id, owner});

            if (loc_rows.empty())
            {
                Telemetry::instance().write(
                    LC_MILIEU_NO_SHIPS_FOR_SURVEY);
                return false;
            }
            target_system = loc_rows[0][0];
        }
        // Validate system exists
        auto check =
            db.Query("SELECT name FROM star_systems WHERE UPPER(name)=UPPER(?)",
                     {target_system});
        if (check.empty())
        {
            Telemetry::instance().write(
                std::format(LC_MILIEU_SURVEY_UNKNOWN_SYSTEM,
                           target_system));

            // BUGBUG - Not liking that we return early..
            return false;
        }
        target_system = check[0][0];

        // Check ship presence
        if (!SurveyStrategy::has_ship_in_system(target_system))
        {
            Telemetry::instance().write(
               std::format(LC_MILIEU_SURVEY_NO_SHIPS_PRESENT, target_system));
            Telemetry::instance().write(LC_MILIEU_SURVEY_SHIPS_REQUIRED);

            // BUGBUG - that we return early
            return false;
        }

        // Get current knowledge level
        auto know_rows =
            db.Query("SELECT knowledge_level FROM codex_entries "
                     "WHERE game_id=? AND player=? AND system_name=?",
                     {s.game_id, owner, target_system});

        std::string current_level = "Unknown";
        if (!know_rows.empty())
        {
            current_level = know_rows[0][0];
        }

        std::string new_level =
            SurveyStrategy::upgrade_knowledge(current_level);

        if (KH_EQU(new_level, current_level))
        {
            Telemetry::instance().write(
              std::format(LC_MILIEU_SURVEY_MAX_INFO, target_system));
            Telemetry::instance().write(LC_MILIEU_SURVEY_MAX_SECRETS);
            return true;
        }

        // Update or insert codex entry
        if (know_rows.empty())
        {
            db.Exec("INSERT INTO codex_entries (game_id, player, system_name, "
                    "knowledge_level, last_updated_turn) VALUES (?,?,?,?,?)",
                    {s.game_id, owner, target_system, new_level, s.round});
        }
        else
        {
            db.Exec("UPDATE codex_entries SET knowledge_level=?, "
                    "last_updated_turn=? WHERE game_id=? AND player=? "
                    "AND system_name=?",
                    {new_level, s.round, s.game_id, owner, target_system});
        }

        // Report success
        std::ostringstream out;
        out << std::format(LC_MILIEU_TARGET_SURVEY_COMPLETE, target_system)
            << "\n"
            << std::format(LC_MILIEU_TARGET_INFO_UPGRADED, current_level,
                       new_level)
            << "\n\n";

        // Show what's newly available
        if (KH_EQU(new_level, "Charted"))
        {
            out << LC_MILIEU_SURVEY_CHARTED_LEVEL
                << "\n"
                << std::format(LC_MILIEU_SURVEY_TARGET_MORE, target_system)
                << "\n";
        }
        else if (KH_EQU(new_level, "Surveyed"))
        {
            out << LC_MILIEU_SURVEY_DETAILED_LEVEL
                << "\n"
                << std::format(LC_MILIEU_SURVEY_TARGET_RESOURCES, target_system)
                << "\n";
        }
        else if (KH_EQU(new_level, "Intimate"))
        {
            out << LC_MILIEU_SURVEY_INTIMATE_LEVEL
                << "\n"
                << std::format(LC_MILIEU_SURVEY_TARGET_ANOMALIES, target_system)
                << "\n";
        }
        Telemetry::instance().write(out.str());
        bres = true;
        break;
    }
    }
    return bres;
}

bool MilieuAgent::apply(SalvageParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string ship_code = param.get_ship_code();

    return bres;
}

bool MilieuAgent::apply(SystemParam& param)
{
    bool bres = false;
    DatabaseManager& db = DatabaseManager::instance();
    GameState s = StateMachine::instance().get_game_state();
    int game_id = param.get_game_id();
    char owner = param.get_player();
    std::string system_name = param.get_system_name();
    SystemMode mode = param.get_mode();

    switch (mode)
    {
    case SystemMode::SYS_SHOW_SYSTEM:
        bres = SystemStrategy::show_overview(system_name);
        break;
    case SystemMode::SYS_PLANETS:
        bres = SystemStrategy::show_planets(system_name);
        break;
    case SystemMode::SYS_RESOURCES:
        bres = SystemStrategy::show_resources(system_name);
        break;
    case SystemMode::SYS_FACILITIES:
        bres = SystemStrategy::show_facilities(system_name);
        break;
    case SystemMode::SYS_ANOMALIES:
        bres = SystemStrategy::show_anomalies(system_name);
        break;
    }

    return bres;
}
