# Borealis / Kepler – Software Design Document (v45 lineage)

This document describes the software architecture and data model for the Borealis (Kepler) implementation as it exists around the v45 lineage: a **web UI** (static HTML/JS) talking to a **C++11 REST server** backed by a **MySQL** database.

The project goal is a faithful implementation of Borealis rules (deterministic, no dice). The system is engineered so that:

- **The database is the source of truth** for the map, topology, and persistent game state.
- **The server is the authority** that validates commands and enforces rules.
- **The UI is a thin client**: it renders status/logs and relays user input; it does not decide legality.

This design doc is written so a developer can understand the moving parts, how they interact, and where to extend the system (movement → combat → replay).

---

## Table of Contents

1. [Database Schema](#1-database-schema)
   1. [Schema Principles](#11-schema-principles)
   2. [Authentication & Sessions](#12-authentication--sessions)
   3. [Games, Seats, and Turn State](#13-games-seats-and-turn-state)
   4. [Ship Inventory and Draft Builds](#14-ship-inventory-and-draft-builds)
   5. [Map and Topology](#15-map-and-topology)
   6. [Event Log / Audit Trail](#16-event-log--audit-trail)
2. [CSV Seed Data and Intent](#2-csv-seed-data-and-intent)
3. [Server Design](#3-server-design)
   1. [Server Responsibilities](#31-server-responsibilities)
   2. [Module Layout and Roles](#32-module-layout-and-roles)
   3. [Request Handling Flow](#33-request-handling-flow)
   4. [Command Dispatch and Validation](#34-command-dispatch-and-validation)
   5. [Turn Phases and Scenario Rules](#35-turn-phases-and-scenario-rules)
   6. [Movement Implementation (Mode 1)](#36-movement-implementation-mode-1)
   7. [JSON Responses and UI Coupling](#37-json-responses-and-ui-coupling)
4. [UI Design](#4-ui-design)
   1. [UI Principles](#41-ui-principles)
   2. [HTML Structure](#42-html-structure)
   3. [JavaScript Module Responsibilities](#43-javascript-module-responsibilities)
   4. [UI Execution Flow](#44-ui-execution-flow)
   5. [Map View Behavior](#45-map-view-behavior)
5. [Rule Coverage and Roadmap](#5-rule-coverage-and-roadmap)

---

## 1. Database Schema

### 1.1 Schema Principles

The schema is designed around these invariants:

- **Map topology is not hardcoded in C++ or JS**. It is seeded in MySQL and read at runtime.
- **All mutable game state lives in the DB**. The server maintains short-lived in-memory objects per request, but commits authoritative changes to MySQL.
- **Multiple games are supported** using `game_id` as the primary partition key for map data and state.
- **Turn authority is enforced** by comparing the authenticated seat to `games.active_player` and validating phase.

Most tables include `game_id` and appropriate indexes to keep queries small and deterministic.

---

### 1.2 Authentication & Sessions

#### `users`
Stores identities for login.

**Typical columns**
- `username` (PK)
- `password_hash` (or password in early iterations; hardened later)
- `created_at`

**Purpose**
- Identify a human user.
- Does *not* imply seat assignment. Seat assignment is per game.

#### `sessions`
Stores bearer tokens.

**Typical columns**
- `username`
- `token` (PK or indexed, sufficiently long)
- `created_at`
- `expires_at` (optional)
- `last_seen` (optional)

**Purpose**
- Enable stateless authentication: UI includes `Authorization: Bearer <token>`.
- Allow multiple browsers/windows per user if desired.

**Important behavior**
- Server endpoints reject missing/invalid bearer tokens.
- UI stores token in memory (and/or localStorage depending on build).

---

### 1.3 Games, Seats, and Turn State

#### `games`
The global state machine for a single game.

**Representative columns**
- `id` (PK)
- `scenario` (ENUM-like: `learning`, `basic`, `advanced`; empty string means “no game in progress”)
- `round` (turn counter)
- `active_player` (`'A'` or `'B'`)
- `phase_index` (integer, maps to enum in code)
- `created_at`
- `ended_at` (nullable)

**Purpose**
- Defines “where the game is”.
- Enables server to compute legality of actions:
  - Is there an active game?
  - Which scenario?
  - Whose turn?
  - Which phase?

**Design note**
- The design supports a “no game” state by setting `scenario=''` (or `NULL`), and resetting other fields.

#### `game_seats` (or similar)
Maps users to seats per game.

**Representative columns**
- `game_id`
- `seat` (`'A'` or `'B'`)
- `username`

**Purpose**
- Allows two different logged-in users to participate in the same game.
- The server uses the bearer token to find the user, then finds their seat for the game.

#### `player_state`
Stores per-seat points and possibly per-seat prompts.

**Representative columns**
- `game_id`
- `seat` (`'A'` or `'B'`)
- `bp` (Build Points currently available)
- `vp` (Victory Points accumulated)

**Purpose**
- BP/VP are scenario-governed values.
- BP is private in some designs; current implementation may return both, but the design intent is: **do not reveal opponent BP**.

---

### 1.4 Ship Inventory and Draft Builds

#### `ships`
Authoritative inventory of all committed ships.

**Representative columns**
- `game_id`
- `owner` (`'A'` or `'B'`)
- `code` (e.g. `W1`, `S20`)
- `name` (user-defined, max length ~32–64)
- `type` (`'W'` warpship, `'S'` systemship)
- Attributes (integers):
  - `pd`, `b`, `s`, `t`, `m`, `sr`
- `tech_level`
- `built_turn`
- Position:
  - `at_system` (nullable, star system name if currently at a star hex)
  - `at_hex` (nullable, hex id if in free space or on star; hex is canonical for movement)
  - `racked_in` (nullable, code of warpship carrying it)

**Purpose**
- Stores everything needed for build, deploy, move, combat, and repair/resupply.
- Encodes “not on the board” via `racked_in` and `at_hex NULL`.

**Rule coupling**
- **SystemShips** cannot move on their own.
- **Warpships** can carry SystemShips up to `sr` capacity.
- If a warpship is destroyed, racked ships are destroyed (combat rule; to be implemented).

#### `ship_drafts`
Temporary state while user is composing a build.

**Representative columns**
- `game_id`, `owner`
- `code`, `name`, `type`
- draft attributes (`pd`, `b`, `s`, `t`, `m`, `sr`)
- `created_at`

**Purpose**
- Supports UX commands like:
  - `build new W Enterprise`
  - `build set PD 6`
  - `build validate`
  - `build commit`
  - `build cancel`
- Lets server validate and compute cost before committing.

**Design note**
- Drafts are per-owner (seat) per-game. Only the active player should be allowed to create/modify drafts in Build phase.

---

### 1.5 Map and Topology

These are the key “space map” tables. The design assumes the DB is the source of truth and the UI map is static (for now).

#### `star_systems`
Star and base-star definitions.

**Given schema (as provided)**
- `game_id INT NOT NULL`
- `hex_id VARCHAR(8) NOT NULL`
- `name VARCHAR(64) NOT NULL`
- `is_base TINYINT NOT NULL DEFAULT 0`
- `base_owner CHAR(1) NULL`
- primary key: `(game_id, name)`
- indexes: `(game_id, hex_id)`, `(game_id, is_base)`, `(game_id, base_owner)`

**Purpose**
- Resolves system names (case-insensitive input) to canonical uppercase names and hex locations.
- Determines which systems are **base stars** and their owner.
- Allows deploy rules:
  - New ships deploy only to controlled base systems (scenario-dependent).

**Design note**
- Output should always render canonical `name` from this table (e.g., `SONAL`), regardless of user input casing.

#### `warplines`
Edges between star systems.

**Representative columns**
- `game_id`
- `id` (PK per game)
- `endpoint_a` (system name or hex id)
- `endpoint_b`

**Purpose**
- Declares which star hexes are connected by warp travel.
- WarpLines are bi-directional.

#### `hexes`
All hexes in the map surface.

**Representative columns**
- `game_id`
- `hex_id` (e.g. `h0606`)
- `q`, `r` (axial coords) or `row`, `col` (depends on implementation)

**Purpose**
- Defines which hex IDs exist. Prevents movement to invalid hexes.
- Provides neighbor discovery for movement algorithms.

**Design note**
- For the current map (the original 7/8 alternating rows), `hex_id` style matches the existing convention, but the design allows any future map with any coordinate system.

#### `warpline_trajectory` / `warpline_hexes`
Precomputed hex occupancy for each warpline.

**Purpose**
- Answers: “Is hex H on/under warpline L?”
- Allows movement rule: from a hex on a warpline, spend +1 PD to warp to an endpoint.

**Typical denormalized approach**
- Each row includes:
  - `game_id`
  - `warpline_id`
  - `endpoint_a_hex`
  - `endpoint_b_hex`
  - `hex_id` (occupied hex)
- Multiple rows per warpline (one per occupied hex).

This table avoids runtime geometry.

---

### 1.6 Event Log / Audit Trail

#### `events`
Append-only log of consequential actions.

**Representative columns**
- `game_id`
- `turn` (round)
- `seat` (who did it)
- `event_text` (human-readable)
- `created_at`

**Purpose**
- Supports later “summarize game X” capability.
- Provides a debugging trail for state transitions.
- Helps resolve disputes: the log is the record.

**Design note**
- Not every server request must be logged; only state-changing and important phase transitions should be logged.

---

## 2. CSV Seed Data and Intent

CSV files exist to make **map topology and initial conditions reproducible**.

### 2.1 What CSV is used for

- `star_systems.csv` seeds `star_systems`
  - defines the set of stars, base stars, ownership, and their hexes.
- `warplines.csv` seeds `warplines`
  - defines which stars are connected.
- `hexes.csv` seeds `hexes`
  - enumerates all valid hexes.
- `warpline_trajectory.csv` seeds `warpline_trajectory`
  - enumerates the occupied hexes of each warpline.

### 2.2 Why CSV exists

- Enables “nuke DB and recreate” without any build-time DB access.
- Keeps CMake clean (only builds the server).
- Supports future map variants without code changes:
  - replace CSV → reload DB → server uses new map.

### 2.3 Relationship to UI static files

The UI map is currently **not generated from DB at runtime**. It remains a static SVG/HTML artifact.

This creates an intentional constraint:

- DB is authoritative for movement/deploy validation.
- UI map is a visualization aid (may drift unless regenerated).
- Long-term goal: generate SVG from the same CSV seed data (not implemented yet).

---

## 3. Server Design

### 3.1 Server Responsibilities

The C++ server is responsible for:

- Authenticating requests (bearer token)
- Determining the user’s game seat (A/B)
- Validating command legality:
  - active game exists
  - correct phase
  - correct player turn
- Applying Borealis rules deterministically
- Mutating DB state atomically
- Emitting JSON responses:
  - `ok`
  - `event` (log lines for console)
  - `state` (for status panel)

The server is **not** responsible for:
- rendering HTML/CSS
- serving static files (Apache does that)
- doing client-side rule enforcement

---

### 3.2 Module Layout and Roles

Typical server file roles (names may differ slightly by refactor, but concepts hold):

- `main.cpp`
  - sets up HTTP listener, routes endpoints.
- `comms.cpp`
  - HTTP parsing/response formatting (minimal REST).
- `db.cpp`
  - MySQL connection wrapper, query helpers.
- `login.cpp`, `logout.cpp`
  - session creation and teardown.
- `state.cpp`
  - builds JSON state payload.
- `cmd.cpp`
  - command parsing and dispatch, calls game logic.
- `game.cpp`, `events.cpp`
  - turn/phase transitions, event logging utilities.
- `util.cpp`, `json.cpp`
  - helpers for JSON, string trimming, tokenization.
- `typs.h`
  - enums and shared structs (Phase enums, AuthContext, ShipRow, etc.)

**Critical design rule**
- No embedded star/system names or warpline lists in C++ source.
- All star/warpline queries must go to DB.

---

### 3.3 Request Handling Flow

For `/api/command`:

1. Parse HTTP request
2. Authenticate:
   - read `Authorization: Bearer ...`
   - lookup session → username
   - lookup username → seat for active game
3. Parse JSON body (contains `command` string)
4. Dispatch command
5. Command handler performs DB reads/writes
6. Log event text if applicable
7. Return JSON:
   - updated state snapshot is included

For `/api/state`:

1. Authenticate
2. Read current game state & player state from DB
3. Return JSON state snapshot
4. UI uses it to refresh status panel

---

### 3.4 Command Dispatch and Validation

The server uses a dispatcher in `cmd.cpp` roughly:

- Split the input line into tokens
- The first token determines command family:
  - `start`, `reset`, `next`
  - `build ...`
  - `deploy ...`
  - `pickup ...`, `drop ...`
  - `list ...`
  - `move ...`

Validation is layered:
- Global: authenticated, game exists, correct seat
- Phase: command allowed only in correct phase
- Rule: per-command constraints (e.g., ship exists, ship owned, etc.)

**Important**: error messages must be precise and consistent.
Example:
- “Not your turn. Active player: A”
- “Not in Movement phase. Current: Build Ships”

---

### 3.5 Turn Phases and Scenario Rules

Borealis turn structure is a known sequence, but the server uses a simplified approach:

- On entering a player’s turn, the server automatically computes victory points (VP).
- Then the interactive phase begins at **Build Ships**.

Scenarios affect:
- how many BP are granted (initial and per turn)
- how many base stars are active
- victory condition VP threshold

The server state includes:
- `scenario`
- `round`
- `active_player`
- `phase_index`
- `notes` (a short prompt to guide the current player)

The `notes` string is not authoritative state; it is an operator prompt.

---

### 3.6 Movement Implementation (Mode 1)

Mode 1 is the chosen approach: movement validation performed in C++ using DB topology, without geometry.

Key rules implemented/targeted:

- A Warpship has movement points equal to current PD (after damage; damage not implemented yet).
- Each adjacent-hex move costs 1.
- If a ship is on a hex that is part of a warpline trajectory, it may spend +1 to jump to an endpoint star hex.
- Movement does not require stopping on star hexes (unless enemy ship present; combat not implemented yet).

**Current v45 “move” command**
- `move W1 <DEST>` where DEST is either:
  - a system name (resolved via `star_systems`), or
  - a hex id (validated via `hexes`)

Implementation strategy:

1. Determine source hex (`ships.at_hex`).
2. Resolve destination hex:
   - if token matches a known star system name: use its `hex_id`
   - else interpret as hex id
3. Build neighbor relationships from `hexes` table:
   - each hex has up to 6 neighbors in axial coords
4. Add warp edges:
   - if `hex_id` ∈ `warpline_trajectory` for some warpline, connect to each endpoint hex of that warpline with cost 1
5. Run BFS/Dijkstra to compute minimal cost from source → destination.
6. Validate cost <= ship PD.
7. Update `ships.at_hex` and `ships.at_system` appropriately.
8. Emit event text with cost.

This design cleanly separates movement legality from UI map rendering.

---

### 3.7 JSON Responses and UI Coupling

Responses follow a consistent pattern:

- `ok: true|false`
- `error: <string>` if `ok=false`
- `event: <string>` optional (multi-line string appended to console log)
- `state: { ... }` always present on success, often present on error

**State fields**
- `gameId`
- `scenario`
- `round`
- `activePlayer`
- `phaseIndex`
- `phase`
- `vp: {A,B}`
- `bp: {A,B}` (may be restricted later)
- `notes`
- `peer` (optional): presence/phase info for opponent

UI is built to be resilient:
- even if `event` empty, state refresh occurs

---

## 4. UI Design

### 4.1 UI Principles

- The UI is an interactive terminal + status panel.
- It does not attempt to be authoritative.
- It can cache state, but always defers to server.
- It is resilient to refresh; session token may persist client-side.

No dynamic DB querying from HTML.
Apache serves static files; server provides only API.

---

### 4.2 HTML Structure

The main UI page includes:

- Left panel: console log (scrolling) and command input line
- Right panel: status summary
- Buttons:
  - Login/Logout
  - Map toggle (optional, static map view)

Console log is an append-only text area (div), with overflow scroll.

---

### 4.3 JavaScript Module Responsibilities

The UI JS is modular:

#### `constraints.js`
- Constants and fixed values (phase names, etc.)
- Any client-side regex/limits (e.g., ship name max length)

#### `slate.js`
- Client-side cached state (“last known server state”)
- Not authoritative; used for rendering

#### `behavior.js`
- Functions called by other JS modules
- API wrapper functions:
  - `apiLogin`, `apiLogout`, `apiState`, `apiCommand`
- State update and parsing:
  - update status panel
  - format event text

#### `interface.js`
- Wiring: DOM event handlers for buttons and input
- When user submits a command:
  - calls `BOREALIS.behavior.apiCommand(...)`
  - appends returned events to console
  - triggers `apiState()` refresh

---

### 4.4 UI Execution Flow

1. Page loads, JS modules initialize.
2. User clicks Login:
   - JS sends `/api/login`
   - stores token in slate or localStorage
3. UI polls `/api/state` periodically (or on command submit)
4. User enters command:
   - JS sends `/api/command` with token
   - receives JSON
   - appends `event` text lines to console log
   - updates status panel from `state`

Opponent awareness (peer status) is achieved by polling state; no push/SSE required initially.

---

### 4.5 Map View Behavior

Map view is a static HTML/SVG overlay.
The map does not currently draw ship counters.

Map toggle swaps the visible panel area between:
- Console log, and
- Map HTML embed

This is purely a visualization toggle; game state remains textual.

---

## 5. Rule Coverage and Roadmap

### 5.1 Currently implemented (typical v45 scope)

- Login / Logout
- Start scenario (`start basic|learning|advanced`)
- Reset game state
- Build draft workflow (`build new`, `build set/add/clear`, `build validate`, `build commit`)
- Deploy ships to base systems (scenario rules enforced)
- List ships
- Move warpships in Movement phase using DB-driven topology

### 5.2 Next steps (planned)

- Enforce “must stop movement on star hex occupied by enemy” (requires ship positions visible to both seats and collision checks)
- Combat phase implementation:
  - order writing
  - CRT lookup
  - screen absorption
  - damage allocation
  - retreat resolution
- Repair/resupply (Advanced)
- Full event log summarization / replay
- Map module generation from CSV seed data (SVG synthesis), not DB build-time queries

---

## Appendix A – Borealis Rules Referenced

This implementation directly uses these rule concepts:

- Ship attributes: PD, Beam, Screen, Tubes, Missiles, Systemship racks
- Build points spent 1:1 for attributes; warp generator cost special (warpship vs systemship)
- Movement cost per adjacent hex = 1 PD
- Warpline travel from occupied warpline hex to endpoints at cost 1 PD (your modified rule)
- Turn phases: VP check, Build, Movement, Combat, Pickup/Drop

Combat specifics are not yet implemented here, but the data model is designed to support it.

---

End of DESIGN.md
