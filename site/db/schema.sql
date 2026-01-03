--
-- BSD 3-Clause License
-- 
-- This file is part of Kepler's Horizon
--
-- Copyright (c) 2025, sibomots
-- 
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions are met:
-- 
-- 1. Redistributions of source code must retain the above copyright notice, this
--    list of conditions and the following disclaimer.
-- 
-- 2. Redistributions in binary form must reproduce the above copyright notice,
--    this list of conditions and the following disclaimer in the documentation
--    and/or other materials provided with the distribution.
-- 
-- 3. Neither the name of the copyright holder nor the names of its
--    contributors may be used to endorse or promote products derived from
--    this software without specific prior written permission.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-- DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
-- DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
-- SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
-- CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
-- OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-- 
-- schema
-- MySQL / MariaDB

drop database khdb;

CREATE DATABASE IF NOT EXISTS khdb;
USE khdb;

CREATE TABLE IF NOT EXISTS users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  email VARCHAR(255) UNIQUE,
  password_plain VARCHAR(128),          -- Legacy, to be removed after migration
  password_hash VARCHAR(255),           -- bcrypt hash
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Demo users (change/remove later)
INSERT IGNORE INTO users(username,password_plain) VALUES
('alice','alicepw'),
('bob','bobpw');

-- Rooms for multiplayer lobby
CREATE TABLE IF NOT EXISTS rooms (
  id INT AUTO_INCREMENT PRIMARY KEY,
  room_code VARCHAR(8) NOT NULL UNIQUE,
  name VARCHAR(64),
  created_by INT NOT NULL,
  seat_a INT DEFAULT NULL,
  seat_b INT DEFAULT NULL,
  game_id INT DEFAULT NULL,
  status ENUM('waiting', 'ready', 'playing', 'finished') DEFAULT 'waiting',
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

-- Maps users to game seats (A or B)
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

-- Draft ships (Build-phase candidates)
CREATE TABLE IF NOT EXISTS drafts (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  game_id INT NOT NULL,
  owner CHAR(1) NOT NULL,          -- 'A' or 'B'
  ship_code VARCHAR(4) NOT NULL,   -- 'W1','S20'
  ship_name VARCHAR(32) NOT NULL,
  ship_type CHAR(1) NOT NULL,      -- 'W' or 'S'
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

-- Committed ships (Fleet)
CREATE TABLE IF NOT EXISTS ships (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  game_id INT NOT NULL,
  owner CHAR(1) NOT NULL,          -- 'A' or 'B'
  ship_code VARCHAR(4) NOT NULL,   -- 'W1','S20'
  ship_name VARCHAR(32) NOT NULL,
  ship_type CHAR(1) NOT NULL,      -- 'W' or 'S'
  tech_level INT NOT NULL DEFAULT 0,
  built_turn VARCHAR(8) NOT NULL,  -- e.g. 'R1A'
  pd INT NOT NULL DEFAULT 0,
  beam INT NOT NULL DEFAULT 0,
  screen INT NOT NULL DEFAULT 0,
  tube INT NOT NULL DEFAULT 0,
  missiles INT NOT NULL DEFAULT 0,
  sr INT NOT NULL DEFAULT 0,
  pd_spent INT NOT NULL DEFAULT 0,
  at_system VARCHAR(16) DEFAULT NULL,
  at_hex VARCHAR(8) DEFAULT NULL,
  racked_in VARCHAR(4) DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uniq_ship (game_id, owner, ship_code),
  FOREIGN KEY (game_id) REFERENCES games(id)
);

-- Future: sightings (last-seen scan results)
CREATE TABLE IF NOT EXISTS sightings (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  game_id INT NOT NULL,
  observer_owner CHAR(1) NOT NULL,  -- who observed (A/B)
  subject_owner CHAR(1) NOT NULL,   -- who was observed (A/B)
  ship_code VARCHAR(4) NOT NULL,
  ship_name VARCHAR(32) NOT NULL,
  ship_type CHAR(1) NOT NULL,
  at_system VARCHAR(16) NOT NULL,
  last_seen_turn VARCHAR(8) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uniq_sighting (game_id, observer_owner, ship_code),
  FOREIGN KEY (game_id) REFERENCES games(id)
);

-- ALTER TABLE sessions ADD COLUMN last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP;


-- Map metadata (shared across games, identified by map_id)
-- map_id=1 is the default Kepler map. Future maps can use map_id=2, 3, etc.
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


-- ships.at_system should match star_systems.name when on a system.

-- NOTE: Seed data (star systems + warplines) is intentionally not embedded in
-- this schema file. See seed.sql + CSV files in this directory.

-- Combat Phase
CREATE TABLE IF NOT EXISTS combat_state (
  game_id INT NOT NULL,
  hex_id VARCHAR(8) NOT NULL, -- e.g. "h1616"
  round INT NOT NULL DEFAULT 1,
  stage INT NOT NULL DEFAULT 0, -- 0=ORDERS, 1=RESOLVE_READY, 2=DAMAGE_PENDING, 3=RETREAT_PENDING
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
  tactic CHAR(1) NOT NULL DEFAULT 'A', -- A, D, R
  target_id VARCHAR(4) DEFAULT NULL,
  power_d INT NOT NULL DEFAULT 0,
  power_b INT NOT NULL DEFAULT 0,
  power_s INT NOT NULL DEFAULT 0,
  power_t INT NOT NULL DEFAULT 0,
  missiles_data TEXT DEFAULT NULL, -- Comma-separated drive values, e.g. "4,5,3"
  committed BOOLEAN NOT NULL DEFAULT 0,
  PRIMARY KEY (game_id, owner, ship_code, round),
  FOREIGN KEY (game_id) REFERENCES games(id),
  FOREIGN KEY (game_id, owner, ship_code) REFERENCES ships(game_id, owner, ship_code)
);

-- Telemetry queue for tell/broadcast messages
CREATE TABLE IF NOT EXISTS telemetry_queue (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  game_id INT NOT NULL,
  target_player ENUM('A', 'B', 'BOTH') NOT NULL,
  message TEXT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  sent_at TIMESTAMP NULL DEFAULT NULL,
  FOREIGN KEY (game_id) REFERENCES games(id),
  INDEX (game_id, target_player, sent_at)
);


