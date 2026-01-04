# Star System Metadata Structure

This document defines the structure and format for star system metadata in Kepler's Horizon.

---

## Overview

Each star system is a rich container of celestial objects, populations, resources, and infrastructure. This metadata provides the foundation for the game's economic, exploratory, and narrative layers.

**Key Principle**: Visiting a system (entering the hex) allows free exploration of all subordinate elements. There is no movement cost within a system.

---

## Entity Hierarchy

```
STAR_SYSTEM (hex-level)
├── STAR(s)           [1 or more, binary/trinary possible]
├── PLANET(s)         [0-12, orbital position from star]
│   └── MOON(s)       [0-many per planet]
├── ASTEROID_BELT(s)  [0-3, between orbital positions]
├── POPULATION(s)     [on planets, moons, or stations]
├── FACILITY(s)       [on planets, moons, belts, or orbit]
├── RESOURCE(s)       [on planets, moons, or belts]
└── ANOMALY(s)        [system-wide discoveries]
```

---

## Data Files

All metadata is stored in CSV format for database ingestion.

| File | Description |
|------|-------------|
| `species.csv` | Sapient species definitions |
| `stars.csv` | Individual stars per system |
| `planets.csv` | Planets per system |
| `moons.csv` | Moons per planet |
| `asteroid_belts.csv` | Asteroid belts per system |
| `resources.csv` | Resource deposits |
| `populations.csv` | Population centers |
| `facilities.csv` | Infrastructure |
| `anomalies.csv` | Discoveries and phenomena |
| `grimoire_rumors.csv` | Starting knowledge hints |

---

## Field Definitions

### STAR
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| system_name | VARCHAR(64) | Yes | Parent system (FK to star_systems.name) |
| designation | VARCHAR(32) | Yes | Star name, e.g., "Sonal Prime" |
| star_class | CHAR(1) | Yes | Spectral class: O, B, A, F, G, K, M, L, T, Y |
| luminosity | VARCHAR(16) | Yes | Supergiant, Giant, Subgiant, Main, Dwarf |
| color | VARCHAR(16) | No | Visual description |
| age_gy | DECIMAL(4,1) | No | Age in billions of years |
| notes | TEXT | No | Flavor text |

### PLANET
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| system_name | VARCHAR(64) | Yes | Parent system |
| star_id | INT | No | Orbiting star (for binaries) |
| orbital_position | INT | Yes | 1-12, distance from star |
| designation | VARCHAR(32) | Yes | Formal name, e.g., "Sonal III" |
| common_name | VARCHAR(64) | No | Local name, e.g., "Verdance" |
| planet_type | VARCHAR(16) | Yes | Rocky, GasGiant, IceGiant, Dwarf |
| atmosphere | VARCHAR(16) | No | None, Thin, Standard, Dense, Toxic, Exotic |
| hydrosphere | VARCHAR(16) | No | None, Trace, Moderate, Ocean, Ice |
| biosphere | VARCHAR(16) | No | None, Microbial, Primitive, Complex, Sapient |
| habitability | VARCHAR(16) | No | Hostile, Marginal, Comfortable, Garden |
| notes | TEXT | No | Flavor text |

### MOON
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| planet_id | INT | Yes | Parent planet |
| designation | VARCHAR(32) | Yes | Formal name, e.g., "Sonal II-a" |
| common_name | VARCHAR(64) | No | Local name |
| moon_type | VARCHAR(16) | Yes | Rocky, Ice, Captured, RingShepherd |
| size | VARCHAR(16) | Yes | Asteroid, Small, Medium, Large |
| notable_feature | VARCHAR(128) | No | What makes it interesting |
| notes | TEXT | No | Flavor text |

### ASTEROID_BELT
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| system_name | VARCHAR(64) | Yes | Parent system |
| designation | VARCHAR(32) | Yes | Belt name, e.g., "The Shatter" |
| orbital_position | DECIMAL(3,1) | Yes | Between planet N and N+1 |
| density | VARCHAR(16) | Yes | Sparse, Moderate, Dense, Extreme |
| composition | VARCHAR(16) | Yes | Silicate, Carbonaceous, Metallic, Mixed, Ice |
| hazard_level | VARCHAR(16) | Yes | None, Minor, Significant, Severe |
| notes | TEXT | No | Flavor text |

### RESOURCE
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| location_type | VARCHAR(16) | Yes | Planet, Moon, Belt, System |
| location_id | INT | Yes | FK to appropriate table |
| resource_type | VARCHAR(16) | Yes | FERROUS, RARE_EARTH, RADIOACTIVE, CRYSTALLINE, VOLATILE, WATER, ORGANIC, EXOTIC |
| abundance | VARCHAR(16) | Yes | Trace, Low, Moderate, High, Rich |
| extraction_difficulty | VARCHAR(16) | No | Easy, Moderate, Difficult, Extreme |
| notes | TEXT | No | Specific details |

### POPULATION
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| location_type | VARCHAR(16) | Yes | Planet, Moon, Station |
| location_id | INT | Yes | FK to appropriate table |
| species_id | INT | Yes | FK to species |
| pop_class | VARCHAR(16) | Yes | Outpost, Settlement, Colony, City, Metropolis |
| population_millions | DECIMAL(8,2) | No | Population in millions |
| tech_level | INT | Yes | 1-10 scale |
| government | VARCHAR(64) | No | Political structure |
| disposition | VARCHAR(16) | Yes | Hostile, Wary, Neutral, Friendly, Allied |
| notes | TEXT | No | Cultural details |

### SPECIES
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| name | VARCHAR(64) | Yes | Species name |
| classification | VARCHAR(16) | Yes | Human, NearHuman, Xenoform, Machine, Elder |
| homeworld_system | VARCHAR(64) | No | Origin system (may be NULL) |
| physiology | TEXT | No | Physical description |
| psychology | TEXT | No | Behavioral traits |
| special_ability | VARCHAR(128) | No | Unique trait |

### FACILITY
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| location_type | VARCHAR(16) | Yes | Planet, Moon, Belt, Orbit |
| location_id | INT | Yes | FK to appropriate table |
| facility_type | VARCHAR(16) | Yes | SHIPYARD, REPAIR_DOCK, REFINERY, TRADE_HUB, FORTRESS, BEACON, MINING_STATION, RESEARCH_LAB, ORBITAL_PLATFORM |
| name | VARCHAR(64) | Yes | Local name |
| capacity | INT | No | Production/service capacity |
| owner | VARCHAR(16) | Yes | A, B, Neutral, Contested, Abandoned |
| operational | BOOLEAN | Yes | Is it functioning? |
| notes | TEXT | No | Details |

### ANOMALY
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | INT | Yes | Unique identifier |
| system_name | VARCHAR(64) | Yes | Parent system |
| anomaly_type | VARCHAR(16) | Yes | DERELICT, ARTIFACT, SPATIAL_RIFT, RADIATION_ZONE, DARK_NEBULA, GRAVEYARD, BEACON_ANCIENT, PHENOMENON |
| name | VARCHAR(64) | Yes | Local designation |
| effect | TEXT | No | Gameplay effect |
| discovery_text | TEXT | No | What players see |

### GRIMOIRE_RUMOR
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| system_name | VARCHAR(64) | Yes | Target system |
| rumor_text | TEXT | Yes | What players know at start |
| knowledge_level | VARCHAR(16) | Yes | Starting level for this rumor |

---

## CSV Format Rules

1. **Header Row**: First row contains column names
2. **Delimiter**: Comma (`,`)
3. **Quoting**: Text with commas must be quoted (`"text, with comma"`)
4. **Encoding**: UTF-8
5. **Newlines in Text**: Use `\n` escape sequence
6. **NULL Values**: Empty field (consecutive commas)

---

## Ingestion Order

Due to foreign key dependencies, load CSV files in this order:

1. `species.csv`
2. `stars.csv`
3. `planets.csv`
4. `moons.csv`
5. `asteroid_belts.csv`
6. `resources.csv`
7. `populations.csv`
8. `facilities.csv`
9. `anomalies.csv`
10. `grimoire_rumors.csv`

---

## Naming Conventions

### Stars
- Primary stars: `[System] Prime`, `[System] Sol`
- Binary components: `[System] Alpha`, `[System] Beta`
- Unusual stars: Descriptive names like `Elam Vermillion`, `Ugarit Dusk`

### Planets
- Formal: `[System] [Roman Numeral]` (e.g., "Sonal III")
- Common: Evocative names (e.g., "Verdance", "The Anvil", "Hanging Gardens")

### Moons
- Formal: `[Planet]-[letter]` (e.g., "Sonal II-a")
- Common: Evocative names (e.g., "Hope", "Citadel", "Shepherd")

### Asteroid Belts
- Descriptive/evocative: "The Shatter", "Old Guard", "The Quarry"

---

## Knowledge Levels (Grimoire)

| Level | Visible Data |
|-------|--------------|
| Unknown | System name only |
| Rumored | Star type, planet count, rumors |
| Charted | Planets, belts, basic resources |
| Surveyed | Detailed resources, populations |
| Intimate | Anomalies, hidden information |

---

## Version

**Structure Version**: 1.0
**Last Updated**: 2026-01-04
