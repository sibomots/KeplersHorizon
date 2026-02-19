///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#include "help_command.h"

#include "db.h"
#include "logger.h"
#include "telemetry.h"

static const std::string topics_keyword("topics");

static std::string help_page(LC_HELP_PAGE_INFO_BASIC);
static const std::string demo_page(LC_DEMO_PAGE_INFO);

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
        if (KH_EQU(m_topic.compare(topics_keyword), 0))
        {
            auto rows = db.Query("SELECT DISTINCT topic_keyword FROM "
                                 "help_lookup ORDER BY topic_keyword",
                                 {});

            if (rows.empty())
            {
                Telemetry::instance().write(LC_HELP_NO_TOPICS);
                return true;
            }

            std::string out = LC_HELP_TOPICS_BANNER ":\n";
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
        std::string q =
            "SELECT t.topic_info FROM help_topics t "
            "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
            " WHERE l.topic_keyword =? LIMIT 1";
        auto rows = db.Query(q, {m_topic});

        if (rows.empty())
        {
            // Fallback to 'general' topic
            rows = db.Query(
                "SELECT t.topic_info FROM help_topics t "
                "JOIN help_lookup l ON l.help_topic_id = t.help_topic_id "
                "WHERE l.topic_keyword = 'general' LIMIT 1",
                {});
        }

        if (!rows.empty() && !rows[0].empty())
        {
            Telemetry::instance().write(rows[0][0]);
        }
        else
        {
            Telemetry::instance().write(
                 std::format(LC_HELP_TARGET_NO_TOPIC,
                          m_topic));
        }
    }
    return true;
}
