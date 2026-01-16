#include "combatagent.h"

bool CombatAgent::find_combat_session(CombatSession& session, int gid, int mid,
                                      char attacker_id, char attackee_id)
{
    bool bresult = false;
    auto it = std::find_if(
        m_combats.begin(), m_combats.end(),
        [gid, mid, attacker_id, attackee_id](const CombatSession& element) {
            return element.game_id == gid && element.module_id == mid &&
                   element.players.first == attacker_id &&
                   element.players.second == attackee_id;
        });

    // Check if the element was found
    if (it != m_combats.end())
    {
        std::cout << "Element found at index: "
                  << std::distance(m_combats.begin(), it) << std::endl;
        // std::cout << "Properties: a=" << it->a << ", b=" << it->b << ", c="
        // << it->c << ", d=" << (it->d ? "true" : "false") << std::endl;
        bresult = true;
    }
    else
    {
        std::cout << "Element not found in the vector." << std::endl;
        bresult = false;
    }
    session = *it;
    return bresult;
}

bool CombatAgent::is_param_valid(const CombatAgentPayload& param) const
{
    bool bresult = false;
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, CombatParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatOrderParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatApplyParam>)
            {
                bresult = true;
            }
            else if constexpr (std::is_same_v<T, CombatRetreatParam>)
            {
                bresult = true;
            }
        },
        param);

    return bresult;
}

bool CombatAgent::apply(CombatParam& param)
{
    return false;
}

bool CombatAgent::apply(CombatOrderParam& param)
{

    bool bresult = false;
    if (!is_param_valid(param))
    {
        bresult = false;
        return bresult;
    }

    int gid = param.get_game_id();
    int mid = param.get_module_id();
    char player = param.get_player();

    FightingPlayers combatting_players{'A', 'B'};
    if (player == 'B')
    {
        combatting_players = {'B', 'A'};
    }

#if 0
    CombatSession session;
    bool locate_combat = find_combat_session(
        session, gid, mid, combatting_players.first, combatting_players.second);

    if (!locate_combat)
    {
        // there is no combat between these players in session
        // We need to establish this??
        m_combats.push_back(session);
    }
#endif

    std::string attacker(param.get_attacker());
    std::string attackee(param.get_attackee());
    AttributeMap combat_attr = param.get_attr();
    CombatOpTactic tactic = param.get_tactic();
    MissileSet missiles_fired = param.get_missiles();


    /////////////////////////////////////////////////////////
    // 1.
    // Validate Ship Ownership of the Attacker
    // Validate Ship Ownership of the Attackee
    /////////////////////////////////////////////////////////

    std::string pick_attacker;
    pick_attacker.append("SELECT S.owner FROM ships S WHERE S.game_id =");
    pick_attacker.append( std::to_string(gid) );
    pick_attacker.append(" AND S.destroyed_at is NULL ");
    pick_attacker.append(" AND S.owner='");
    pick_attacker += combatting_players.first;
    pick_attacker.append("' ");
    pick_attacker.append(" AND ( S.ship_code='");
    pick_attacker.append( db_esc(attacker) );
    pick_attacker.append("'" OR S.ship_name ='");
    pick_attacker.append( db_esc(attacker) );
    pick_attacker.append("')");

    auto attacker_owner = db.query(pick_attacker);

    if (attacker_owner.empty()) {
        // BUGBUG
        // ship is not found
        return false;
    }
    std::string pick_attackee;
    pick_attackee.append("SELECT S.owner FROM ships S WHERE S.game_id =");
    pick_attackee.append( std::to_string(gid) );
    pick_attackee.append(" AND S.destroyed_at is NULL ");
    pick_attackee.append(" AND S.owner='");
    pick_attackee += combatting_players.second;
    pick_attackee.append("' ");
    pick_attackee.append(" AND ( S.ship_code='");
    pick_attackee.append( db_esc(attackee) );
    pick_attackee.append("'" OR S.ship_name ='");
    pick_attackee.append( db_esc(attackee) );
    pick_attackee.append("')");

    auto attacker_attackee = db.query(pick_attackee);

    if (attackee_owner.empty()) {
        // BUGBUG
        // ship is not found
        return false;
    }

    /////////////////////////////////////////////////////////
    // 2.
    // Validate combat state and stats
    ////////////////////////////////////////////////////////
    std::string query_stats_attacker;
    query_stats_attacker.append(SELECT at_hex, pd, beam, screen, tube, missiles, sr ");
    query_stats_attacker.append(" FROM ships WHERE game_id=");
    query_stats_attacker.append(std::to_string(gid));
    query_stats_attacker.append(" AND destroyed_at is NULL ");
    query_stats_attacker.append(" AND ( ship_code='");
    query_stats_attacker.append(db.esc(attacker));
    query_stats_attacker.append("' OR ship_name='");
    query_stats_attacker.append(db.esc(attacker));
    query_stats_attacker.append("'");
   
    auto attacker_stats = db.query(query_stats_attacker.c_str());

    if (attacker_stats.empty() || attacker_stats[0][0].empty())
    {
        // BUGBUG
        // didn't find the ship?   How can this be?
        // return "Ship not in space";
        return false;
    }

#if 0
enum class AttributeID : int
{
    POWER_DRIVE,
    BEAM,
    SCREEN,
    TUBE,
    MISSILE,
    SYSTEM_RACK
};
typedef int AttributeValue;
typedef std::map<AttributeID, AttributeValue> AttributeMap;
typedef std::pair<int, int> FiringMissile;
typedef std::vector<FiringMissile> MissileSet;
#endif

    std::string combat_hex_id = attacker_stats[0][0];
    AttributeMap max_attack_attr;
    max_attack_attr[AttributeID::POWER_DRIVE] = std::atoi(attacker_stats[0][1].c_str());
    max_attack_attr[AttributeID::BEAM] = std::atoi(attacker_stats[0][2].c_str());
    max_attack_attr[AttributeID::SCREEN] = std::atoi(attacker_stats[0][3].c_str());
    max_attack_attr[AttributeID::TUBE] = std::atoi(attacker_stats[0][4].c_str());
    max_attack_attr[AttributeID::MISSILE] = std::atoi(attacker_stats[0][5].c_str());
    max_attack_attr[AttributeID::SYSTEM_RACK] = std::atoi(attacker_stats[0][6].c_str());

    ////////////////////////////////
    // 2.1
    // Validate Power Limits
    ///////////////////////////////




}

void CombatAgent::evolve_combat(void)
{
    for (std::vector<CombatSession>::iterator itr = m_combats.begin();
         itr != m_combats.end(); ++itr)
    {
        CombatSession session = *itr;
    }
}

bool CombatAgent::apply(CombatApplyParam& param)
{
}
bool CombatAgent::apply(CombatRetreatParam& param)
{
}
bool CombatAgent::apply(CombatCommitParam& param)
{
}
bool CombatAgent::apply(CombatCancelParam& param)
{
}
bool CombatAgent::apply(CombatDraftsParam& param)
{
}
bool CombatAgent::apply(CombatStatusParam& param)
{
}
