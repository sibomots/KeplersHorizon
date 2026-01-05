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

bool HelpCommand::invoke(void)
{

    // this case happens if the command was:  > demo   OR  > help demo
    if (m_demo) {

    }
    else if (m_topic.empty())
    {
    std::string help = 
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
    
    Telemetry::getInstance().write(help);
    }
    else {
        // Look up topic from database
        DatabaseManager& db = DatabaseManager::getInstance();
        std::string keyword = m_topic;
        
        // Convert to lowercase for case-insensitive lookup
        for (auto& c : keyword) c = std::tolower(c);
        
        // Special case: "topics" lists all available keywords
        if (keyword == "topics") {
            auto rows = db.query(
                "SELECT DISTINCT topic_keyword FROM help_lookup ORDER BY topic_keyword");
            
            if (rows.empty()) {
                Telemetry::getInstance().write("No help topics available.");
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
            if (col > 0) out += "\n";
            Telemetry::getInstance().write(out);
            return true;
        }
        
        // Look up topic by keyword
        auto rows = db.query(
            "SELECT t.topic_info FROM help_topics t "
            "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
            "WHERE l.topic_keyword = '" + db.esc(keyword) + "' LIMIT 1");
        
        if (rows.empty()) {
            // Fallback to 'general' topic
            rows = db.query(
                "SELECT t.topic_info FROM help_topics t "
                "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
                "WHERE l.topic_keyword = 'general' LIMIT 1");
        }
        
        if (!rows.empty() && !rows[0].empty()) {
            Telemetry::getInstance().write(rows[0][0]);
        } else {
            Telemetry::getInstance().write(
                "No help available for '" + m_topic + "'. Try: help topics");
        }
    }    
    return true;
}
