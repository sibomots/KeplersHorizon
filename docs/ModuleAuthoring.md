# Module Authoring Guidelines

## Overview

A **Module** is a complete universe definition for Kepler's Horizon. Each module contains:
- Map topology (star systems, hexes, warplines)
- Milieu content (planets, species, populations, resources, etc.)

The game engine is module-agnostic. Players select a module when creating a game.

## Directory Structure

```
site/db/modules/<module_name>/
├── LoadModule.sql          # Main loader script
├── star_systems.csv        # Map topology (required)
├── hexes.csv               # Hex coordinates (required)
├── warplines.csv           # Warpline connections (required)
├── warpline_hexes.csv      # Warpline path hexes (required)
├── planets.csv             # Planetary bodies
├── moons.csv               # Moon data
├── stars.csv               # Stellar data
├── species.csv             # Species definitions
├── populations.csv         # Population centers
├── resources.csv           # Extractable resources
├── facilities.csv          # Facilities and stations
├── anomalies.csv           # System anomalies
├── salvageables.csv        # Salvage sites
├── salvageable_drops.csv   # Salvage loot tables
└── codex_rumors.csv       # Knowledge/rumors
```

## Creating a New Module

### Step 1: Register the Module

Add an entry to the `modules` table:

```sql
INSERT INTO modules (name, description) 
VALUES ('My Universe', 'Description of your universe');
-- Note the assigned module_id for your CSV files
```

### Step 2: Create Map Topology

You can reuse the existing Kepler's Horizon map topology or create your own.

#### Reusing KH Topology
Copy these files from `modules/kh/`:
- `star_systems.csv`
- `hexes.csv`
- `warplines.csv`
- `warpline_hexes.csv`

Update the first column (module_id) to your assigned module_id.

#### Creating New Topology
1. **hexes.csv**: Define hex grid coordinates (module_id, hex_id, q, r)
2. **star_systems.csv**: Name hexes as star systems with base assignments
3. **warplines.csv**: Connect star hexes with warplines
4. **warpline_hexes.csv**: Define intermediate hexes for each warpline

### Step 3: Create Milieu Content

Each milieu CSV should use SET module_id=@module in the loader to associate with your module.

See `modules/template/` for minimal examples of each file format.

## CSV Column Reference

### star_systems.csv
| Column | Type | Description |
|--------|------|-------------|
| module_id | INT | Your module ID |
| hex_id | VARCHAR(8) | Hex coordinate (e.g., "0101") |
| name | VARCHAR(64) | System name (e.g., "ARVEN") |
| is_base | BOOLEAN | 1 if this is a home base star |
| base_owner | CHAR(1) | 'A' or 'B' for base ownership |
| base_side | VARCHAR(8) | 'ALPHA' or 'BETA' for faction |
| territory_name | VARCHAR(64) | Territory name |

### planets.csv
| Column | Type | Description |
|--------|------|-------------|
| id | INT | Unique planet ID |
| system_name | VARCHAR(64) | Parent star system |
| star_id | INT | Parent star ID |
| orbital_position | INT | Orbital order (1-12) |
| designation | VARCHAR(16) | Official designation |
| common_name | VARCHAR(64) | Common name |
| planet_type | ENUM | Rocky, Gas Giant, Ice, etc. |
| atmosphere | ENUM | None, Thin, Standard, Dense, Toxic |
| hydrosphere | ENUM | None, Trace, Moderate, Abundant |
| biosphere | ENUM | None, Microbial, Flora, Fauna |
| habitability | INT | 1-10 rating |
| notes | TEXT | Additional notes |

## Testing Your Module

1. Run the schema: `mysql -u user -p khdb < 0-schema.sql`
2. Load your module: `mysql --local-infile=1 -u user -p khdb < modules/your_module/LoadModule.sql`
3. Verify: `SELECT * FROM modules; SELECT COUNT(*) FROM star_systems WHERE module_id=YOUR_ID;`
4. Create a game with your module and test gameplay

## Constraints

- Map topology must form a connected graph
- Each module must have exactly 2 base-star systems (one A, one B)
- System names must be unique within a module
- All foreign keys must reference valid IDs
