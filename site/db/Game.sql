-- Game.sql - Complete database schema for Kepler's Horizon
-- Run this to create/reset the database
-- Module data (CSV files) loaded separately as admin choice

USE khdb;

CREATE TABLE IF NOT EXISTS users (
id INT AUTO_INCREMENT PRIMARY KEY,
username VARCHAR(64) NOT NULL UNIQUE,
email VARCHAR(255) UNIQUE,
password_plain VARCHAR(128),
password_hash VARCHAR(255),
is_admin TINYINT NOT NULL DEFAULT 0,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT IGNORE INTO users(username,password_plain)
VALUES('alice','alicepw'),('bob','bobpw');

CREATE TABLE IF NOT EXISTS rooms (
id INT AUTO_INCREMENT PRIMARY KEY,
room_code VARCHAR(8) NOT NULL UNIQUE,
name VARCHAR(64),
created_by INT NOT NULL,
seat_a INT DEFAULT NULL,
seat_b INT DEFAULT NULL,
game_id INT DEFAULT NULL,
status ENUM('waiting','ready','playing','finished') DEFAULT 'waiting',
module_id INT NOT NULL DEFAULT 1,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
FOREIGN KEY (created_by) REFERENCES users(id),
FOREIGN KEY (seat_a) REFERENCES users(id),
FOREIGN KEY (seat_b) REFERENCES users(id),
INDEX (status),
INDEX (room_code)
);

CREATE TABLE IF NOT EXISTS sessions (
token CHAR(64) PRIMARY KEY,
user_id INT NOT NULL,
game_id INT DEFAULT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Module = Complete universe definition
-- All milieu tables are keyed by module_id
CREATE TABLE IF NOT EXISTS modules (
module_id INT AUTO_INCREMENT PRIMARY KEY,
name VARCHAR(64) NOT NULL,
description TEXT,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert default module
INSERT INTO modules (module_id, name, description)
VALUES (1, 'Kepler''s Horizon', 'The original frontier universe - humanity''s expansion into the stars');

CREATE TABLE IF NOT EXISTS games (
id INT AUTO_INCREMENT PRIMARY KEY,
room_id INT DEFAULT NULL,
module_id INT NOT NULL DEFAULT 1,
state_json MEDIUMTEXT NOT NULL,
current_draft_A VARCHAR(4) DEFAULT NULL,
current_draft_B VARCHAR(4) DEFAULT NULL,
active_combat_hex VARCHAR(8) DEFAULT NULL,
vp_A INT NOT NULL DEFAULT 0,  -- Victory Points for Player A
vp_B INT NOT NULL DEFAULT 0,  -- Victory Points for Player B
winner CHAR(1) DEFAULT NULL,  -- 'A', 'B', or NULL if ongoing
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
FOREIGN KEY (room_id) REFERENCES rooms(id),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS game_seats (
game_id INT NOT NULL,
user_id INT NOT NULL,
seat CHAR(1) NOT NULL,
PRIMARY KEY (game_id, seat),
UNIQUE KEY uniq_user_game (game_id, user_id),
FOREIGN KEY (game_id) REFERENCES games(id),
FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS game_events (
id BIGINT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
user_id INT NOT NULL,
seq INT NOT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
command_text VARCHAR(256) NOT NULL,
result_text MEDIUMTEXT NOT NULL,
state_json MEDIUMTEXT NOT NULL,
FOREIGN KEY (game_id) REFERENCES games(id),
FOREIGN KEY (user_id) REFERENCES users(id),
UNIQUE KEY uniq_game_seq (game_id, seq)
);

CREATE TABLE IF NOT EXISTS drafts (
id BIGINT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
owner CHAR(1) NOT NULL,
ship_code VARCHAR(4) NOT NULL,
ship_name VARCHAR(32) NOT NULL,
ship_type CHAR(1) NOT NULL,
pd INT NOT NULL DEFAULT 0,
beam INT NOT NULL DEFAULT 0,
screen INT NOT NULL DEFAULT 0,
tube INT NOT NULL DEFAULT 0,
missiles INT NOT NULL DEFAULT 0,
sr INT NOT NULL DEFAULT 0,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
UNIQUE KEY uniq_draft (game_id, owner, ship_code),
FOREIGN KEY (game_id) REFERENCES games(id)
);

CREATE TABLE IF NOT EXISTS ships (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
owner CHAR(1) NOT NULL,
ship_code VARCHAR(4) NOT NULL,
ship_name VARCHAR(32) NOT NULL,
ship_type CHAR(1) NOT NULL,
tech_level INT NOT NULL DEFAULT 0,
built_turn VARCHAR(8) NOT NULL,
pd INT NOT NULL DEFAULT 0,
beam INT NOT NULL DEFAULT 0,
screen INT NOT NULL DEFAULT 0,
tube INT NOT NULL DEFAULT 0,
missiles INT NOT NULL DEFAULT 0,
sr INT NOT NULL DEFAULT 0,
-- Extraction equipment (don't affect destruction)
lrs INT NOT NULL DEFAULT 0,   -- Long Range Scanner
tb INT NOT NULL DEFAULT 0,    -- Transporter Beam
pd_spent INT NOT NULL DEFAULT 0,
at_system VARCHAR(16) DEFAULT NULL,
at_hex VARCHAR(8) DEFAULT NULL,
racked_in VARCHAR(4) DEFAULT NULL,
destroyed_at TIMESTAMP NULL DEFAULT NULL,
escape_pending TINYINT NOT NULL DEFAULT 0,  -- Flag for pending retreat
-- Cargo holds for resources
cargo_ferrous INT NOT NULL DEFAULT 0,
cargo_rare_earth INT NOT NULL DEFAULT 0,
cargo_radioactive INT NOT NULL DEFAULT 0,
cargo_crystalline INT NOT NULL DEFAULT 0,
cargo_volatile INT NOT NULL DEFAULT 0,
cargo_water INT NOT NULL DEFAULT 0,
cargo_organic INT NOT NULL DEFAULT 0,
cargo_exotic INT NOT NULL DEFAULT 0,
cargo_missiles INT NOT NULL DEFAULT 0,
cargo_capacity INT NOT NULL DEFAULT 10,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
UNIQUE KEY uniq_ship (game_id, owner, ship_code),
FOREIGN KEY (game_id) REFERENCES games(id)
);


ALTER TABLE ships 
ADD COLUMN pd_max INT NOT NULL DEFAULT 0 COMMENT 'Original Power Drive rating' AFTER pd,
ADD COLUMN beam_max INT NOT NULL DEFAULT 0 COMMENT 'Original Beam rating' AFTER beam,
ADD COLUMN screen_max INT NOT NULL DEFAULT 0 COMMENT 'Original Screen rating' AFTER screen,
ADD COLUMN tube_max INT NOT NULL DEFAULT 0 COMMENT 'Original Tube rating' AFTER tube,
ADD COLUMN missiles_max INT NOT NULL DEFAULT 0 COMMENT 'Original Missile capacity' AFTER missiles,
ADD COLUMN sr_max INT NOT NULL DEFAULT 0 COMMENT 'Original System Rack rating' AFTER sr;


CREATE TABLE IF NOT EXISTS sightings (
id BIGINT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
observer_owner CHAR(1) NOT NULL,
subject_owner CHAR(1) NOT NULL,
ship_code VARCHAR(4) NOT NULL,
ship_name VARCHAR(32) NOT NULL,
ship_type CHAR(1) NOT NULL,
at_system VARCHAR(16) NOT NULL,
last_seen_turn VARCHAR(8) NOT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
UNIQUE KEY uniq_sighting (game_id, observer_owner, ship_code),
FOREIGN KEY (game_id) REFERENCES games(id)
);


-- =====================================================
-- MODULE DEFINITION TABLES
-- Tables keyed by module_id for universe customization
-- =====================================================

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

CREATE TABLE IF NOT EXISTS system_codex_rumors (
module_id INT NOT NULL DEFAULT 1,
system_name VARCHAR(64) NOT NULL,
rumor_text TEXT,
knowledge_level VARCHAR(16),
PRIMARY KEY (module_id, system_name),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

-- Per-game player knowledge tracking for Milieu Codex of Stars
CREATE TABLE IF NOT EXISTS codex_entries (
game_id INT NOT NULL,
player CHAR(1) NOT NULL,
system_name VARCHAR(64) NOT NULL,
knowledge_level VARCHAR(16) NOT NULL DEFAULT 'Unknown',
last_updated_turn INT,
notes TEXT,
PRIMARY KEY (game_id, player, system_name),
FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
INDEX idx_game_player (game_id, player)
);

-- Per-game configuration (loaded from kh.conf via configure command)
CREATE TABLE IF NOT EXISTS game_config (
game_id INT NOT NULL,
config_key VARCHAR(64) NOT NULL,
config_value VARCHAR(128) NOT NULL DEFAULT '',
PRIMARY KEY (game_id, config_key),
FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS market_base_prices (
module_id INT NOT NULL DEFAULT 1,
resource_type VARCHAR(32) NOT NULL,
base_price INT NOT NULL,
PRIMARY KEY (module_id, resource_type),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

INSERT INTO market_base_prices (module_id, resource_type, base_price) VALUES
(1, 'FERROUS', 5),
(1, 'RARE_EARTH', 20),
(1, 'RADIOACTIVE', 30),
(1, 'CRYSTALLINE', 25),
(1, 'VOLATILE', 8),
(1, 'WATER', 3),
(1, 'ORGANIC', 6),
(1, 'EXOTIC', 100);

CREATE TABLE IF NOT EXISTS equipment_catalog (
module_id INT NOT NULL DEFAULT 1,
equipment_type VARCHAR(16) NOT NULL,
description VARCHAR(64) NOT NULL,
price INT NOT NULL,
ship_column VARCHAR(16) NOT NULL,
PRIMARY KEY (module_id, equipment_type),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

INSERT INTO equipment_catalog (module_id, equipment_type, description, price, ship_column) VALUES
(1, 'LRS', 'Long Range Scanner', 50, 'lrs'),
(1, 'TB', 'Transporter Beam', 75, 'tb');

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
system_name VARCHAR(64) NOT NULL,
facility_type VARCHAR(32) NOT NULL,
controller CHAR(1) DEFAULT NULL,
PRIMARY KEY (module_id, system_name, facility_type),
FOREIGN KEY (module_id) REFERENCES modules(module_id)
);

CREATE TABLE IF NOT EXISTS combat_state (
game_id INT NOT NULL,
hex_id VARCHAR(8) NOT NULL,
round INT NOT NULL DEFAULT 1,
stage INT NOT NULL DEFAULT 0,
attacker_remains BOOLEAN NOT NULL DEFAULT 0,
stalemate_counter INT NOT NULL DEFAULT 0,
-- REMOVED:  pending_damage_json TEXT DEFAULT NULL,
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
-- Per-player delivery tracking for BOTH messages (avoids race condition)
sent_to_A BOOLEAN DEFAULT FALSE,
sent_to_B BOOLEAN DEFAULT FALSE,
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

CREATE TABLE IF NOT EXISTS fabrication_plan (
id INT AUTO_INCREMENT PRIMARY KEY,
module_id INT NOT NULL DEFAULT 1,
name VARCHAR(32) NOT NULL UNIQUE,
description VARCHAR(128) NOT NULL,
build_time INT DEFAULT 1,
cost_ferrous INT DEFAULT 0,
cost_rare_earth INT DEFAULT 0,
cost_radioactive INT DEFAULT 0,
cost_crystalline INT DEFAULT 0,
cost_volatile INT DEFAULT 0,
cost_water INT DEFAULT 0,
cost_organic INT DEFAULT 0,
cost_exotic INT DEFAULT 0,
output_qty INT DEFAULT 1,
output_type ENUM('MISSILE','TUBE','BEAM','SCREEN','TECH') NOT NULL,
FOREIGN KEY (module_id) REFERENCES modules(module_id),
INDEX (module_id, name),
INDEX (output_type)
);

INSERT INTO fabrication_plan
(name, description, build_time, cost_ferrous, cost_radioactive, cost_volatile, output_qty, output_type)
VALUES
('missiles', 'Basic Missiles (x4)', 1, 2, 1, 1, 4, 'MISSILE');

INSERT INTO fabrication_plan
(name, description, build_time, cost_ferrous, cost_rare_earth, cost_crystalline, output_qty, output_type)
VALUES
('tubes', 'Tube Upgrade (+1 capacity)', 3, 5, 3, 2, 1, 'TUBE');

INSERT INTO fabrication_plan
(name, description, build_time, cost_ferrous, cost_rare_earth, cost_crystalline, output_qty, output_type)
VALUES
('beams', 'Beam Upgrade (+1 rating)', 3, 8, 4, 3, 1, 'BEAM');

INSERT INTO fabrication_plan
(name, description, build_time, cost_ferrous, cost_rare_earth, cost_crystalline, output_qty, output_type)
VALUES
('screens', 'Screen Upgrade (+1 rating)', 3, 6, 2, 4, 1, 'SCREEN');

INSERT INTO fabrication_plan
(name, description, build_time, cost_rare_earth, cost_crystalline, cost_exotic, output_qty, output_type)
VALUES
('tech', 'Tech Research (+1 level)', 5, 10, 5, 2, 1, 'TECH');

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

-- Per-game base star ownership (for repair/resupply eligibility)
CREATE TABLE IF NOT EXISTS base_stars (
    game_id INT NOT NULL,
    hex_id VARCHAR(8) NOT NULL,
    owner CHAR(1) NOT NULL,
    PRIMARY KEY (game_id, hex_id),
    FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
    INDEX idx_game_owner (game_id, owner)
);

-- Saved game ship snapshots
CREATE TABLE IF NOT EXISTS saved_ships (
    id INT AUTO_INCREMENT PRIMARY KEY,
    save_id INT NOT NULL,
    ship_code VARCHAR(4) NOT NULL,
    ship_name VARCHAR(64),
    owner CHAR(1) NOT NULL,
    ship_json TEXT,
    FOREIGN KEY (save_id) REFERENCES saved_games(id) ON DELETE CASCADE,
    INDEX idx_save (save_id)
);

-- Dynamic hex events that affect gameplay
CREATE TABLE IF NOT EXISTS hex_events (
id INT AUTO_INCREMENT PRIMARY KEY,
game_id INT NOT NULL,
hex_id VARCHAR(8) NOT NULL,
event_type VARCHAR(32) NOT NULL,  -- NAVIGATION_HAZARD, COMBAT_INTERFERENCE, etc.
modifier_value INT DEFAULT 0,
spawned_turn INT NOT NULL,
expires_turn INT NOT NULL,
FOREIGN KEY (game_id) REFERENCES games(id),
INDEX idx_hex_events (game_id, hex_id)
);


-- appending for new pending damage table work

CREATE TABLE IF NOT EXISTS pending_damage (
    game_id INT NOT NULL,
    hex_id VARCHAR(8) NOT NULL,
    round INT NOT NULL,
    ship_code VARCHAR(4) NOT NULL,
    owner CHAR(1) NOT NULL,
    damage_amount INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (game_id, hex_id, round, ship_code, owner),
    INDEX idx_pending_owner (game_id, hex_id, owner),
    INDEX idx_pending_ship (game_id, ship_code)
);

-- Index for finding all damage for a player in a hex
-- CREATE INDEX IF NOT EXISTS idx_pending_player_hex 
-- ON pending_damage(game_id, hex_id, owner);

-- Index for checking if a specific ship has pending damage
-- CREATE INDEX IF NOT EXISTS idx_pending_ship_check 
-- ON pending_damage(game_id, ship_code, owner);

-- More Milieu Codex 


-- Hex Objects: Discoverable entities at hex coordinates
-- For debris: source_ship_id references the destroyed ship (ships table has all data)
-- For other types: use explicit columns, no JSON
CREATE TABLE IF NOT EXISTS hex_objects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    game_id INT NOT NULL,
    hex_id VARCHAR(8) NOT NULL,
    object_type ENUM('debris','wreckage','alien_ship','ghost_ship','anomaly') NOT NULL,
    state ENUM('hidden','detected','identified','salvaged','destroyed') NOT NULL DEFAULT 'hidden',
    owner CHAR(1) DEFAULT NULL,
    discovered_by CHAR(1) DEFAULT NULL,
    source_ship_id INT DEFAULT NULL,
    salvage_value INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    salvaged_at TIMESTAMP NULL DEFAULT NULL,
    FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,
    FOREIGN KEY (source_ship_id) REFERENCES ships(id) ON DELETE SET NULL,
    INDEX idx_hex_objects_lookup (game_id, hex_id, state),
    INDEX idx_hex_objects_discovery (game_id, state, object_type)
);

-- AI Metric Persistence: cross-turn memory for AutonomyAgency
-- Stores deduced/computed metrics only (not raw DB copies)
CREATE TABLE IF NOT EXISTS aa_metrics (
    game_id      INT NOT NULL,
    player       CHAR(1) NOT NULL,
    metric_name  VARCHAR(64) NOT NULL,
    metric_value DOUBLE NOT NULL DEFAULT 0,
    updated_round INT NOT NULL DEFAULT 0,
    PRIMARY KEY (game_id, player, metric_name),
    FOREIGN KEY (game_id) REFERENCES games(id)
);
