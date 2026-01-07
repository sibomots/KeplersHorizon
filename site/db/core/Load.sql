-- Core Data Loader
-- Loads core engine data that is not module-specific
-- Run from site/db/ with: mysql --local-infile=1 -u <user> -p khdb < core/Load.sql

USE khdb;

-- Help Topics (game command help)
DELETE FROM help_lookup;
DELETE FROM help_topics;

LOAD DATA LOCAL INFILE 'core/help/help_topics.csv'
INTO TABLE help_topics
FIELDS TERMINATED BY ','
ENCLOSED BY '"'
LINES TERMINATED BY '\n'
(help_topic_id, topic_info);

LOAD DATA LOCAL INFILE 'core/help/help_lookup.csv'
INTO TABLE help_lookup
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
IGNORE 1 ROWS
(topic_keyword, help_topic_id);
