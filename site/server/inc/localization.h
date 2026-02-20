///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_LOCALISATION_H__
#define __KH_LOCALISATION_H__


#define LC_PARSER_ROOT_ERROR "Unrecognized command. Type 'help' for a list of commands."
#define LC_CONFIGURE_PREFIX(x) "CONFIGURE: " x
#define LC_CONFIGURE_ACCESS_DENIED                                             \
    LC_CONFIGURE_PREFIX("Access denied. Administrator privilege required.")
#define LC_CONFIGURE_BANNER LC_CONFIGURE_PREFIX("Current Settings")
#define LC_CONFIGURE_SUBCMDS "Subcommands:"
#define LC_CONFIGURE_RELOAD_CONF "  configure reload conf  — Reload kh.conf"
#define LC_CONFIGURE_AI_RELOAD "  configure reload ai    — Reload AI DSL"
#define LC_CONFIGURE_LOADED LC_CONFIGURE_PREFIX("Loaded {} settings.")
#define LC_CONFIGURE_CANNOT_LOAD                                               \
    LC_CONFIGURE_PREFIX("Cannot open kh.conf. File not found in server directory.")
#define LC_CONFIGURE_DSL_LOADING LC_CONFIGURE_PREFIX("Reloading AI DSL...")
#define LC_CONFIGURE_DSL_LOADED LC_CONFIGURE_PREFIX("AI DSL reloaded.")


#define LC_SAVE_PREFIX(x) "SAVE: " x
#define LC_LOAD_PREFIX(x) "LOAD: " x
#define LC_DELETE_PREFIX(x) "DELETE: " x
#define LC_COMMAND_PREFIX(x) "COMMAND: " x
#define LC_SAVE_CMD_NO_NAME                                                    \
    LC_DELETE_PREFIX("No save name specified.\nUsage: delete <save_name>")
#define LC_SAVE_CMD_NO_SAVED_GAME LC_DELETE_PREFIX("No saved game '{}' found.")
#define LC_SAVE_CMD_DELETED_GAME LC_DELETE_PREFIX("game '{}' deleted.")
#define LC_SAVE_CMD_HINT                                                       \
    "Usage: save <NAME> - save current game under label NAME"
#define LC_SAVE_FAILED LC_SAVE_PREFIX("Failed to snapshot game state.")
#define LC_SAVE_UPDATE_TARGET_SAVED LC_SAVE_PREFIX("Updated '{}' (Turn {}).")
#define LC_SAVE_NEW_TARGET_SAVED LC_SAVE_PREFIX("Saved '{}' (Turn {}).")
#define LC_LOAD_NOT_FOUND LC_LOAD_PREFIX("No saved games found.")
#define LC_LOAD_LIST_BANNER LC_LOAD_PREFIX("Your saved games")
#define LC_LOAD_INHIBITED LC_LOAD_PREFIX("Only allowed during BUILD_SHIPS phase.")
#define LC_LOAD_TARGET_NOT_FOUND LC_LOAD_PREFIX("No saved game named '{}' found.")
#define LC_LOAD_OVERWRITE LC_LOAD_PREFIX("You're already in that game.")
#define LC_LOAD_TARGET_SUCCEEDED "Game '{}' loaded. Resuming Turn {}."
#define LC_LOAD_TARGET_REQUESTED                                               \
    LC_LOAD_PREFIX("A load request for '{}' is pending.\nType 'reject {}' first.")
#define LC_LOAD_TARGET_WAIT                                                    \
    LC_LOAD_PREFIX("Requested to load '{}' (Turn {}).\nWaiting for other player to accept.")
#define LC_LOAD_TARGET_PROPOSAL                                                \
    LC_COMMAND_PREFIX("{} proposes loading '{}' (Turn {}).\nType 'accept {}' or 'reject {}'")
#define LC_LOAD_NO_PENDING "No pending load request."
#define LC_LOAD_TARGET_PENDING "Pending request is for '{}', not '{}'"
#define LC_LOAD_WAIT_FOR_ACCEPT                                                \
    "Waiting for the other player to accept your request."

#define LC_LOAD_TARGET_LOAD_CONFIRMED                                          \
    "[LOAD] Accept confirmed, switching to game {}"
#define LC_LOAD_TARGET_LOADED LC_COMMAND_PREFIX("Game '{}' loaded. Resuming Turn {}.\n{}")
#define LC_LOAD_NO_PENDING_REQ "No pending load request."

#define LC_LOAD_TARGET_REJECT_NOT_SAME_REQ                                     \
    "Pending request is for '{}', not '{}'"

#define LC_LOAD_TARGET_REJECT_REQ "Load request for '{}' rejected."
#define LC_LOAD_TARGET_REJECTED_RESPONSE LC_COMMAND_PREFIX("{} rejected loading '{}'")

#define LC_RECAP_IN_PROGRESS "Game is still in progress."
#define LC_RECAP_FLAVOR_1 "\"The stars remember those who dared.\""
#define LC_RECAP_FLAVOR_2 "\"Per aspera ad astra.\""
#define LC_RECAP_FLAVOR_3 "\"Beyond the horizon, the light endures.\""
#define LC_RECAP_FLAVOR_4 "\"What was charted can never be uncharted.\""
#define LC_RECAP_FLAVOR_5 "\"The void yields its secrets to the persistent.\""
#define LC_RECAP_FLAVOR_6 "\"All voyages end. Few are remembered.\""

#define LC_CRT_TABLE_BANNER "COMBAT RESULTS TABLE"
#define LC_CRT_SUFFIX_PHASIC_NOTE "Damage = Phasic Power + Tech Level"
#define LC_CRT_SUFFIX_HEX_NOTE ">> may be modified by hex events <<"
#define LC_GAME_WINNER "GAME OVER - Winner: {}"
#define LC_VP_TO_WIN "Victory: {} to win"
#define LC_ADDED_TARGET_VP "{} VP for controlling enemy base star system!"
#define LC_VP_SELF_WON_GAME  "*** VICTORY! You have won the game! ***"
#define LC_VP_SELF_LOST_GAME  "*** DEFEAT. Your opponent has won. ***"

#define LC_COMBAT_PREFIX(x) "TACTICAL: " x
#define LC_COMBAT_ERR_PREFIX(x) "TACTICAL ERROR: " x
#define LC_COMBAT_SHIP_PREFIX(x) LC_COMBAT_PREFIX("Ship {} ") x
#define LC_COMBAT_SHIP_NOT_EXIST LC_COMBAT_SHIP_PREFIX("does not exist.")
#define LC_COMBAT_SHIP_DESTROYED LC_COMBAT_SHIP_PREFIX("has been destroyed.")
#define LC_COMBAT_SHIP_NOT_YOURS LC_COMBAT_SHIP_PREFIX("is not yours.")
#define LC_COMBAT_SHIP_NOT_HOSTILE                                             \
    LC_COMBAT_SHIP_PREFIX("is not a hostile target.")
#define LC_COMBAT_SHIP_NOT_DEPLOYED LC_COMBAT_SHIP_PREFIX("is not deployed.")

#define LC_COMBAT_INITIATIVE_PLAYER_ONLY                                       \
    LC_COMBAT_ERR_PREFIX("Only the initiative player may initiate combat.")
#define LC_COMBAT_NO_RED_FORCES LC_COMBAT_PREFIX("No enemy forces at this hex.")
#define LC_COMBAT_NO_ORDERS                                                    \
    LC_COMBAT_ERR_PREFIX("Not accepting orders (stage {})")
#define LC_COMBAT_NO_QUEUED_ORDERS LC_COMBAT_PREFIX("No combat orders queued.")
#define LC_COMBAT_PENDING_ORDER_ERR                                            \
    LC_COMBAT_ERR_PREFIX("Orders pending for hex {} but active combat is in hex {}")

#define LC_COMBAT_ORDERS_TX LC_COMBAT_PREFIX("Orders transmitted.")
#define LC_COMBAT_ORDERS_NO_RX                                                 \
    LC_COMBAT_PREFIX("No combat orders have been received.")
#define LC_COMBAT_ORDERS_RECINDED                                              \
    LC_COMBAT_PREFIX("Orders rescinded. Issue new orders.")
#define LC_COMBAT_NO_PENDING_ORDERS                                            \
    LC_COMBAT_PREFIX("No pending combat orders.")

#define LC_COMBAT_RED_FORCE_HAS_ORDERS                                         \
    LC_COMBAT_PREFIX("Enemy has committed combat orders for hex {}")
#define LC_COMBAT_WAIT_RED_FORCE_ORDERS                                        \
    LC_COMBAT_PREFIX("Sector {} - Awaiting enemy orders from enemy")

#define LC_COMBAT_TACTICAL_COMBAT_PENDING_STAT                                 \
    LC_COMBAT_PREFIX("Combat pending! {} combats.size() active engagement(s) must be resolved.")

#define LC_COMBAT_TACTICAL_HINT                                                \
    "Use: CO <ship> <target> <tactic> [PD=X [P=Y [...]]"

#define LC_COMBAT_TACTICAL_TARGET_RETREAT_PENDING                              \
    LC_COMBAT_PREFIX("Retreat pending! {} ship(s) must complete withdrawal")
#define LC_COMBAT_TACTICAL_RETREAT_HINT "Use: retreat <ship> <hex>"
#define LC_COMBAT_TACTICAL_GAME_OVER ">> GAME OVER <<"
#define LC_COMBAT_ROUND_TARGET_DMG_REPORT \
          LC_COMBAT_PREFIX("ROUND {} DAMAGE REPORT\n" \
           "{} TOTAL: {} DAMAGE TO ASSIGN\n" \
           "Use 'combat apply <ship> pd=X [P=Y] ...' to assign damage.")
#define LC_COMBAT_ROUND_TARGET_NO_DMG LC_COMBAT_PREFIX("ROUND {} - YOUR SHIPS TOOK NO DAMAGE.")
#define LC_COMBAT_TARGET_STALEMATE_MSG LC_COMBAT_PREFIX("Two consecutive stalemates: You must retreat your ships from hex {}")
#define LC_COMBAT_NEXT_ROUND LC_COMBAT_PREFIX("Combat Round {} begins in hex {}. Submit orders.")
#define LC_COMBAT_TARGET_DMG_ASSIGNED LC_COMBAT_PREFIX("{} has assigned all damage.")
#define LC_COMBAT_TARGET_COMBAT_ENDED LC_COMBAT_PREFIX("Combat in hex {} ended. {} controls the hex.")
#define LC_COMBAT_TARGET_ALL_DMG_NEW_ORDERS LC_COMBAT_PREFIX("All damage assigned. Combat Round {} begins in hex {}. Make orders!")
#define LC_COMBAT_ENEMY_DETECTED LC_COMBAT_PREFIX("Enemy forces detected. Combat orders are optional.")
#define LC_COMBAT_TARGET_CONFLICT_BANNER "CONFLICT IN STAR SYSTEM: {} [{}]\n" \
                                         "SHIPS IN SYSTEM {}"
#define LC_COMBAT_TARGET_CONFLICT_BLUEFORCE "Blue-Force"
#define LC_COMBAT_TARGET_CONFLICT_REDFORCE "Blue-Force"



#define LC_ROOM_TARGET_SELF_HAVE_INIT LC_COMMAND_PREFIX("YOU HAVE THE INITIATIVE! {}")
#define LC_ROOM_TARGET_N_SELF_HAVE_INIT LC_COMMAND_PREFIX("{} HAS INITIATIVE. STANDING BY...")



#define LC_TURN_HAS_ENDED "Your turn has ended."
#define LC_TURN_TARGET_PLAYER_TURN_BEGUN "{}'s turn has begun."
#define LC_TURN_TARGET_THIS_PLAYER_TURN_BEGUN "Your Turn: {} Round {}"
#define LC_TURN_TARGET_IT_IS_YOUR_TURN "It's YOUR turn! Phase: {}"
#define LC_TURN_DECLARE_PHASE "Phase {}"
#define LC_TURN_DECLARE_ROUND "Round {}"
#define LC_TURN_OPP_LEFT_GAME                                                  \
    "Opponent has left the game. Game saved and ended."
#define LC_TURN_TARGET_AUTO_SAVE_TO_LOBBY                                      \
    "Game auto-saved as '{}'.\nReturning to lobby.\n\033[LOBBY]"

#define LC_TARGET_SHIP_NOT_FOUND "Ship {} not found"
#define LC_MILIEU_PREFIX(y, x) y ": " x
#define LC_MILIEU_FABRICATE "FABRICATE"
#define LC_MILIEU_SURVEY "SURVEY"
#define LC_MILIEU_SYSTEM "SYSTEM"
#define LC_MILIEU_OUTFIT "OUTFIT"
#define LC_MILIEU_SALVAGE "SALVAGE"
#define LC_MILIEU_MARKET "MARKET"
#define LC_MILIEU_EXTRACT "EXTRACT"

#define LC_MILIEU_UNKNOWN_FABRICATE_MODE                                       \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Unknown fabrication mode.")
#define LC_MILIEU_FABRICATE_HINT "Use: fabricate <plan> [qty]"
#define LC_MILIEU_FABRICATE_CARGO_CHK_FAILED                                   \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Unable to check cargo inventory.")

#define LC_MILIEU_FABRICATE_TARGET_LACK_RES                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Insufficient {}. Need {}, have {}")

#define LC_MILIEU_FABRICATE_TARGET_NO_PLAN                                     \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Plan '{}' not found in database.")
#define LC_MILIEU_FABRICATE_NO_SHIPS_DEPLOYED                                  \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "No ships deployed to fabrication sites.")
#define LC_MILIEU_FABRICATE_SHIP_LOCATION_INVALID                              \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "A Ship must be at a SHIPYARD or REFINERY you control.")
#define LC_MILIEU_FABRICATE_TARGET_RESULT                                      \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "{} torpedoes loaded to {}")
#define LC_MILIEU_FABRICATE_LAUNCHER_UPGRADE_TARGET_RESULT                     \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "Queued launcher upgrade +{} will complete in round {}")
#define LC_MILIEU_FABRICATE_PHASIC_UPGRADE_TARGET_RESULT                       \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "Queued phasic upgrade +{} will complete in round {}")

#define LC_MILIEU_FABRICATE_SHIELD_UPGRADE_TARGET_RESULT                       \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "Queued shield upgrade +{} will complete in round {}")

#define LC_MILIEU_FABRICATE_TECH_UPGRADE_TARGET_RESULT                         \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE,                                      \
                     "Queued tech research +{} will complete in round {}")


#define LC_MILIEU_FABRICATE_TARGET_QUEUED_COMPLETE                         \
    LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "FABRICATION COMPLETE: {} - {}")


// Survey

#define LC_MILIEU_NO_SHIPS_FOR_SURVEY                                          \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "No ships available to conduct "        \
                                       "survey.")

#define LC_MILIEU_SURVEY_UNKNOWN_SYSTEM                                        \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "Unknown system '{}'")
#define LC_MILIEU_SURVEY_NO_SHIPS_PRESENT                                      \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "No ships present in {}")
#define LC_MILIEU_SURVEY_SHIPS_REQUIRED                                        \
    "You must have a vessel in-system to conduct survey operations."
#define LC_MILIEU_SURVEY_MAX_INFO                                              \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, \
      "{} already at maximum knowledge level (Intimate).")
#define LC_MILIEU_SURVEY_MAX_SECRETS "All system secrets are known to you."
#define LC_MILIEU_TARGET_SURVEY_COMPLETE                                       \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "{} survey complete.")
#define LC_MILIEU_TARGET_INFO_UPGRADED                                         \
    LC_MILIEU_PREFIX(LC_MILIEU_SURVEY,                                         \
                     "Knowledge upgraded: {} -> {} survey complete.")
#define LC_MILIEU_SURVEY_CHARTED_LEVEL                                         \
    "Planetary data and facility locations now available."
#define LC_MILIEU_SURVEY_DETAILED_LEVEL "Detailed resource data now available."
#define LC_MILIEU_SURVEY_INTIMATE_LEVEL                                        \
    "All system secrets revealed, including anomalies."
#define LC_MILIEU_SURVEY_TARGET_MORE "Use 'system {} planets' to view."
#define LC_MILIEU_SURVEY_TARGET_RESOURCES "Use 'system {} resources' to view."
#define LC_MILIEU_SURVEY_TARGET_ANOMALIES "Use 'system {} anomalies' to view."
#define LC_MILIEU_UNKNOWN_SYSTEM                                               \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unknown system '{}'")
#define LC_MILIEU_UNKNOWN_SYSTEM_SUBCOMMAND                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unknown subcommand. > HELP SYSTEM")
#define LC_MILIEU_SYSTEM_NO_SHIPS                                              \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "You have No ships at this system to investigate.")
#define LC_MILIEU_SYSTEM_NO_POWER                                              \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Insufficient PD to investigate. Requires {} PD.")

#define LC_MILIEU_SYSTEM_SHOW_BANNER                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Star system: {}")
#define LC_MILIEU_SYSTEM_SHOW_FACILITIES_BANNER                                \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "{} FACILITIES")

#define LC_MILIEU_SYSTEM_SHOW_INFO_LEVEL                                       \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Knowledge Level: {}")

#define LC_MILIEU_SYSTEM_SHOW_PLANET                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Planets: {}")
#define LC_MILIEU_SYSTEM_SHOW_ASTEROID                                         \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Asteroid Belts: {}")

#define LC_MILIEU_SYSTEM_SHOW_TARGET_PLANET_HINT                               \
    "Use 'system {} planets' for planetary data."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_FACILITIES_HINT                           \
    "Use 'system {} facilities' for infrastructure."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_RESOURCES_HINT                            \
    "Use 'system {} resources' for deposits."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_ANOMALIES_HINT                            \
    "Use 'system {} anomalies' for discoveries."

#define LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN                                  \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Planetary data requires Charted knowledge "              \
                     "level.\nCurrent: {}. Send a ship to survey.")

#define LC_MILIEU_SYSTEM_NO_PLANET_RECORDS "No planetary bodies on record."
#define LC_MILIEU_SYSTEM_NO_INFRA_RECORDS                                      \
    "No significant infrastructure on record."

#define LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN_SURVEY                           \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Resource data requires Surveyed knowledge "              \
                     "level.\nCurrent: {}. Use 'survey' command.")

#define LC_MILIEU_SYSTEM_FACILITIES_DATA_TARGET_INFO_MIN_CHARTED               \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Facility data requires Charted knowledge "               \
                     "level.\nCurrent: {}. Send a ship to survey.")

#define LC_MILIEU_SYSTEM_ANOMALIES_DATA_TARGET_INFO_MIN_INTIMATE               \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Anomaly data requires Intimate knowledge "               \
                     "level.\nCurrent: {}. Extended presence required.")

#define LC_MILIEU_SYSTEM_TARGET_SYSTEM_UNKNOWN                                 \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unknown system '{}'")
#define LC_MILIEU_SYSTEM_NO_SHIPS_FOR_SURVEY                                   \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "You have no ships at this system to perform a survey.")

#define LC_MILIEU_SYSTEM_LACK_CR_FOR_SURVEY                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM,                                         \
                     "Insufficient PD to perform survey. Requires {} PD.")

#define LC_MILIEU_MARKET_SHOW_BANNER                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "MARKET EXCHANGE (Round {})")


#define LC_MILIEU_MARKET_TARGET_HUB_REVENUE LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Trade hub revenue +{} CR this round.")

#define LC_MILIEU_MARKET_TARGET_NO_HISTORY                                     \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "No trading history for {}.")
#define LC_MILIEU_MARKET_NO_HISTORY_HINT                                       \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET,                                         \
                     "Prices may not have fluctuated yet this game.\nUse "     \
                     "'trade list' to see current exchange rates.")

#define LC_MILIEU_EXTRACT_NO_DEPLOYED_SHIPS                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT,                                        \
                     "No deployed ships available for extracting.")

#define LC_MILIEU_EXTRACT_BANNER "HARVESTABLE RESOURCES"
#define LC_MILIEU_TARGET_AT "{} at {}: "
#define LC_MILIEU_TARGET_AT_PREFIX(x) "{} at {}: " x
#define LC_MILIEU_TARGET_NO_EXTRACT_RESOURCES                                  \
    LC_MILIEU_TARGET_AT_PREFIX("No resources")

#define LC_MILIEU_EXTRACT_HINT_RESOURCE "Use: extract <ship> <resource_type>"

#define LC_MILIEU_EXTRACT_TARGET_SHIP_MISSING                                  \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Vessel {} not found for extraction.")

#define LC_MILIEU_EXTRACT_SHIP_MUST_BE                                         \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT,                                        \
                     "Ship must be deployed to a system to extract.")

#define LC_MILIEU_EXTRACT_TARGET_NO_DEPOSITS                                   \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT,                                        \
                     "No {} deposits found in {} star system")

#define LC_MILIEU_EXTRACT_CARGO_FULL                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "{} cargo hold is full!")

#define LC_MILIEU_EXTRACT_TARGET_SUCCEEDED                                     \
    LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT,                                        \
                     "{} extracted {} units of {} from {} ({})")

#define LC_MILIEU_MARKET_TRADE_REQMT                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET,                                         \
                     "Trade requires ship at TRADE_HUB facility.")
#define LC_MILIEU_MARKET_TRADE_BANNER "TRADE EXCHANGE RATES"

#define LC_MILIEU_MARKET_TRADE_TARGET_UNKNOWN_RESOURCE                         \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Unknown resource type: {}")

#define LC_MILIEU_MARKET_TRADE_TARGET_LACKING_CREDIT                           \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET,                                         \
                     "Insufficient credits. Need {} CR, have {} CR")

#define LC_MILIEU_MARKET_TRADE_TARGET_LACKING_RESOURCE                         \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Insufficient {}. Have {}, need {}")

#define LC_MILIEU_MARKET_TRADE_TARGET_SUCCEEDED                                \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET,                                         \
                     "Purchased {} {} for {} CR.\nLoaded onto {}")

#define LC_MILIEU_MARKET_TRADE_NO_CARGO_SHIP                                   \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "No ships available to receive cargo.")

#define LC_MILIEU_MARKET_TRADE_TARGET_SOLD                                     \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Sold {} {} for {} CR")

#define LC_MILIEU_MARKET_TRADE_TARGET_SHIP_LACKING_RESOURCE                    \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "{} only has {} {}")

#define LC_MILIEU_MARKET_TRADE_TARGET_XFR                                      \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Transferred {} {} from {} to {}")

#define LC_MILIEU_MARKET_TRADE_SHIP_MISSING                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Ship {} not found.")

#define LC_MILIEU_OUTFIT_BANNER "EQUIPMENT OUTFITTING"
#define LC_MILIEU_OUTFIT_REQMT "Requires ship at controlled SHIPYARD."
#define LC_MILIEU_OUTFIT_USAGE "Use: outfit <ship> <equipment>"
#define LC_MILIEU_OUTFIT_EXAMPLE "Example: outfit W1 lrs"
#define LC_MILIEU_OUTFIT_TARGET_NOT_IN_CATALOG                                 \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Equipment {} not in catalog.")
#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_FOUND                                 \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, LC_TARGET_SHIP_NOT_FOUND)
#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_DEPLOYED                              \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Ship {} is not deployed.")

#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_AT_SYSTEM                             \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Ship {} is not at a star system.")

#define LC_MILIEU_OUTFIT_TARGET_NO_SHIPYARD                                    \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "{} has no SHIPYARD you control.")

#define LC_MILIEU_OUTFIT_LACK_CREDIT                                           \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT,                                         \
                     "Insufficient credits. Need {} CR, have {} CR.")

#define LC_MILIEU_OUTFIT_INSTALLED                                             \
    LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Installed {} on {} for {} CR.")

#define LC_SALVAGE_EXAMPLE "Use: salvage <ship> <resource> to salvage."
#define LC_SALVAGE_NO_SHIP_CODE "Cannot salvage without shipcode."
#define LC_SALVAGE_NO_SHIPS "SALVAGE SCAN: No deployed ships to scan from."
#define LC_SALVAGE_UNKNOWN_SIGNAL "[?] Unknown signal detected (scan again?)"
#define LC_SALVAGE_NONE "No salvageables found."
#define LC_SALVAGE_FOUND "{} salvageable(s) known."
#define LC_SALVAGE_SHIP_NOT_FOUND                                              \
    LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Ship {} not found.")
#define LC_SALVAGE_NEED_DEPLOYED_SHIP                                          \
    LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Ship must be deployed to salvage.")

#define LC_SALVAGE_TARGET_NOT_FOUND                                            \
    LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "No known salvageables in {}.")
#define LC_SALVAGE_SUGGEST_SCAN "Use 'salvage scan' to search."
#define LC_SALVAGE_SUGGEST_TARGET_SALVAGE "Use: salvage {} <resource>"

#define LC_SALVAGE_TARGET_AVAILABLE                                            \
    LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Available salvageables in {}")

#define LC_SALVAGE_NO_MATCH_TARGET                                             \
    LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE,                                        \
                     "No salvageable matching '{}' found in {}")

#define LC_SALVAGE_TARGET_HAZARD                                               \
    LC_MILIEU_PREFIX(                                                          \
        LC_MILIEU_SALVAGE,                                                     \
        "{} encountered hazards at {}.  Suffered {} hull damage.\n")

#define LC_SALVAGE_SHIP_DESTROYED "Ship {} destroyed!"
#define LC_SALVAGE_TARGET_DEPLEATED "{} is now depleated."

// Repair / Resupply
#define LC_REPRES_NO_DAMAGED_SHIPS_AT_BASES                                    \
    "No damaged ships at your base star systems."
#define LC_REPRES_LIST_BANNER "Repairable ships at your base stars"
#define LC_REPRES_HINT "Use: repair <ship_code> <attribute>=<amount>"
#define LC_REPRES_EXAMPLE "Example: repair W1 pd=2 (costs 2 BP)"
#define LC_REPRES_TARGET_SHIP_NOT_FOUND "Ship {} not found."
#define LC_REPRES_SHIP_MUST_BE_AT_FACILITY                                     \
    "Ship must be at your base star or a controlled repair facility."
#define LC_REPRES_TARGET_SPECIFY                                               \
    "Specify attribute and amount: repair {} pd=N [p=N [ ...]]  See help "     \
    "topic: Attributes"
#define LC_REPRES_INVALID_ATTR "Invalid attribute. See help topic: Attributes"
#define LC_REPRES_AT_MAX "Attribute already at maximum."
#define LC_REPRES_REPAIR_TORPEDO_AT_MAX                                        \
    "Ship already at maximum torpedo capacity."
#define LC_REPRES_COST_LIMIT "Insufficient CR. Need {}, have {}"
#define LC_REPRES_TARGET_SUCCEEDED "Repaired {} {} by {} (cost: {}) CR"
#define LC_REPRES_NO_TORPEDO_NEEDED                                            \
    "No ships need torpedo resupply at your base star systems."
#define LC_REPRES_RESUP_LIST_BANNER                                            \
    "Ships that can be resupplied at your base star systems"
#define LC_REPRES_RESUPPLY_HINT "Use: resupply <ship_code> <quantity>"
#define LC_REPRES_RESUPPLY_EXAMPLE                                             \
    "Example: resupply W1 6 (costs 2 CR for 6 torpedoes)"
#define LC_REPRES_RESUPPLY_SHIP_NOT_AT_BASE                                    \
    "Ship not found or not at your base stars systems"
#define LC_REPRES_RESUPPLY_TARGET_SPECIFY_TORPEDO                              \
    "Specify quantity: resupply {} N"
#define LC_REPRES_TARGET_RESUPPLY_COST_LIMIT                                   \
    "Insufficient CR. Need {}, have {} CR"
#define LC_REPRES_TARGET_RESUPPLY_SUCCEEDED                                    \
    "Resupplied {} with {} torpedoe(s) (cost: {} CR)"

// Hex Info

#define LC_HEX_INFO_TARGET_NO_SYSTEM "HEX: No system found at '{}'"
#define LC_HEX_INFO_TARGET_NO_SHIP_AT                                          \
    "HEX: You have no ships at this location ({}) to perform a scan."
#define LC_HEX_INFO_TARGET_LACK_CR                                             \
    "HEX: Insufficient PD to perform scan. Requires {} PD."
#define LC_HEX_INFO_SCAN_TARGET_VIA "(LRS scan via {})"
#define LC_HEX_INFO_SCAN_COST_TARGET "(Scan cost: {} PD from {})"

#define LC_SS_H "Shipyard report: SystemShips cannot have Hangars"
#define LC_TORPEDO_BY_THREE "Error: Torpedoes must be a multiple of 3."
#define LC_NEG_ATTR "Error: Negative attribute values not allowed."
#define LC_TORPEDO_LAUNCHER_MISMATCH "Launchers must be > 0 for Torpedoes > 0"
#define LC_INSUFFICIENT_BUILD_CREDITS                                          \
    "SHIPYARD: Insufficient Build Points. Construction halted."
#define LC_INVALID_HULL_DESIGNATION                                            \
    "SHIPYARD: Invalid hull designation. Review ship code."
#define LC_DUPLICATE_HULL_CODE "SHIPYARD: Hull {} already on drafting board."
#define LC_SHIP_ALREADY_COMISSIONED                                            \
    "SHIPYARD: Vessel {} already commissioned in fleet."
#define LC_SHIP_HULL_LAID "SHIPYARD: Hull {} laid down. Designation: {}"
#define LC_NEED_HULL_DESIGNATOR_TO_FIND                                        \
    "SHIPYARD: Need ship hull designator or name to find it."
#define LC_SHIPYARD_SHIP_NOT_EXIST "SHIPYARD: Ship {} does not exist."
#define LC_SHIPYARD_LOST_SHIP "SHIPYARD: Ship {} is LOST from space dock."
#define LC_SHIPYARD_SHIP_NOT_FOUND                                             \
    "SHIPYARD: Ship with hull designation {} not in shipyard"
#define LC_SHIPYARD_HULL_CANCELED "SHIPYARD: Canceled: {}"
#define LC_FLEET_OPS_NO_SHIPS "FLEET OPS: No ships under your command."

//  MOVE

#define LC_MOVE_TARGET_SHIP_NOT_IN_FLEET                                       \
    "FLEET REGISTRY: Vessel {} is not in your fleet!"
#define LC_MOVE_ONLY_W_SHIP_CAN_CHART                                          \
    "CHART: Only WarpShip class vessels can chart warp courses."
#define LC_MOVE_TARGET_SHIP_HAS_NO_WARP_CAP                                    \
    "CHART: {} has no power drive capacity."
#define LC_MOVE_TARGET_SHIP_NOT_DEPLOYED "CHART: {} is not deployed."
#define LC_MOVE_TARGET_MOVEMENT_COST_PREAMBLE "Cost: {} PD (available: {})"
#define LC_MOVE_TARGET_WARPLINE_JUMP "{} warpline {}"
#define LC_MOVE_TARGET_SHIP_IN_HANGAR_ERROR                                    \
    "Ship is racked. Drop it before deploying {}"
#define LC_MOVE_DEPLOY_UNKNOWN_SYSTEM "DEPLOY: Unknown system '{}'"
#define LC_MOVE_DEPLOY_ONLY_AT_BASES                                           \
    "DEPLOY: Ships can only be deployed at a home base star system."
#define LC_MOVE_TARGET_DEPLOY_ANNOUNCE_BASE_CLAIM                              \
    "DEPLOY: {} has claimed the {}."
#define LC_MOVE_DEPLOY_INHIBITED_IN_ENEMY_TERRITORY                            \
    "DEPLOY: Cannot deploy in enemy territory. Use your home bases."
#define LC_MOVE_TARGET_ANNOUNCE_DEPLOY "FLEET COMMAND: {} ({}) deployed to {}"

#define LC_MOVE_INHIBITED_WARP                                                 \
    "NAV: Only WarpShip class vessels can engage hyperdrive."
#define LC_MOVE_TARGET_INHIBITED_WARP                                          \
    "NAV: {} has no power drive capacity. Unable to maneuver."
#define LC_MOVE_TARGET_SHIP_IN_HANGAR "OPS: Ship {} is racked and cannot move"

#define LC_MOVE_TARGET_IN_SHIPYARD_INHIBITED                                   \
    "NAV: {} is not deployed. It is still in shipyard."

#define LC_MOVE_TARGET_AT_DEST "NAV: {} is already at {}."
#define LC_MOVE_TARGET_POWERDRIVE_DEPLEATED                                    \
    "NAV: {} has exhausted power drive for this turn."
#define LC_MOVE_TARGET_HAZARD_ON_MOVE                                          \
    "HAZARD: Navigation hazard in hex {}!\n{} PD used to overcome hazard."
#define LC_MOVE_COURSE_PLOT_BANNER "NAV: Course plotted. Standby."
#define LC_MOVE_TARGET_TRANSIT_PATH                                            \
    "OPS: {} ({}) transit to {} [{}] complete.\nPower expended: {} PD"
#define LC_MOVE_TARGET_TRIGGER_TACTICAL                                        \
    "TACTICAL ALERT: Contact!\nEnemy forces detected in {}!"
#define LC_MOVE_TARGET_TRIGGER_ENEMY_IN_RANGE                                  \
    LC_MOVE_TARGET_TRIGGER_TACTICAL                                            \
        "\nYou may elect to engage during Combat phase."
#define LC_MOVE_TACTICAL_ALERT_BANNER "TACTICAL ALERT"
#define LC_MOVE_TACTICAL_ALERT_DETECTED "SCANNERS DETECT"
#define LC_RETREAT_TARGET_SHIP_NOT_FOUND "RETREAT: Ship {} not found"
#define LC_RETREAT_TARGET_NO_PENDING_RETREAT_REQMT                             \
    "RETREAT: {} has no pending retreat."
#define LC_RETREAT_TARGET_INVALID_HEX "RETREAT: Invalid hex: {}"
#define LC_RETREAT_TARGET_INVALID_PROXIMITY "RETREAT: {} is not adjacent to {}"
#define LC_RETREAT_VALID_DEST_BANNER "Valid retreat destinations"
#define LC_RETREAT_TARGET_ANNOUNCE "RETREAT: {} withdraws to {}"

// PICK DROP

#define LC_PICKDROP_PREFIX(x) "OPS: " x
#define LC_PICKDROP_TARGET_NO_SHIP_IN_FLEET                                    \
    LC_PICKDROP_PREFIX("Systemship {} not found in your fleet.")
#define LC_PICKDROP_TARGET_INHIBIT_WARPSHIP                                    \
    LC_PICKDROP_PREFIX("{} is a warpship. Cannot rack a warpship.")
#define LC_PICKDROP_TARGET_ALREADY_HANGAR                                      \
    LC_PICKDROP_PREFIX("{} is already secured in Systemship hangar of {}")
#define LC_PICKDROP_TARGET_NO_WARPSHIP_IN_FLEET                                \
    LC_PICKDROP_PREFIX("Warpship {} not found in your fleet.")
#define LC_PICKDROP_TARGET_NOT_WARPSHIP                                        \
    LC_PICKDROP_PREFIX("{} is not a warpship.")
#define LC_PICKDROP_TARGET_WARPSHIP_LACK_HANGAR                                \
    LC_PICKDROP_PREFIX("{} has no Systemship hangars (H=0).")
#define LC_PICKDROP_TARGET_WARPSHIP_LOCATION                                   \
    LC_PICKDROP_PREFIX("Ships must be at same location. {} at {}, {} at {}")
#define LC_PICKDROP_TARGET_WARPSHIP_HANGAR_CAPACITY                            \
    LC_PICKDROP_PREFIX("{} Systemship hangars are full ({}/{}).")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_HANGAR_SUCCESS                           \
    LC_PICKDROP_PREFIX("{} held aboard {}.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_NOT_IN_HANGAR                            \
    LC_PICKDROP_PREFIX("Systemship {} is not held in any warpship.")
#define LC_PICKDROP_TARGET_NO_WARP_SHIP_IN_FLEET                               \
    LC_PICKDROP_PREFIX("Warpship {} not found in your fleet.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_HELD_DIFFERENT                           \
    LC_PICKDROP_PREFIX("{} is not held in {}. (Currently held by {}).")
#define LC_PICKDROP_TARGET_INHIBIT_DROP_SPACE                                  \
    LC_PICKDROP_PREFIX(                                                        \
        "Cannot drop systemship in deep space. {} must be at a star system.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_DROP_SUCCESS                             \
    LC_PICKDROP_PREFIX("{} deployed from {} at {}.")

// Demo page info

#define LC_DEMO_PAGE_INFO                                                      \
    "DEMONSTRATION\n"                                                          \
    "BUILD\n"                                                                  \
    " > BN W IRONSTAR\n"                                                       \
    " > BS PD=8 P=4 S=1 L=1 T=3 H=1\n"                                         \
    " > BC\n"                                                                  \
    "MOVE\n"                                                                   \
    "  A Waypoint can be a HEX or a named Star System\n"                       \
    "  If W4 desires movement from HAVOR to LYRIS to NOREX to H1613\n"         \
    " > M W4 LYRIS NOREX H1613\n"                                              \
    "COMBAT\n"                                                                 \
    " > CO W2 A W9 PD=5 P=3\n"                                                 \
    "  W2 is using Attack tactic against W9.\n"                                \
    "  W2 is powering up to powerdrive level 5\n"                              \
    "  W2 is powering phasics to level 3\n"                                    \
    " As the defender (owner of W9), set your orders:\n"                       \
    " > CO W9 D W2 PD=4 P=1\n"                                                 \
    "  W9 is using Dodge tactic against W2.\n"                                 \
    "  W9 is powering up to powerdrive level 4\n"                              \
    "  W9 is powering phasics to level 1\n"

#define LC_HELP_PAGE_INFO_BASIC                                                \
    "TACTICAL COMMANDS\n"                                                      \
    "BUILD\n"                                                                  \
    "  BN W/S {NAME}          Build New Warpship/Systemship NAME\n"            \
    "  BS {SHIP} {SPECS}      Set ship attributes\n"                           \
    "  BC/BX                  Commit to Fleet / Cancel Draft Build\n"          \
    "  BD                     Show Drafts\n"                                   \
    "MOVE\n"                                                                   \
    "  M {SHIP} {LIST}        Move ship to/through SYS(s) or HEX(s)\n"         \
    "  DEPLOY {SHIP} {SYS}    Deploy SHIP to SYS\n"                            \
    "  PICK/DROP {SHIP}       Rack/Drop systemship between warpship\n"         \
    "COMBAT\n"                                                                 \
    "  C                      Show active combat\n"                            \
    "  CO {SHIP} A/D/E {TGT}  Your SHIP  Attack/Defend/Evade TGT\n"            \
    "  CC/CX                  Execute/Cancel orders\n"                         \
    "  CD {SHIP} {SPECS}      Assign damage to ship\n"                         \
    "TURN\n"                                                                   \
    "  N/FF                   Next Phase / Fast-Forward (ends turn)\n"         \
    "INFO\n"                                                                   \
    "  FLEET | SCORE | STATUS | SYSTEM {NAME} | SURVEY | CRT | DEMO\n"

#define LC_HELP_NO_TOPICS "No topics available."

#define LC_HELP_TOPICS_BANNER "AVAILABLE HELP TOPICS"

#define LC_HELP_TARGET_NO_TOPIC "No help available for '{}'. Use: help topics"

#endif
