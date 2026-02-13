///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_MILIEU_AGENT_H__
#define __KH_MILIEU_AGENT_H__

#include <string>
#include <variant>

#include "extract_strategy.h"
#include "fabricate_strategy.h"
#include "market_modes.h"
#include "outfit_modes.h"
#include "survey_strategy.h"
#include "system_strategy.h"
#include "trade_modes.h"
#include "trade_strategy.h"

class MilieuParam
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
        Builder& set_player(char _player)
        {
            player = _player;
            return *this;
        }
        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        MilieuParam build() const
        {
            return MilieuParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };

    MilieuParam(int gid, int mid, char user)
        : m_gid(gid), m_mid(mid), m_user(user)
    {
    }

    MilieuParam() : m_mid(0), m_gid(0), m_user(0)
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
    int m_gid;
    int m_mid;
    char m_user;
};

//////////////////////////////////////////////////////////////////////////
// TradeParam
//
///////////////

class TradeParam : public MilieuParam
{
  public:
    TradeParam(int gid, int mid, char user, const TradeMode& mode,
               const std::string& resource, const int qty,
               const std::string& srcship, const std::string& destship)
        : MilieuParam(gid, mid, user), m_mode(mode), m_resource(resource),
          m_srcship(srcship), m_destship(destship), m_qty(qty)
    {
    }
    TradeParam() : MilieuParam()
    {
        m_mode = TradeMode::TRADE_LIST;
        m_qty = 0;
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_list_mode()
        {
            mode = TradeMode::TRADE_LIST;
            return *this;
        }
        Builder& set_buy_mode()
        {
            mode = TradeMode::TRADE_BUY;
            return *this;
        }
        Builder& set_sell_mode()
        {
            mode = TradeMode::TRADE_SELL;
            return *this;
        }
        Builder& set_transfer_mode()
        {
            mode = TradeMode::TRADE_TRANSFER;
            return *this;
        }

        Builder& set_resource(const std::string& _resource)
        {
            resource = _resource;
            return *this;
        }
        Builder& set_srcship(const std::string& _srcship)
        {
            srcship = _srcship;
            return *this;
        }
        Builder& set_destship(const std::string& _destship)
        {
            destship = _destship;
            return *this;
        }
        Builder& set_qty(const int _qty)
        {
            qty = _qty;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            qty = 0;
        }

        TradeParam build() const
        {
            return TradeParam(gid, mid, player, mode, resource, qty, srcship,
                              destship);
        }

      protected:
        int mid;
        int gid;
        char player;
        TradeMode mode;
        std::string resource;
        std::string srcship;
        std::string destship;
        int qty;
    };

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

    TradeMode get_mode() const
    {
        return m_mode;
    }
    std::string get_resource() const
    {
        return m_resource;
    }
    std::string get_src_ship() const
    {
        return m_srcship;
    }
    std::string get_dest_ship() const
    {
        return m_destship;
    }
    int get_qty() const
    {
        return m_qty;
    }

  protected:
    TradeMode m_mode;
    std::string m_resource;
    std::string m_srcship;
    std::string m_destship;
    int m_qty;
};

//////////////////////////////////////////////////////////////////////////
// CargoParam
//
///////////////

class CargoParam : public MilieuParam
{
  public:
    CargoParam(int gid, int mid, char user, const std::string& ship_code,
               const std::string& target)
        : MilieuParam(gid, mid, user), m_ship_code(ship_code), m_target(target)
    {
    }
    CargoParam() : MilieuParam()
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_ship_code(const std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }
        Builder& set_target(const std::string& _target)
        {
            target = _target;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        CargoParam build() const
        {
            return CargoParam(gid, mid, player, ship_code, target);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string ship_code;
        std::string target;
    };

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

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_ship_code;
    std::string m_target;
};

//////////////////////////////////////////////////////////////////////////
// ExtractParam
//
///////////////

class ExtractParam : public MilieuParam
{
  public:
    ExtractParam(int gid, int mid, char user, const std::string& ship_code,
                 const std::string& resource)
        : MilieuParam(gid, mid, user), m_mode(ExtractMode::EXT_SCAN),
          m_ship_code(ship_code), m_resource(resource)
    {
        if (!resource.empty() && !ship_code.empty())
        {
            m_mode = ExtractMode::EXT_EXTRACT;
        }
        else
        {
            m_mode = ExtractMode::EXT_SCAN;
        }
    }
    ExtractParam() : MilieuParam(), m_mode(ExtractMode::EXT_SCAN)
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_ship_code(const std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }
        Builder& set_resource(const std::string& _resource)
        {
            resource = _resource;
            return *this;
        }
        Builder& set_scan_mode(void)
        {
            mode = ExtractMode::EXT_SCAN;
            return *this;
        }
        Builder& set_extract_mode(void)
        {
            mode = ExtractMode::EXT_EXTRACT;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            mode = ExtractMode::EXT_SCAN;
        }

        ExtractParam build() const
        {
            return ExtractParam(gid, mid, player, ship_code, resource);
        }

      protected:
        int mid;
        int gid;
        char player;
        ExtractMode mode;
        std::string ship_code;
        std::string resource;
    };

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

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    std::string get_resource() const
    {
        return m_resource;
    }
    bool get_scan_mode() const
    {
        return (KH_EQU(m_mode, ExtractMode::EXT_SCAN));
    }
    bool get_extract_mode() const
    {
        return (KH_EQU(m_mode, ExtractMode::EXT_EXTRACT));
    }

  protected:
    std::string m_ship_code;
    std::string m_resource;
    ExtractMode m_mode;
};

//////////////////////////////////////////////////////////////////////////
// FabricateParam
//
///////////////

class FabricateParam : public MilieuParam
{
  public:
    FabricateParam(int gid, int mid, char user, FabricateMode mode, int qty)
        : MilieuParam(gid, mid, user), m_mode(mode), m_qty(qty)
    {
    }
    FabricateParam()
        : MilieuParam(), m_mode(FabricateMode::LIST_PLANS), m_qty(0)
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_mode(FabricateMode _mode)
        {
            mode = _mode;
            return *this;
        }
        Builder& set_qty(int _qty)
        {
            qty = _qty;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            mode = FabricateMode::LIST_PLANS;
            qty = 0;
        }

        FabricateParam build() const
        {
            return FabricateParam(gid, mid, player, mode, qty);
        }

      protected:
        int mid;
        int gid;
        char player;
        FabricateMode mode;
        int qty;
    };

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

    FabricateMode get_mode() const
    {
        return m_mode;
    }

    int get_qty() const
    {
        return m_qty;
    }

  protected:
    FabricateMode m_mode;
    int m_qty;
};

//////////////////////////////////////////////////////////////////////////
// FacilitiesParam
//
///////////////

class FacilitiesParam : public MilieuParam
{
  public:
    FacilitiesParam(int gid, int mid, char user, const std::string& ship_code,
                    const std::string& target)
        : MilieuParam(gid, mid, user), m_ship_code(ship_code), m_target(target)
    {
    }
    FacilitiesParam() : MilieuParam()
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_ship_code(const std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }
        Builder& set_target(const std::string& _target)
        {
            target = _target;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        FacilitiesParam build() const
        {
            return FacilitiesParam(gid, mid, player, ship_code, target);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string ship_code;
        std::string target;
    };

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

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_ship_code;
    std::string m_target;
};

//////////////////////////////////////////////////////////////////////////
// GalaxyParam
//
///////////////

class GalaxyParam : public MilieuParam
{
  public:
    GalaxyParam(int gid, int mid, char user) : MilieuParam(gid, mid, user)
    {
    }
    GalaxyParam() : MilieuParam()
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        GalaxyParam build() const
        {
            return GalaxyParam(gid, mid, player);
        }

      protected:
        int mid;
        int gid;
        char player;
    };

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
};

//////////////////////////////////////////////////////////////////////////
// MarketParam
//
///////////////

class MarketParam : public MilieuParam
{
  public:
    MarketParam(int gid, int mid, char user, MarketMode mode,
                const std::string& resource)
        : MilieuParam(gid, mid, user), m_mode(mode), m_resource(resource)
    {
    }
    MarketParam() : MilieuParam(), m_mode(MarketMode::MARKET_LIST_PRICES)
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_mode(MarketMode _mode)
        {
            mode = _mode;
            return *this;
        }
        Builder& set_resource(const std::string& _resource)
        {
            resource = _resource;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            mode = MarketMode::MARKET_LIST_PRICES;
        }

        MarketParam build() const
        {
            return MarketParam(gid, mid, player, mode, resource);
        }

      protected:
        int mid;
        int gid;
        char player;
        MarketMode mode;
        std::string resource;
    };

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

    MarketMode get_mode() const
    {
        return m_mode;
    }

    std::string get_resource() const
    {
        return m_resource;
    }

  protected:
    MarketMode m_mode;
    std::string m_resource;
};

//////////////////////////////////////////////////////////////////////////
// OutfitParam
//
///////////////

class OutfitParam : public MilieuParam
{
  public:
    OutfitParam(int gid, int mid, char user, OutfitMode mode,
                const std::string& ship_code)
        : MilieuParam(gid, mid, user), m_mode(mode), m_ship_code(ship_code)
    {
    }
    OutfitParam() : MilieuParam(), m_mode(OutfitMode::OUTFIT_LIST)
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_mode(OutfitMode _mode)
        {
            mode = _mode;
            return *this;
        }
        Builder& set_ship_code(const std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            mode = OutfitMode::OUTFIT_LIST;
        }

        OutfitParam build() const
        {
            return OutfitParam(gid, mid, player, mode, ship_code);
        }

      protected:
        int mid;
        int gid;
        char player;
        OutfitMode mode;
        std::string ship_code;
    };

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

    OutfitMode get_mode() const
    {
        return m_mode;
    }

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

  protected:
    OutfitMode m_mode;
    std::string m_ship_code;
};

//////////////////////////////////////////////////////////////////////////
// SalvageParam
//
///////////////

class SalvageParam : public MilieuParam
{
  public:
    SalvageParam(int gid, int mid, char user, const std::string& ship_code,
                 const std::string& target)
        : MilieuParam(gid, mid, user), m_ship_code(ship_code), m_target(target)
    {
    }
    SalvageParam() : MilieuParam()
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_ship_code(const std::string& _ship_code)
        {
            ship_code = _ship_code;
            return *this;
        }
        Builder& set_target(const std::string& _target)
        {
            target = _target;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        SalvageParam build() const
        {
            return SalvageParam(gid, mid, player, ship_code, target);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string ship_code;
        std::string target;
    };

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

    std::string get_ship_code() const
    {
        return m_ship_code;
    }

    std::string get_target() const
    {
        return m_target;
    }

  protected:
    std::string m_ship_code;
    std::string m_target;
};

//////////////////////////////////////////////////////////////////////////
// SurveyParam
//
///////////////

class SurveyParam : public MilieuParam
{
  public:
    SurveyParam(int gid, int mid, char user, const std::string& system_name,
                const SurveyMode& mode)
        : MilieuParam(gid, mid, user), m_system_name(system_name), m_mode(mode)

    {
    }
    SurveyParam() : MilieuParam()
    {
        m_mode = SurveyMode::SURV_NONE;
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_system_name(const std::string& _name)
        {
            system_name = _name;
            return *this;
        }
        Builder& set_mode(const SurveyMode& _mode)
        {
            mode = _mode;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
        }

        SurveyParam build() const
        {
            return SurveyParam(gid, mid, player, system_name, mode);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string system_name;
        SurveyMode mode;
    };

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

    std::string get_system_name() const
    {
        return m_system_name;
    }

    SurveyMode get_mode() const
    {
        return m_mode;
    }

  protected:
    std::string m_system_name;
    SurveyMode m_mode;
};

//////////////////////////////////////////////////////////////////////////
// SystemParam
//
///////////////

class SystemParam : public MilieuParam
{
  public:
    SystemParam(int gid, int mid, char user, const std::string& name,
                const SystemMode& mode)
        : MilieuParam(gid, mid, user), m_system_name(name), m_mode(mode)
    {
    }

    SystemParam() : MilieuParam()
    {
        m_mode = SystemMode::SYS_SHOW_SYSTEM;
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
        Builder& set_player(int _player)
        {
            player = _player;
            return *this;
        }
        Builder& set_system_name(const std::string& _name)
        {
            system_name = _name;
            return *this;
        }
        Builder& set_mode(const SystemMode& _mode)
        {
            mode = _mode;
            return *this;
        }

        Builder()
        {
            mid = 0;
            gid = 0;
            player = 0;
            mode = SystemMode::SYS_SHOW_SYSTEM;
        }

        SystemParam build() const
        {
            return SystemParam(gid, mid, player, system_name, mode);
        }

      protected:
        int mid;
        int gid;
        char player;
        std::string system_name;
        SystemMode mode;
    };

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

    std::string get_system_name(void) const
    {
        return m_system_name;
    }
    SystemMode get_mode(void) const
    {
        return m_mode;
    }

  protected:
    std::string m_system_name;
    SystemMode m_mode;
};

using MilieuAgentParam =
    std::variant<TradeParam, CargoParam, ExtractParam, FabricateParam,
                 FacilitiesParam, GalaxyParam, MarketParam, OutfitParam,
                 SalvageParam, SurveyParam, SystemParam>;

class MilieuAgent
{
  public:
    static MilieuAgent& instance()
    {
        static MilieuAgent _instance;
        return _instance;
    }

    // Deleted copy constructor and assignment operator
    MilieuAgent(const MilieuAgent&) = delete;
    MilieuAgent& operator=(const MilieuAgent&) = delete;

    bool apply(MilieuAgentParam& param);
    bool apply(TradeParam& param);
    bool apply(CargoParam& param);
    bool apply(ExtractParam& param);
    bool apply(FabricateParam& param);
    bool apply(FacilitiesParam& param);
    bool apply(GalaxyParam& param);
    bool apply(MarketParam& param);
    bool apply(OutfitParam& param);
    bool apply(SalvageParam& param);
    bool apply(SurveyParam& param);
    bool apply(SystemParam& param);

  private:
    MilieuAgent() = default;
    ~MilieuAgent() = default;
};

#endif
