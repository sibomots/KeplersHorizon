-- Kepler's Horizon Module Loader
-- This script loads all milieu data for the Kepler's Horizon module (module_id=1)
-- Run from site/db/ with: mysql --local-infile=1 -u <user> -p khdb < modules/kh/LoadModule.sql

USE khdb;
START TRANSACTION;

SET @module = 1;

-- Map topology (shared with template)
LOAD DATA LOCAL INFILE 'modules/kh/star_systems.csv'
INTO TABLE star_systems
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, hex_id, name, is_base, base_owner, base_side, territory_name);

LOAD DATA LOCAL INFILE 'modules/kh/warplines.csv'
INTO TABLE warplines
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, a_hex, b_hex);

LOAD DATA LOCAL INFILE 'modules/kh/hexes.csv'
INTO TABLE hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, hex_id, q, r);

LOAD DATA LOCAL INFILE 'modules/kh/warpline_hexes.csv'
INTO TABLE warpline_hexes
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(module_id, warpline_id, hex_id);

-- Milieu content (unique to this module)
LOAD DATA LOCAL INFILE 'modules/kh/anomalies.csv'
INTO TABLE system_anomalies
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,anomaly_type,name,effect,discovery_text)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/asteroid_belts.csv'
INTO TABLE system_asteroid_belts
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,designation,composition,density,notable_feature,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/facilities.csv'
INTO TABLE system_facilities
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,facility_type,name,capacity,owner,operational,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/grimoire_rumors.csv'
INTO TABLE system_grimoire_rumors
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(system_name,rumor_text,knowledge_level)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/moons.csv'
INTO TABLE system_moons
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,planet_id,designation,common_name,moon_type,size,notable_feature,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/planets.csv'
INTO TABLE system_planets
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,star_id,orbital_position,designation,common_name,planet_type,atmosphere,hydrosphere,biosphere,habitability,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/populations.csv'
INTO TABLE system_populations
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,species_id,pop_class,population_millions,tech_level,government,disposition,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/resources.csv'
INTO TABLE system_resources
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,resource_type,abundance,extraction_difficulty,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/species.csv'
INTO TABLE system_species
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,name,classification,homeworld_system,physiology,psychology,special_ability)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/stars.csv'
INTO TABLE system_stars
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,designation,star_class,luminosity,color,age_gy,notes)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/salvageables.csv'
INTO TABLE salvageables
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,name,description,discovery_chance,hazard_chance,hazard_damage_min,hazard_damage_max,max_salvages)
SET module_id = @module;

LOAD DATA LOCAL INFILE 'modules/kh/salvageable_drops.csv'
INTO TABLE salvageable_drops
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,salvageable_id,item_type,item_name,drop_chance,quantity_min,quantity_max);

LOAD DATA LOCAL INFILE 'modules/kh/facility_control_initial.csv'
INTO TABLE facility_control_initial
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
IGNORE 1 LINES
(system_name, facility_type, controller)
SET module_id = @module;

COMMIT;
