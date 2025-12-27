-- Seed data for Kepler map (game_id=1)
-- Run this after schema.sql.
-- Recommended invocation from this directory:
--   mysql --local-infile=1 -u <user> -p borealis < schema.sql
--   mysql --local-infile=1 -u <user> -p borealis < seed.sql

START TRANSACTION;

-- DELETE FROM warplines WHERE game_id=1;
-- DELETE FROM star_systems WHERE game_id=1;

LOAD DATA LOCAL INFILE 'star_systems.csv'
INTO TABLE star_systems
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(game_id, hex_id, name, is_base, base_owner);

LOAD DATA LOCAL INFILE 'warplines.csv'
INTO TABLE warplines
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(game_id, a_hex, b_hex);

COMMIT;
