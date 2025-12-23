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

-- drop database khdb;

CREATE DATABASE IF NOT EXISTS khdb;
USE khdb;

CREATE TABLE IF NOT EXISTS users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  password_plain VARCHAR(128) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Demo users (change/remove later)
INSERT IGNORE INTO users(username,password_plain) VALUES
('alice','alicepw'),
('bob','bobpw');

CREATE TABLE IF NOT EXISTS sessions (
  token CHAR(64) PRIMARY KEY,
  user_id INT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS games (
  id INT AUTO_INCREMENT PRIMARY KEY,
  scenario VARCHAR(16) DEFAULT NULL,
  state_json MEDIUMTEXT NOT NULL,
  current_draft_A VARCHAR(4) DEFAULT NULL,
  current_draft_B VARCHAR(4) DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS game_events (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  game_id INT NOT NULL,
  user_id INT NOT NULL,
  seq INT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  command_text VARCHAR(256) NOT NULL,
  result_text VARCHAR(256) NOT NULL,
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
  at_system VARCHAR(16) DEFAULT NULL,
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

ALTER TABLE sessions ADD COLUMN last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP;


-- map metadata per game
CREATE TABLE IF NOT EXISTS star_systems (
  game_id INT NOT NULL,
  hex_id VARCHAR(8) NOT NULL,
  name VARCHAR(64) NOT NULL,
  is_base TINYINT NOT NULL DEFAULT 0,
  base_owner CHAR(1) NULL,
  PRIMARY KEY (game_id, name),
  INDEX (game_id, hex_id),
  INDEX (game_id, is_base),
  INDEX (game_id, base_owner)
);

CREATE TABLE IF NOT EXISTS warplines (
  game_id INT NOT NULL,
  id INT NOT NULL AUTO_INCREMENT,
  a_hex VARCHAR(8) NOT NULL,
  b_hex VARCHAR(8) NOT NULL,
  PRIMARY KEY (id),
  INDEX (game_id),
  INDEX (game_id, a_hex),
  INDEX (game_id, b_hex)
);

-- ships.at_system should match star_systems.name when on a system.
