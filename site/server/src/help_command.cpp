//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "help_command.h"

#include "db.h"
#include "logger.h"
#include "telemetry.h"

static const std::string topics_keyword("topics");

static  std::string help_page = 
        "══════════════════ T A C T I C A L   C O M M A N D S ════════════════\n"
        "BUILD\n"
        "  BN W/S {NAME}              Build New Warpship/Systemship NAME\n"
        "  BS {SHIP} {SPECS}⧫         Set ship attributes\n"
        "  BC                         Commit to Fleet\n"
        "  BX                         Cancel Build\n"
        "  BD                         Show Drafts\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "MOVE\n"
        "  M {SHIP} {LIST}⧫⧫          Move ship to/through SYSTEM(s) or HEX(s)\n"
        "  PICK {SHIP}                Rack systemship into warpship\n"
        "  DROP {SHIP}                Unrack systemship from warpship\n"
        "  DEPLOY {SHIP} {SYSTEM}     Deploy Ship to SYSTEM\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "COMBAT\n"
        "  C                          Show active combat\n"
        "  CO {SHIP} A/D/E {TARGET}   Order SHIP to Attack/Defend/Evade\n"
        "                             Enemy TARGET\n"
        "  CC                         Execute orders\n"
        "  CX                         Cancel orders\n"
        "  CD {SHIP} {SPECS}⧫         Assign damage to ship\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "TURN\n"
        "  FF                         Fast-forward (end your turn)\n"
        "  N                          Go to next Game Phase\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "ECONOMY\n"
        "  EXTRACT | MARKET | TRADE | FABRICATE\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "INFO\n"
        "  STATUS | FLEET | SCORE | SYSTEM {NAME} | SURVEY | CRT | DEMO\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "          MORE HELP: github.com/sibomots/KeplersHorizon/wiki\n"
        "     B BEAM   S SCREEN   T TUBE   M MISSILES   PD POWERDRIVE\n"
        "                     W WARPSHIP   S SYSTEMSHIP\n"
        "             ⧫  ONE OR MORE OF:  PD=# B=# S=# T=# M=#\n"
        "  ⧫⧫ MAY MOVE TO/THROUGH LIST OF ONE OR MORE SYSTEM(s) AND/OR HEX(s)\n"
        "═════════════════════════════════════════════════════════════════════\n";

static const std::string demo_page =
        "══════════════════════ D E M O N S T R A T I O N ════════════════════\n"
        "BUILD\n"
        "  Build New Warpship called IRONSTAR\n"
        "  Desired: Power Drive 8, Beams 4, Screens 1, Tubes 1, Misiles 3\n"        
        "      Also desired:  1 System Ship Rack\n"
        " > BN W IRONSTAR\n" 
        " > BS PD=8 B=4 S=1 T=1 M=3 SR=1\n"
        " > BC\n"
        "  Build New SystemShip called PULSEFIRE\n"
        "  Desired: Power Drive 6, Beams 4, Screens 4\n"        
        " > BN S PULSEFIRE\n" 
        " > BS PD=6 B=4 S=4\n"
        " > BC\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "MOVE\n"
        "  Movement command takes a list of ONE or MORE waypoints.\n"
        "  A Waypoint can be a HEX or a named Star System\n"
        "  - If a warpline exists between two waypoints, it will be used.\n"
        "  - If a warpline does not exist between two waypoints, the shortest\n"
        "    path of hexes will be used between those waypoints.\n"
        "\n" 
        "  If W4 is on ABAD, then:\n"
        "  If W4 desires movement from ABAD to KHAFA to LAGASH to H1613\n"
        " > M W4 KHAFA LAGASH H1613\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "COMBAT\n"
        "  Your ship W2 and the enemy ship W9 are in the same star hex.\n"
        "  This will trigger combat.\n"
        "  As the intitiator of combat, the command to attack W9 is:\n"
        " > CO W2 A W9 PD=5 B=3\n"
        "  Meaning:  W2 is using Attack tactic against W9.\n"
        "            W2 is powering up to powerdrive level 5\n"
        "            W2 is powering beams to level 3\n" 
        " As the defender (owner of W9), set your orders:\n"
        " > CO W9 D W2 PD=4 B=1\n"
        "  Meaning:  W9 is using Dodge tactic against W2.\n"
        "            W9 is powering up to powerdrive level 4\n"
        "            W9 is powering beams to level 1\n"
        "─────────────────────────────────────────────────────────────────────\n"
        "          MORE HELP: github.com/sibomots/KeplersHorizon/wiki\n"
        "═════════════════════════════════════════════════════════════════════\n";

bool HelpCommand::invoke(void)
{
    if (m_demo) {
        Telemetry::getInstance().write(demo_page);
    }
    else if (m_topic.empty())
    {
        Telemetry::getInstance().write(help_page);
    }
    else {
        // Look up topic from database
        DatabaseManager& db = DatabaseManager::getInstance();
        
        // Convert to lowercase for case-insensitive lookup
        for (auto& c : m_topic) c = std::tolower(c);
        
        // Special case: "topics" lists all available topics
        if (m_topic.compare(topics_keyword) == 0) {
            auto rows = 
              db.query("SELECT DISTINCT topic_keyword FROM help_lookup ORDER BY topic_keyword");
            
            if (rows.empty()) {
                Telemetry::getInstance().write("No topics available.\n");
                return true;
            }
            
            std::string out = "AVAILABLE HELP TOPICS:\n";
            int col = 0;
            for (const auto& row : rows) {
                out += row[0];
                col++;
                if (col >= 6) {
                    out += "\n";
                    col = 0;
                } else {
                    // Pad to 12 chars for column alignment
                    int pad = 12 - row[0].length();
                    for (int i = 0; i < pad && i < 12; i++) out += " ";
                }
            }

            if (col > 0) {
                 out += "\n";
            }
            Telemetry::getInstance().write(out);
            return true;
        }
       
        // Actually trying to find help on m_topic
        auto rows = db.query(
            "SELECT t.topic_info FROM help_topics t "
            "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
            "WHERE l.topic_keyword = '" + db.esc(m_topic) + "' LIMIT 1");
        
        if (rows.empty())
        {
            // Fallback to 'general' topic
            rows = db.query(
                "SELECT t.topic_info FROM help_topics t "
                "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
                "WHERE l.topic_keyword = 'general' LIMIT 1");
        }
        
        if (!rows.empty() && !rows[0].empty())
        {
            Telemetry::getInstance().write(rows[0][0]);
        }
        else
        {
            Telemetry::getInstance().write(
                "No help available for '" + m_topic + "'. Try: help topics");
        }
    }    
    return true;
}
