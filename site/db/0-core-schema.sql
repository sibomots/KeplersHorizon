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

