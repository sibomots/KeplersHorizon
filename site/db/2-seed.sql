-- Seed data for Kepler map (map_id=1)
-- Run this after schema.sql.
-- Recommended invocation from this directory:
--   mysql --local-infile=1 -u <user> -p borealis < schema.sql
--   mysql --local-infile=1 -u <user> -p borealis < seed.sql

use khdb;

START TRANSACTION;

-- DELETE FROM warplines WHERE map_id=1;
-- DELETE FROM star_systems WHERE map_id=1;

LOAD DATA LOCAL INFILE 'data/star_systems.csv'
INTO TABLE star_systems
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(map_id, hex_id, name, is_base, base_owner);

LOAD DATA LOCAL INFILE 'data/warplines.csv'
INTO TABLE warplines
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(map_id, a_hex, b_hex);


LOAD DATA LOCAL INFILE 'data/hexes.csv'
INTO TABLE hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '
'
(map_id, hex_id, q, r);

LOAD DATA LOCAL INFILE 'data/warpline_hexes.csv'
INTO TABLE warpline_hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '
'
(map_id, warpline_id, hex_id);


COMMIT;
