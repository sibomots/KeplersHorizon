-- Milieu Seed: System Constraints
-- Load using: mysql -u khdb -p khdb < milieu_constraints_seed.sql
-- Or use LOAD DATA INFILE for CSV import

USE khdb;

-- System Constraints (from anomalies)
-- These are global constraints, not per-game
LOAD DATA LOCAL INFILE 'milieu/system_constraints.csv'
INTO TABLE system_constraints
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"'
LINES TERMINATED BY '\n'
IGNORE 1 ROWS
(system_name, constraint_type, modifier_type, modifier_value, condition_text, source);

-- Market Base Prices (reference table)
-- Used to initialize market_prices per-game
CREATE TABLE IF NOT EXISTS market_base_prices (
    resource_type ENUM('FERROUS', 'RARE_EARTH', 'RADIOACTIVE', 'CRYSTALLINE', 'VOLATILE', 'WATER', 'ORGANIC', 'EXOTIC') PRIMARY KEY,
    base_price INT NOT NULL
);

LOAD DATA LOCAL INFILE 'milieu/market_base_prices.csv'
INTO TABLE market_base_prices
FIELDS TERMINATED BY ','
ENCLOSED BY '"'
LINES TERMINATED BY '\n'
IGNORE 1 ROWS
(resource_type, base_price);

-- Facility Control Initial State (reference table)
-- Used to initialize facility_control per-game
CREATE TABLE IF NOT EXISTS facility_control_initial (
    system_name VARCHAR(64) NOT NULL,
    facility_type VARCHAR(32) NOT NULL,
    controller CHAR(1),
    PRIMARY KEY (system_name, facility_type)
);

LOAD DATA LOCAL INFILE 'milieu/facility_control_initial.csv'
INTO TABLE facility_control_initial
FIELDS TERMINATED BY ','
ENCLOSED BY '"'
LINES TERMINATED BY '\n'
IGNORE 1 ROWS
(system_name, facility_type, controller);

-- Note: Game initialization should copy from these reference tables:
--
-- INSERT INTO facility_control (game_id, system_name, facility_type, controller)
-- SELECT ?, system_name, facility_type, controller FROM facility_control_initial;
--
-- INSERT INTO market_prices (game_id, resource_type, current_price, base_price, price_trend, last_updated_turn)
-- SELECT ?, resource_type, base_price, base_price, 'STABLE', 1 FROM market_base_prices;
