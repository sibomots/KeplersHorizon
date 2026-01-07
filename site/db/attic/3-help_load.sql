-- Help Topics Loader
-- Uses LOAD DATA to import CSV files

USE khdb;

-- Clear existing data
DELETE FROM help_lookup;
DELETE FROM help_topics;

-- Load topics (multi-line text fields)
LOAD DATA LOCAL INFILE 'help/help_topics.csv'
INTO TABLE help_topics
FIELDS TERMINATED BY ','
ENCLOSED BY '"'
LINES TERMINATED BY '\n'
(help_topic_id, topic_info);

-- Load keyword mappings
LOAD DATA LOCAL INFILE 'help/help_lookup.csv'
INTO TABLE help_lookup
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
IGNORE 1 ROWS
(topic_keyword, help_topic_id);
