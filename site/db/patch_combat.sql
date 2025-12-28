-- Combat State: Tracks progress of combat in a specific hex
CREATE TABLE IF NOT EXISTS combat_state (
    game_id INT NOT NULL,
    hex_id VARCHAR(8) NOT NULL,
    round INT NOT NULL DEFAULT 1,
    stage INT NOT NULL DEFAULT 0, -- 0=ORDERS, 1=RESOLVE_READY, 2=DAMAGE_PENDING, 3=RETREAT_PENDING
    attacker_remains TINYINT NOT NULL DEFAULT 0,
    stalemate_counter INT NOT NULL DEFAULT 0,
    pending_damage_json TEXT NULL, -- JSON object {"ShipCode": hits}
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (game_id, hex_id)
);

-- Combat Orders: Stores secret orders for each ship per round
CREATE TABLE IF NOT EXISTS combat_orders (
    game_id INT NOT NULL,
    ship_code VARCHAR(8) NOT NULL,
    round INT NOT NULL,
    tactic CHAR(1) NOT NULL, -- 'A', 'D', 'R'
    target_id VARCHAR(8) NULL, -- Beam target
    power_d INT NOT NULL,
    power_b INT NOT NULL,
    power_s INT NOT NULL,
    power_t INT NOT NULL,
    missiles_json TEXT NULL, -- JSON array of missile targets
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (game_id, ship_code, round),
    INDEX (game_id, ship_code)
);
