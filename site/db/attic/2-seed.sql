-- Seed data for Kepler Module (module_id=1)
-- Run this after 0-schema.sql.
-- Recommended invocation from this directory:
--   mysql --local-infile=1 -u <user> -p khdb < 0-schema.sql
--   mysql --local-infile=1 -u <user> -p khdb < 2-seed.sql

use khdb;

START TRANSACTION;

-- DELETE FROM warplines WHERE module_id=1;
-- DELETE FROM star_systems WHERE module_id=1;

LOAD DATA LOCAL INFILE 'data/star_systems.csv'
INTO TABLE star_systems
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, hex_id, name, is_base, base_owner, base_side, territory_name);

LOAD DATA LOCAL INFILE 'data/warplines.csv'
INTO TABLE warplines
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, a_hex, b_hex);

LOAD DATA LOCAL INFILE 'data/hexes.csv'
INTO TABLE hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, hex_id, q, r);

LOAD DATA LOCAL INFILE 'data/warpline_hexes.csv'
INTO TABLE warpline_hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, warpline_id, hex_id);

COMMIT;
