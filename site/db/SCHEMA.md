# Star System Database Schema

This document defines the SQL schema for star system metadata tables.

**Note**: These tables are independent of the existing game schema. They share `star_systems.name` as a reference point but have no formal foreign key constraints to the parent schema yet.

---

## Schema Version

- **Version**: 1.0
- **Created**: 2026-01-04
- **Status**: Metadata only (no game integration yet)

---

## Table: system_species

Defines sapient species in the galaxy.

```sql
CREATE TABLE IF NOT EXISTS system_species (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(64) NOT NULL UNIQUE,
    classification ENUM('Human', 'NearHuman', 'Xenoform', 'Machine', 'Elder') NOT NULL,
    homeworld_system VARCHAR(64) DEFAULT NULL,
    physiology TEXT,
    psychology TEXT,
    special_ability VARCHAR(128),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

---

## Table: system_stars

Individual stars within each system.

```sql
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
```

---

## Table: system_planets

Planets within each system.

```sql
CREATE TABLE IF NOT EXISTS system_planets (
    id INT AUTO_INCREMENT PRIMARY KEY,
    system_name VARCHAR(64) NOT NULL,
    star_id INT,
    orbital_position INT NOT NULL,
    designation VARCHAR(32) NOT NULL,
    common_name VARCHAR(64),
    planet_type ENUM('Rocky', 'GasGiant', 'IceGiant', 'Dwarf') NOT NULL,
    atmosphere ENUM('None', 'Thin', 'Standard', 'Dense', 'Toxic', 'Exotic'),
    hydrosphere ENUM('None', 'Trace', 'Moderate', 'Ocean', 'Ice'),
    biosphere ENUM('None', 'Microbial', 'Primitive', 'Complex', 'Sapient'),
    habitability ENUM('Hostile', 'Marginal', 'Comfortable', 'Garden'),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_system (system_name),
    INDEX idx_star (star_id)
);
```

---

## Table: system_moons

Moons of planets.

```sql
CREATE TABLE IF NOT EXISTS system_moons (
    id INT AUTO_INCREMENT PRIMARY KEY,
    planet_id INT NOT NULL,
    designation VARCHAR(32) NOT NULL,
    common_name VARCHAR(64),
    moon_type ENUM('Rocky', 'Ice', 'Captured', 'RingShepherd') NOT NULL,
    size ENUM('Asteroid', 'Small', 'Medium', 'Large') NOT NULL,
    notable_feature VARCHAR(128),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_planet (planet_id)
);
```

---

## Table: system_asteroid_belts

Asteroid belts within systems.

```sql
CREATE TABLE IF NOT EXISTS system_asteroid_belts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    system_name VARCHAR(64) NOT NULL,
    designation VARCHAR(32) NOT NULL,
    orbital_position DECIMAL(3,1) NOT NULL,
    density ENUM('Sparse', 'Moderate', 'Dense', 'Extreme') NOT NULL,
    composition ENUM('Silicate', 'Carbonaceous', 'Metallic', 'Mixed', 'Ice') NOT NULL,
    hazard_level ENUM('None', 'Minor', 'Significant', 'Severe') NOT NULL,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_system (system_name)
);
```

---

## Table: system_resources

Resource deposits on celestial bodies.

```sql
CREATE TABLE IF NOT EXISTS system_resources (
    id INT AUTO_INCREMENT PRIMARY KEY,
    location_type ENUM('Planet', 'Moon', 'Belt', 'System') NOT NULL,
    location_id INT NOT NULL,
    resource_type ENUM('FERROUS', 'RARE_EARTH', 'RADIOACTIVE', 'CRYSTALLINE', 'VOLATILE', 'WATER', 'ORGANIC', 'EXOTIC') NOT NULL,
    abundance ENUM('Trace', 'Low', 'Moderate', 'High', 'Rich') NOT NULL,
    extraction_difficulty ENUM('Easy', 'Moderate', 'Difficult', 'Extreme'),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location (location_type, location_id),
    INDEX idx_resource_type (resource_type)
);
```

---

## Table: system_populations

Population centers.

```sql
CREATE TABLE IF NOT EXISTS system_populations (
    id INT AUTO_INCREMENT PRIMARY KEY,
    location_type ENUM('Planet', 'Moon', 'Station') NOT NULL,
    location_id INT NOT NULL,
    species_id INT NOT NULL,
    pop_class ENUM('Outpost', 'Settlement', 'Colony', 'City', 'Metropolis') NOT NULL,
    population_millions DECIMAL(8,2),
    tech_level INT NOT NULL,
    government VARCHAR(64),
    disposition ENUM('Hostile', 'Wary', 'Neutral', 'Friendly', 'Allied') NOT NULL,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location (location_type, location_id),
    INDEX idx_species (species_id)
);
```

---

## Table: system_facilities

Infrastructure installations.

```sql
CREATE TABLE IF NOT EXISTS system_facilities (
    id INT AUTO_INCREMENT PRIMARY KEY,
    location_type ENUM('Planet', 'Moon', 'Belt', 'Orbit') NOT NULL,
    location_id INT NOT NULL,
    facility_type ENUM('SHIPYARD', 'REPAIR_DOCK', 'REFINERY', 'TRADE_HUB', 'FORTRESS', 'BEACON', 'MINING_STATION', 'RESEARCH_LAB', 'ORBITAL_PLATFORM') NOT NULL,
    name VARCHAR(64) NOT NULL,
    capacity INT,
    owner ENUM('A', 'B', 'Neutral', 'Contested', 'Abandoned') NOT NULL,
    operational BOOLEAN NOT NULL DEFAULT TRUE,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location (location_type, location_id),
    INDEX idx_facility_type (facility_type),
    INDEX idx_owner (owner)
);
```

---

## Table: system_anomalies

Discoveries and phenomena.

```sql
CREATE TABLE IF NOT EXISTS system_anomalies (
    id INT AUTO_INCREMENT PRIMARY KEY,
    system_name VARCHAR(64) NOT NULL,
    anomaly_type ENUM('DERELICT', 'ARTIFACT', 'SPATIAL_RIFT', 'RADIATION_ZONE', 'DARK_NEBULA', 'GRAVEYARD', 'BEACON_ANCIENT', 'PHENOMENON') NOT NULL,
    name VARCHAR(64) NOT NULL,
    effect TEXT,
    discovery_text TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_system (system_name),
    INDEX idx_anomaly_type (anomaly_type)
);
```

---

## Table: system_grimoire_rumors

Starting knowledge for each system.

```sql
CREATE TABLE IF NOT EXISTS system_grimoire_rumors (
    id INT AUTO_INCREMENT PRIMARY KEY,
    system_name VARCHAR(64) NOT NULL,
    rumor_text TEXT NOT NULL,
    knowledge_level ENUM('Unknown', 'Rumored', 'Charted', 'Surveyed', 'Intimate') NOT NULL DEFAULT 'Rumored',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_system (system_name)
);
```

---

## Runtime Table: grimoire_entries

Per-game, per-player knowledge tracking (created at game start, not seeded).

```sql
CREATE TABLE IF NOT EXISTS grimoire_entries (
    game_id INT NOT NULL,
    player ENUM('A', 'B') NOT NULL,
    system_name VARCHAR(64) NOT NULL,
    knowledge_level ENUM('Unknown', 'Rumored', 'Charted', 'Surveyed', 'Intimate') NOT NULL DEFAULT 'Unknown',
    last_updated_turn VARCHAR(8),
    notes TEXT,
    PRIMARY KEY (game_id, player, system_name),
    INDEX idx_game_player (game_id, player)
);
```

---

## Seed Data Loading

Use the following order to respect implicit dependencies:

```sql
-- 1. Species (no dependencies)
LOAD DATA LOCAL INFILE 'species.csv' INTO TABLE system_species
    FIELDS TERMINATED BY ',' ENCLOSED BY '"'
    LINES TERMINATED BY '\n'
    IGNORE 1 ROWS;

-- 2. Stars (depends on star_systems.name)
LOAD DATA LOCAL INFILE 'stars.csv' INTO TABLE system_stars ...

-- 3. Planets (depends on stars)
LOAD DATA LOCAL INFILE 'planets.csv' INTO TABLE system_planets ...

-- 4. Moons (depends on planets)
LOAD DATA LOCAL INFILE 'moons.csv' INTO TABLE system_moons ...

-- 5. Asteroid Belts (depends on star_systems.name)
LOAD DATA LOCAL INFILE 'asteroid_belts.csv' INTO TABLE system_asteroid_belts ...

-- 6. Resources (depends on planets, moons, belts)
LOAD DATA LOCAL INFILE 'resources.csv' INTO TABLE system_resources ...

-- 7. Populations (depends on planets, moons, species)
LOAD DATA LOCAL INFILE 'populations.csv' INTO TABLE system_populations ...

-- 8. Facilities (depends on planets, moons, belts)
LOAD DATA LOCAL INFILE 'facilities.csv' INTO TABLE system_facilities ...

-- 9. Anomalies (depends on star_systems.name)
LOAD DATA LOCAL INFILE 'anomalies.csv' INTO TABLE system_anomalies ...

-- 10. Grimoire Rumors (depends on star_systems.name)
LOAD DATA LOCAL INFILE 'grimoire_rumors.csv' INTO TABLE system_grimoire_rumors ...
```

---

## Query Examples

### Get all planets in a system
```sql
SELECT * FROM system_planets WHERE system_name = 'KORAL' ORDER BY orbital_position;
```

### Get resources for a planet
```sql
SELECT r.* FROM system_resources r
WHERE r.location_type = 'Planet' AND r.location_id = ?;
```

### Get grimoire-filtered view (player A knows 'Charted')
```sql
SELECT p.designation, p.common_name, p.planet_type
FROM system_planets p
WHERE p.system_name = 'KORAL';
-- Detailed resources only if knowledge_level >= 'Surveyed'
```

### Get all anomalies discovered by player
```sql
SELECT a.* FROM system_anomalies a
JOIN grimoire_entries g ON a.system_name = g.system_name
WHERE g.game_id = ? AND g.player = 'A' AND g.knowledge_level = 'Intimate';
```

---

## Future Integration Points

1. **star_systems.name**: Bridge to existing schema (shared by map_id)
2. **ships.at_system**: When ship enters system, can query metadata
3. **game state**: grimoire_entries tied to game_id
4. **combat/movement**: System constraints from anomalies/populations

---

## Notes

- All `system_*` tables are map-independent (same data for all games)
- Only `grimoire_entries` is game-specific
- No formal FKs to main schema yet (future work)
