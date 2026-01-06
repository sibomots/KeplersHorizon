-- OK

drop database khdb;

CREATE DATABASE IF NOT EXISTS khdb;
USE khdb;

DROP TABLE IF EXISTS telemetry_queue;
DROP TABLE IF EXISTS combat_orders;
DROP TABLE IF EXISTS combat_state;
DROP TABLE IF EXISTS warpline_hexes;
DROP TABLE IF EXISTS hexes;
DROP TABLE IF EXISTS warplines;
DROP TABLE IF EXISTS star_systems;
DROP TABLE IF EXISTS sightings;
DROP TABLE IF EXISTS ships;
DROP TABLE IF EXISTS drafts;
DROP TABLE IF EXISTS game_events;
DROP TABLE IF EXISTS game_seats;
DROP TABLE IF EXISTS games;
DROP TABLE IF EXISTS sessions;
DROP TABLE IF EXISTS rooms;
DROP TABLE IF EXISTS users;


CREATE TABLE IF NOT EXISTS users (
id INT AUTO_INCREMENT PRIMARY KEY,
username VARCHAR(64) NOT NULL UNIQUE,
email VARCHAR(255) UNIQUE,
password_plain VARCHAR(128),
password_hash VARCHAR(255),
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
scenario VARCHAR(16) DEFAULT NULL,
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

CREATE TABLE IF NOT EXISTS games (
id INT AUTO_INCREMENT PRIMARY KEY,
room_id INT DEFAULT NULL,
scenario VARCHAR(16) DEFAULT NULL,
state_json MEDIUMTEXT NOT NULL,
current_draft_A VARCHAR(4) DEFAULT NULL,
current_draft_B VARCHAR(4) DEFAULT NULL,
active_combat_hex VARCHAR(8) DEFAULT NULL,
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
FOREIGN KEY (room_id) REFERENCES rooms(id)
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
id BIGINT AUTO_INCREMENT PRIMARY KEY,
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
dr INT NOT NULL DEFAULT 0,    -- Drones
pd_spent INT NOT NULL DEFAULT 0,
at_system VARCHAR(16) DEFAULT NULL,
at_hex VARCHAR(8) DEFAULT NULL,
racked_in VARCHAR(4) DEFAULT NULL,
destroyed_at TIMESTAMP NULL DEFAULT NULL,
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

CREATE TABLE IF NOT EXISTS star_systems (
map_id INT NOT NULL DEFAULT 1,
hex_id VARCHAR(8) NOT NULL,
name VARCHAR(64) NOT NULL,
is_base TINYINT NOT NULL DEFAULT 0,
base_owner CHAR(1) NULL,
PRIMARY KEY (map_id, name),
INDEX (map_id, hex_id),
INDEX (map_id, is_base),
INDEX (map_id, base_owner)
);

CREATE TABLE IF NOT EXISTS warplines (
map_id INT NOT NULL DEFAULT 1,
id INT NOT NULL AUTO_INCREMENT,
a_hex VARCHAR(8) NOT NULL,
b_hex VARCHAR(8) NOT NULL,
PRIMARY KEY (id),
INDEX (map_id),
INDEX (map_id, a_hex),
INDEX (map_id, b_hex)
);

CREATE TABLE IF NOT EXISTS hexes (
map_id INT NOT NULL DEFAULT 1,
hex_id VARCHAR(8) NOT NULL,
q INT NOT NULL,
r INT NOT NULL,
PRIMARY KEY (map_id, hex_id),
INDEX (map_id, q),
INDEX (map_id, r)
);

CREATE TABLE IF NOT EXISTS warpline_hexes (
map_id INT NOT NULL DEFAULT 1,
warpline_id INT NOT NULL,
hex_id VARCHAR(8) NOT NULL,
PRIMARY KEY (map_id, warpline_id, hex_id),
INDEX (map_id, hex_id),
INDEX (map_id, warpline_id)
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

CREATE TABLE IF NOT EXISTS harvest_operations (
game_id INT NOT NULL,
ship_code VARCHAR(4) NOT NULL,
owner CHAR(1) NOT NULL,
location_type VARCHAR(16) NOT NULL,
location_id INT NOT NULL,
resource_type VARCHAR(16) NOT NULL,
started_turn INT NOT NULL,
completed BOOLEAN DEFAULT FALSE,
yield INT DEFAULT 0,
PRIMARY KEY (game_id, owner, ship_code, started_turn),
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
system_name VARCHAR(64) NOT NULL,
constraint_type ENUM('MOVEMENT', 'COMBAT', 'TRADE', 'HARVEST', 'BUILD') NOT NULL,
modifier_type ENUM('BONUS', 'PENALTY', 'BLOCK') NOT NULL,
modifier_value INT DEFAULT 0,
condition_text TEXT,
source VARCHAR(64),
INDEX (system_name)
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

-- Salvage records from Graveyard-type anomalies
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
