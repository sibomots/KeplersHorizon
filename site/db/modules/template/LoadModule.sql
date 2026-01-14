-- Template Module Loader
-- Copy this file and customize for your module
-- Usage: mysql --local-infile=1 -u <user> -p khdb < modules/your_module/LoadModule.sql

USE khdb2;
START TRANSACTION;

-- Change this to your module_id from the modules table
SET @module = 2;

-- Map topology (copy from KH or create your own)
-- Uncomment and modify paths as needed:
-- LOAD DATA LOCAL INFILE 'modules/template/star_systems.csv'
-- INTO TABLE star_systems
-- FIELDS TERMINATED BY ','
-- LINES TERMINATED BY '\n'
-- (module_id, hex_id, name, is_base, base_owner, base_side, territory_name);

-- Milieu content
-- Each SET module_id = @module associates data with your module

COMMIT;
