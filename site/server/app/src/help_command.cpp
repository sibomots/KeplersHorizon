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

static std::string help_page =
    "TACTICAL COMMANDS\n"
    "BUILD\n"
    "  BN W/S {NAME}          Build New Warpship/Systemship NAME\n"
    "  BS {SHIP} {SPECS}      Set ship attributes\n"
    "  BC/BX                  Commit to Fleet / Cancel Draft Build\n"
    "  BD                     Show Drafts\n"
    "MOVE\n"
    "  M {SHIP} {LIST}        Move ship to/through SYS(s) or HEX(s)\n"
    "  DEPLOY {SHIP} {SYS}    Deploy SHIP to SYS\n"
    "  PICK/DROP {SHIP}       Rack/Drop systemship between warpship\n"
    "COMBAT\n"
    "  C                      Show active combat\n"
    "  CO {SHIP} A/D/E {TGT}  Your SHIP  Attack/Defend/Evade TGT\n"
    "  CC/CX                  Execute/Cancel orders\n"
    "  CD {SHIP} {SPECS}      Assign damage to ship\n"
    "TURN\n"
    "  N/FF                   Next Phase / Fast-Forward (ends turn)\n"
    "INFO\n"
    "  FLEET | SCORE | STATUS | SYSTEM {NAME} | SURVEY | CRT | DEMO\n";

static const std::string demo_page =
    "DEMONSTRATION\n"
    "BUILD\n"
    " > BN W IRONSTAR\n"
    " > BS PD=8 B=4 S=1 T=1 M=3 SR=1\n"
    " > BC\n"
    "MOVE\n"
    "  A Waypoint can be a HEX or a named Star System\n"
    "  If W4 desires movement from HAVOR to LYRIS to NOREX to H1613\n"
    " > M W4 LYRIS NOREX H1613\n"
    "COMBAT\n"
    " > CO W2 A W9 PD=5 B=3\n"
    "  W2 is using Attack tactic against W9.\n"
    "  W2 is powering up to powerdrive level 5\n"
    "  W2 is powering beams to level 3\n"
    " As the defender (owner of W9), set your orders:\n"
    " > CO W9 D W2 PD=4 B=1\n"
    "  W9 is using Dodge tactic against W2.\n"
    "  W9 is powering up to powerdrive level 4\n"
    "  W9 is powering beams to level 1\n";

bool HelpCommand::invoke(void)
{
    if (m_demo)
    {
        Telemetry::instance().write(demo_page);
    }
    else if (m_topic.empty())
    {
        Telemetry::instance().write(help_page);
    }
    else
    {
        // Look up topic from database
        DatabaseManager& db = DatabaseManager::instance();

        // Convert to lowercase for case-insensitive lookup
        for (auto& c : m_topic)
            c = std::tolower(c);

        // Special case: "topics" lists all available topics
        if (m_topic.compare(topics_keyword) == 0)
        {
            auto rows = db.query("SELECT DISTINCT topic_keyword FROM "
                                 "help_lookup ORDER BY topic_keyword");

            if (rows.empty())
            {
                Telemetry::instance().write("No topics available.\n");
                return true;
            }

            std::string out = "AVAILABLE HELP TOPICS:\n";
            int col = 0;
            for (const auto& row : rows)
            {
                out += row[0];
                col++;
                if (col >= 6)
                {
                    out += "\n";
                    col = 0;
                }
                else
                {
                    // Pad to 12 chars for column alignment
                    int pad = 12 - row[0].length();
                    for (int i = 0; i < pad && i < 12; i++)
                        out += " ";
                }
            }

            if (col > 0)
            {
                out += "\n";
            }
            Telemetry::instance().write(out);
            return true;
        }

        // Actually trying to find help on m_topic
        auto rows =
            db.query("SELECT t.topic_info FROM help_topics t "
                     "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
                     "WHERE l.topic_keyword = '" +
                     db.esc(m_topic) + "' LIMIT 1");

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
            Telemetry::instance().write(rows[0][0]);
        }
        else
        {
            Telemetry::instance().write("No help available for '" + m_topic +
                                           "'. Try: help topics");
        }
    }
    return true;
}
