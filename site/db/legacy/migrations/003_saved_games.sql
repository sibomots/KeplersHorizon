-- Saved Games Schema
-- Allows players to save and load game states

CREATE TABLE IF NOT EXISTS saved_games (
    id INT AUTO_INCREMENT PRIMARY KEY,
    save_name VARCHAR(128) NOT NULL,
    user_id INT NOT NULL,
    original_game_id INT NOT NULL,
    room_code VARCHAR(6),
    scenario VARCHAR(32) DEFAULT 'basic',
    
    -- Snapshot of game state JSON
    state_json TEXT NOT NULL,
    
    -- Metadata
    round INT DEFAULT 1,
    player_a_name VARCHAR(64),
    player_b_name VARCHAR(64),
    
    saved_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_saves (user_id, saved_at DESC)
);

-- Also store ship snapshots
CREATE TABLE IF NOT EXISTS saved_ships (
    id INT AUTO_INCREMENT PRIMARY KEY,
    save_id INT NOT NULL,
    ship_code VARCHAR(10) NOT NULL,
    ship_name VARCHAR(64),
    owner CHAR(1) NOT NULL,
    ship_json TEXT NOT NULL,
    
    FOREIGN KEY (save_id) REFERENCES saved_games(id) ON DELETE CASCADE
);
