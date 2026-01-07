-- Module Schema Tables (for any module)
USE khdb;

CREATE TABLE IF NOT EXISTS star_systems (
module_id INT NOT NULL DEFAULT 1,
hex_id VARCHAR(8) NOT NULL,
name VARCHAR(64) NOT NULL,
is_base TINYINT NOT NULL DEFAULT 0,
base_owner CHAR(1) NULL,
base_side CHAR(1) NULL,
territory_name VARCHAR(64) NULL,
PRIMARY KEY (module_id, name),
INDEX (module_id, hex_id),
INDEX (module_id, is_base),
INDEX (module_id, base_owner),
INDEX (module_id, base_side),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS warplines (
module_id INT NOT NULL DEFAULT 1,
id INT NOT NULL AUTO_INCREMENT,
a_hex VARCHAR(8) NOT NULL,
b_hex VARCHAR(8) NOT NULL,
PRIMARY KEY (id),
INDEX (module_id),
INDEX (module_id, a_hex),
INDEX (module_id, b_hex),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS hexes (
module_id INT NOT NULL DEFAULT 1,
hex_id VARCHAR(8) NOT NULL,
q INT NOT NULL,
r INT NOT NULL,
PRIMARY KEY (module_id, hex_id),
INDEX (module_id, q),
INDEX (module_id, r),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS warpline_hexes (
module_id INT NOT NULL DEFAULT 1,
warpline_id INT NOT NULL,
hex_id VARCHAR(8) NOT NULL,
PRIMARY KEY (module_id, warpline_id, hex_id),
INDEX (module_id, hex_id),
INDEX (module_id, warpline_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

-- Module content tables: planetary data
CREATE TABLE IF NOT EXISTS system_planets (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
star_id INT,
orbital_position INT,
designation VARCHAR(32),
common_name VARCHAR(64),
planet_type VARCHAR(16),
atmosphere VARCHAR(16),
hydrosphere VARCHAR(16),
biosphere VARCHAR(16),
habitability VARCHAR(16),
notes TEXT,
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_moons (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
planet_id INT NOT NULL,
designation VARCHAR(32),
common_name VARCHAR(64),
moon_type VARCHAR(16),
size VARCHAR(16),
notable_feature VARCHAR(64),
notes TEXT,
INDEX (module_id, planet_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_stars (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
designation VARCHAR(64),
star_class CHAR(1),
luminosity VARCHAR(16),
color VARCHAR(16),
age_gy DECIMAL(4,1),
notes TEXT,
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_anomalies (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
anomaly_type VARCHAR(32),
name VARCHAR(64),
effect TEXT,
discovery_text TEXT,
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_facilities (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
location_type VARCHAR(16),
location_id INT,
facility_type VARCHAR(32),
name VARCHAR(64),
capacity INT DEFAULT 0,
owner CHAR(1),
operational TINYINT DEFAULT 1,
notes TEXT,
INDEX (module_id, location_type, location_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_resources (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
location_type VARCHAR(16),
location_id INT,
resource_type VARCHAR(32),
abundance VARCHAR(16),
extraction_difficulty VARCHAR(16),
notes TEXT,
INDEX (module_id, location_type, location_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_populations (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
location_type VARCHAR(16),
location_id INT,
species_id INT,
pop_class VARCHAR(16),
population_millions DECIMAL(10,2),
tech_level INT,
government VARCHAR(64),
disposition VARCHAR(16),
notes TEXT,
INDEX (module_id, location_type, location_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_species (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
name VARCHAR(64) NOT NULL,
classification VARCHAR(16),
homeworld_system VARCHAR(64),
physiology TEXT,
psychology TEXT,
special_ability TEXT,
INDEX (module_id, name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_grimoire_rumors (
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
rumor_text TEXT,
knowledge_level VARCHAR(16),
PRIMARY KEY (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS market_base_prices (
module_id INT NOT NULL DEFAULT 1,
resource_type VARCHAR(32) NOT NULL,
base_price INT NOT NULL,
PRIMARY KEY (module_id, resource_type),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS system_asteroid_belts (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
designation VARCHAR(64),
composition VARCHAR(32),
density VARCHAR(16),
notable_feature VARCHAR(64),
notes TEXT,
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS facility_control_initial (
module_id INT NOT NULL DEFAULT 1,
facility_id INT NOT NULL,
initial_owner CHAR(1),
PRIMARY KEY (module_id, facility_id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS combat_state (
game_id INT NOT NULL,
hex_id VARCHAR(8) NOT NULL,
round INT NOT NULL DEFAULT 1,
stage INT NOT NULL DEFAULT 0,
attacker_remains BOOLEAN NOT NULL DEFAULT 0,
stalemate_counter INT NOT NULL DEFAULT 0,
pending_damage_json TEXT DEFAULT NULL,
damage_assigned_A BOOLEAN NOT NULL DEFAULT 0,
damage_assigned_B BOOLEAN NOT NULL DEFAULT 0,
last_log TEXT DEFAULT NULL,
PRIMARY KEY (game_id, hex_id),
FOREIGN KEY (game_id) REFERENCES games(id)
);

CREATE TABLE IF NOT EXISTS combat_orders (
game_id INT NOT NULL,
ship_code VARCHAR(4) NOT NULL,
owner CHAR(1) NOT NULL,
round INT NOT NULL,
tactic CHAR(1) NOT NULL DEFAULT 'A',
target_id VARCHAR(4) DEFAULT NULL,
power_d INT NOT NULL DEFAULT 0,
power_b INT NOT NULL DEFAULT 0,
power_s INT NOT NULL DEFAULT 0,
power_t INT NOT NULL DEFAULT 0,
missiles_data TEXT DEFAULT NULL,
committed BOOLEAN NOT NULL DEFAULT 0,
PRIMARY KEY (game_id, owner, ship_code, round),
FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
FOREIGN KEY (game_id, owner, ship_code) REFERENCES ships(game_id, owner,
ship_code) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS telemetry_queue (
id BIGINT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
target_player ENUM('A','B','BOTH') NOT NULL,
message TEXT NOT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
sent_at TIMESTAMP NULL DEFAULT NULL,
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX (game_id, target_player, sent_at)
);

-- Economy Tables

CREATE TABLE IF NOT EXISTS extract_operations (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
ship_code VARCHAR(4) NOT NULL,
owner CHAR(1) NOT NULL,
location_type VARCHAR(16) NOT NULL,
location_id INT NOT NULL,
resource_type VARCHAR(32) NOT NULL,
started_turn INT NOT NULL,
completed BOOLEAN DEFAULT FALSE,
yield INT DEFAULT 0,
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX (game_id, owner)
);

CREATE TABLE IF NOT EXISTS fabrication_queue (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
player ENUM('A','B') NOT NULL,
ship_code VARCHAR(4) DEFAULT NULL,
recipe VARCHAR(32) NOT NULL,
quantity INT DEFAULT 1,
started_turn INT NOT NULL,
completion_turn INT NOT NULL,
materials_consumed TEXT,
status ENUM('QUEUED','IN_PROGRESS','COMPLETED','CANCELLED') DEFAULT 'QUEUED',
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX (game_id, player)
);

CREATE TABLE IF NOT EXISTS system_constraints (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
constraint_type ENUM('MOVEMENT', 'COMBAT', 'TRADE', 'HARVEST', 'BUILD') NOT NULL,
modifier_type ENUM('BONUS', 'PENALTY', 'BLOCK') NOT NULL,
modifier_value INT DEFAULT 0,
condition_text TEXT,
source VARCHAR(64),
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

-- Market dynamic pricing
CREATE TABLE IF NOT EXISTS market_prices (
game_id INT NOT NULL,
resource_type VARCHAR(16) NOT NULL,
current_price INT NOT NULL,
base_price INT NOT NULL,
price_trend ENUM('RISING','STABLE','FALLING') DEFAULT 'STABLE',
total_bought INT NOT NULL DEFAULT 0,
total_sold INT NOT NULL DEFAULT 0,
last_updated_turn INT NOT NULL DEFAULT 1,
PRIMARY KEY (game_id, resource_type),
FOREIGN KEY (game_id) REFERENCES games(id)
);

CREATE TABLE IF NOT EXISTS market_history (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
resource_type VARCHAR(16) NOT NULL,
price INT NOT NULL,
turn INT NOT NULL,
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX (game_id, resource_type, turn)
);

-- Resource depletion and regeneration
CREATE TABLE IF NOT EXISTS resource_state (
game_id INT NOT NULL,
resource_id INT NOT NULL,
current_supply INT NOT NULL,
max_supply INT NOT NULL,
regen_rate INT NOT NULL DEFAULT 1,
last_extracted_turn INT DEFAULT NULL,
PRIMARY KEY (game_id, resource_id),
FOREIGN KEY (game_id) REFERENCES games(id)
);

-- Facility ownership (per game, copied from global facilities)
CREATE TABLE IF NOT EXISTS facility_control (
game_id INT NOT NULL,
system_name VARCHAR(64) NOT NULL,
facility_type VARCHAR(32) NOT NULL,
controller CHAR(1) DEFAULT NULL,   -- NULL = neutral, A or B = player
occupied_since INT DEFAULT NULL,   -- Turn when occupation began
capture_progress INT DEFAULT 0,    -- Turns of continuous occupation
PRIMARY KEY (game_id, system_name, facility_type),
FOREIGN KEY (game_id) REFERENCES games(id)
);

-- Anomaly events
CREATE TABLE IF NOT EXISTS anomaly_events (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
system_name VARCHAR(64) NOT NULL,
anomaly_name VARCHAR(64) NOT NULL,
event_type VARCHAR(32) NOT NULL,   -- SALVAGE, SCAN, DISCOVER, etc.
player CHAR(1) NOT NULL,
ship_code VARCHAR(4) NOT NULL,
turn INT NOT NULL,
result_json TEXT,                  -- What was found/gained
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX (game_id, system_name)
);

-- Salvage System: Data-driven salvageables

-- What can be salvaged (seeded milieu)
CREATE TABLE IF NOT EXISTS salvageables (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
name VARCHAR(64) NOT NULL,
description TEXT,
discovery_chance INT DEFAULT 100,
hazard_chance INT DEFAULT 10,
hazard_damage_min INT DEFAULT 1,
hazard_damage_max INT DEFAULT 2,
max_salvages INT DEFAULT NULL,
INDEX (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

-- What each salvageable yields
CREATE TABLE IF NOT EXISTS salvageable_drops (
id INT AUTO_INCREMENT PRIMARY KEY,
salvageable_id INT NOT NULL,
item_type VARCHAR(16) NOT NULL,
item_name VARCHAR(32) NOT NULL,
drop_chance INT DEFAULT 100,
quantity_min INT DEFAULT 1,
quantity_max INT DEFAULT 5,
FOREIGN KEY (salvageable_id) REFERENCES salvageables(id)
);

-- Per-game discovery and depletion state
CREATE TABLE IF NOT EXISTS discovered_salvageables (
game_id INT NOT NULL,
salvageable_id INT NOT NULL,
discovered_by CHAR(1),
discovered_turn INT,
times_salvaged INT DEFAULT 0,
depleted BOOLEAN DEFAULT FALSE,
PRIMARY KEY (game_id, salvageable_id),
FOREIGN KEY (game_id) REFERENCES games(id),
FOREIGN KEY (salvageable_id) REFERENCES salvageables(id)
);

-- Salvage operation log
CREATE TABLE IF NOT EXISTS salvage_operations (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
system_name VARCHAR(64) NOT NULL,
ship_code VARCHAR(4) NOT NULL,
turn INT NOT NULL,
resources_found TEXT,              -- JSON of resources gained
hazard_encountered BOOLEAN DEFAULT FALSE,
FOREIGN KEY (game_id) REFERENCES games(id)
);

-- Help Topics System
CREATE TABLE IF NOT EXISTS help_topics (
help_topic_id INT AUTO_INCREMENT PRIMARY KEY,
topic_info TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS help_lookup (
help_lookup_id INT AUTO_INCREMENT PRIMARY KEY,
topic_keyword VARCHAR(64) NOT NULL,
help_topic_id INT NOT NULL,
FOREIGN KEY (help_topic_id) REFERENCES help_topics(help_topic_id),
INDEX (topic_keyword)
);


-- Saved Games Schema
-- Bookmark model: saved game is just a pointer to an existing game_id
-- No state copying - all data remains in DB under the original game_id

CREATE TABLE IF NOT EXISTS saved_games (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    save_name VARCHAR(64) NOT NULL,
    game_id INT NOT NULL,
    saved_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uniq_user_save (user_id, save_name),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
    INDEX idx_user_saves (user_id, saved_at DESC)
);

-- Pending load requests for two-factor confirmation
-- Only one pending request per active game
CREATE TABLE IF NOT EXISTS load_requests (
    game_id INT NOT NULL PRIMARY KEY,
    requester CHAR(1) NOT NULL,
    requester_user_id INT NOT NULL,
    target_game_id INT NOT NULL,
    save_name VARCHAR(64) NOT NULL,
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
    FOREIGN KEY (requester_user_id) REFERENCES users(id),
    FOREIGN KEY (target_game_id) REFERENCES games(id)
);
