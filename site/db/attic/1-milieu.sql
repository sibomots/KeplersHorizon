-- OK
use khdb;

DROP TABLE IF EXISTS grimoire_entries;
DROP TABLE IF EXISTS system_grimoire_rumors;
DROP TABLE IF EXISTS system_anomalies;
DROP TABLE IF EXISTS system_facilities;
DROP TABLE IF EXISTS system_populations;
DROP TABLE IF EXISTS system_resources;
DROP TABLE IF EXISTS system_moons;
DROP TABLE IF EXISTS system_asteroid_belts;
DROP TABLE IF EXISTS system_planets;
DROP TABLE IF EXISTS system_stars;
DROP TABLE IF EXISTS system_species;

CREATE TABLE IF NOT EXISTS system_species (
id INT AUTO_INCREMENT PRIMARY KEY,
name VARCHAR(64) NOT NULL UNIQUE,
classification ENUM('Human','NearHuman','Xenoform','Machine','Elder') NOT NULL,
homeworld_system VARCHAR(64) DEFAULT NULL,
physiology TEXT,
psychology TEXT,
special_ability VARCHAR(128),
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS system_stars (
id INT AUTO_INCREMENT PRIMARY KEY,
system_name VARCHAR(64) NOT NULL,
designation VARCHAR(32) NOT NULL,
star_class CHAR(1) NOT NULL,
luminosity VARCHAR(16) NOT NULL,
color VARCHAR(16),
age_gy DECIMAL(4,1),
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_system (system_name)
);

CREATE TABLE IF NOT EXISTS system_planets (
id INT AUTO_INCREMENT PRIMARY KEY,
system_name VARCHAR(64) NOT NULL,
star_id INT,
orbital_position INT NOT NULL,
designation VARCHAR(32) NOT NULL,
common_name VARCHAR(64),
planet_type ENUM('Rocky','GasGiant','IceGiant','Dwarf') NOT NULL,
atmosphere ENUM('None','Thin','Standard','Dense','Toxic','Exotic'),
hydrosphere ENUM('None','Trace','Moderate','Ocean','Ice'),
biosphere ENUM('None','Microbial','Primitive','Complex','Sapient'),
habitability ENUM('Hostile','Marginal','Comfortable','Garden'),
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_system (system_name),
INDEX idx_star (star_id)
);

CREATE TABLE IF NOT EXISTS system_moons (
id INT AUTO_INCREMENT PRIMARY KEY,
planet_id INT NOT NULL,
designation VARCHAR(32) NOT NULL,
common_name VARCHAR(64),
moon_type ENUM('Rocky','Ice','Captured','RingShepherd') NOT NULL,
size ENUM('Asteroid','Small','Medium','Large') NOT NULL,
notable_feature VARCHAR(128),
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_planet (planet_id)
);

CREATE TABLE IF NOT EXISTS system_asteroid_belts (
id INT AUTO_INCREMENT PRIMARY KEY,
system_name VARCHAR(64) NOT NULL,
designation VARCHAR(32) NOT NULL,
orbital_position DECIMAL(3,1) NOT NULL,
density ENUM('Sparse','Moderate','Dense','Extreme') NOT NULL,
composition ENUM('Silicate','Carbonaceous','Metallic','Mixed','Ice') NOT NULL,
hazard_level ENUM('None','Minor','Significant','Severe') NOT NULL,
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_system (system_name)
);

CREATE TABLE IF NOT EXISTS system_resources (
id INT AUTO_INCREMENT PRIMARY KEY,
location_type ENUM('Planet','Moon','Belt','System') NOT NULL,
location_id INT NOT NULL,
resource_type ENUM('FERROUS','RARE_EARTH','RADIOACTIVE','CRYSTALLINE',
'VOLATILE','WATER','ORGANIC','EXOTIC') NOT NULL,
abundance ENUM('Trace','Low','Moderate','High','Rich') NOT NULL,
extraction_difficulty ENUM('Easy','Moderate','Difficult','Extreme'),
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_location (location_type, location_id),
INDEX idx_resource_type (resource_type)
);

CREATE TABLE IF NOT EXISTS system_populations (
id INT AUTO_INCREMENT PRIMARY KEY,
location_type ENUM('Planet','Moon','Station') NOT NULL,
location_id INT NOT NULL,
species_id INT NOT NULL,
pop_class ENUM('Outpost','Settlement','Colony','City','Metropolis') NOT NULL,
population_millions DECIMAL(8,2),
tech_level INT NOT NULL,
government VARCHAR(64),
disposition ENUM('Hostile','Wary','Neutral','Friendly','Allied') NOT NULL,
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_location (location_type, location_id),
INDEX idx_species (species_id)
);

CREATE TABLE IF NOT EXISTS system_facilities (
id INT AUTO_INCREMENT PRIMARY KEY,
location_type ENUM('Planet','Moon','Belt','Orbit') NOT NULL,
location_id INT NOT NULL,
facility_type ENUM('SHIPYARD','REPAIR_DOCK','REFINERY','TRADE_HUB',
'FORTRESS','BEACON','MINING_STATION','RESEARCH_LAB','ORBITAL_PLATFORM') NOT NULL,
name VARCHAR(64) NOT NULL,
capacity INT,
owner ENUM('A','B','Neutral','Contested','Abandoned') NOT NULL,
operational BOOLEAN NOT NULL DEFAULT TRUE,
notes TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_location (location_type, location_id),
INDEX idx_facility_type (facility_type),
INDEX idx_owner (owner)
);

CREATE TABLE IF NOT EXISTS system_anomalies (
id INT AUTO_INCREMENT PRIMARY KEY,
system_name VARCHAR(64) NOT NULL,
anomaly_type ENUM('DERELICT','ARTIFACT','SPATIAL_RIFT','RADIATION_ZONE',
'DARK_NEBULA','GRAVEYARD','BEACON_ANCIENT','PHENOMENON') NOT NULL,
name VARCHAR(64) NOT NULL,
effect TEXT,
discovery_text TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_system (system_name),
INDEX idx_anomaly_type (anomaly_type)
);

CREATE TABLE IF NOT EXISTS system_grimoire_rumors (
id INT AUTO_INCREMENT PRIMARY KEY,
system_name VARCHAR(64) NOT NULL,
rumor_text TEXT NOT NULL,
knowledge_level ENUM('Unknown','Rumored','Charted','Surveyed','Intimate') 
NOT NULL DEFAULT 'Rumored',
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
INDEX idx_system (system_name)
);

CREATE TABLE IF NOT EXISTS grimoire_entries (
game_id INT NOT NULL,
player ENUM('A','B') NOT NULL,
system_name VARCHAR(64) NOT NULL,
knowledge_level ENUM('Unknown','Rumored','Charted','Surveyed','Intimate') 
NOT NULL DEFAULT 'Unknown',
last_updated_turn VARCHAR(8),
notes TEXT,
PRIMARY KEY (game_id, player, system_name),
INDEX idx_game_player (game_id, player)
);

