# Design Patterns Analysis

*Analysis of GoF Design Patterns in Kepler's Horizon Server*

---

## Patterns Currently in Use

### Singleton (Creational)
**Files:** `db.h`, `statemachine.h`, `telemetry.h`, `roommanager.h`, `logger.h`

All major service classes use the Meyer's Singleton pattern:
```cpp
static DatabaseManager& getInstance() {
    static DatabaseManager instance;
    return instance;
}
```

**Assessment:** Appropriate for global services (DB, telemetry, state). Consider if StateMachine should be per-game rather than global singleton.

---

### Command (Behavioral)
**Files:** `icmd.h`, all `*_command.h` files

Clean implementation with `ICmd` interface:
```cpp
class ICmd {
    virtual bool invoke() = 0;
};
```

**Concrete commands:** BuildCommand, MoveCommand, DeployCommand, CombatOrderCommand, SaveCommand, etc.

**Assessment:** Well-implemented. Commands decouple request senders from receivers.

---

### Builder (Creational)
**Files:** All command headers

Each command has a nested Builder class:
```cpp
class BuildCommand : public ICmd {
    class Builder {
        Builder& set_draft_code(const std::string& code);
        ICmd* build();
    };
};
```

**Assessment:** Good for constructing commands with many optional parameters.

---

### Factory Method (Creational)
**Files:** `ships.h`

`ShipRow::from_draft()` is a static factory method:
```cpp
static ShipRow from_draft(const DraftRow& draft, int tech_level, 
                          int round, const std::string& active_player);
```

**Assessment:** Used sparingly. More opportunities exist.

---

### Facade (Structural)
**Files:** `combat.h`, `statemachine.h`

`CombatEngine` provides a simplified interface to combat subsystem:
- `check_for_combat_triggers()`
- `submit_order()`
- `resolve_round()`
- `apply_damage()`

**Assessment:** Good encapsulation of complex combat logic.

---

## Opportunities for New Patterns

### 1. Strategy (Behavioral)
**Opportunity:** Combat tactics (Attack, Defend, Evade/Retreat)

**Current:** Tactics are handled via switch/if statements in `Combat.cpp`

**Proposed:**
```cpp
class CombatTactic {
public:
    virtual int calculateToHit(ShipRow& attacker, ShipRow& defender) = 0;
    virtual int calculateDamage(int roll, int power) = 0;
};

class AttackTactic : public CombatTactic { ... };
class DefendTactic : public CombatTactic { ... };
class EvadeTactic : public CombatTactic { ... };
```

**Benefit:** New tactics can be added without modifying existing code.

---

### 2. State (Behavioral)
**Opportunity:** Game phase management

**Current:** `phase_index` integer with switch statements in StateMachine

**Proposed:**
```cpp
class GamePhase {
public:
    virtual void onEnter(GameState& state) = 0;
    virtual void onExit(GameState& state) = 0;
    virtual bool canExecuteCommand(CommandID cmd) = 0;
    virtual GamePhase* nextPhase() = 0;
};

class BuildShipsPhase : public GamePhase { ... };
class MovementPhase : public GamePhase { ... };
class CombatPhase : public GamePhase { ... };
```

**Benefit:** Phase-specific logic isolated. Phase transitions explicit.

---

### 3. Observer (Behavioral)
**Opportunity:** Event notification system

**Current:** Telemetry writes are scattered through command code

**Proposed:**
```cpp
class GameEventObserver {
public:
    virtual void onShipBuilt(const ShipRow& ship) = 0;
    virtual void onShipMoved(const ShipRow& ship, const std::string& from, 
                             const std::string& to) = 0;
    virtual void onCombatStarted(const std::string& hex) = 0;
};

class TelemetryObserver : public GameEventObserver { ... };
class LoggingObserver : public GameEventObserver { ... };
```

**Benefit:** Decouple game logic from notification. Easy to add new observers (achievements, analytics).

---

### 4. Memento (Behavioral)
**Opportunity:** Game state save/load

**Current:** `GameState::to_json()` and `from_json_min()` handle serialization

**Proposed:**
```cpp
class GameMemento {
private:
    friend class Game;
    std::string state_json;
    GameMemento(const std::string& json) : state_json(json) {}
};

class Game {
    GameMemento saveState();
    void restoreState(const GameMemento& memento);
};
```

**Benefit:** Encapsulates save/restore logic. Better undo/redo support.

---

### 5. Repository (Domain-Driven Design)
**Opportunity:** Ship and draft data access

**Current:** Free functions in `ships.h`:
- `load_ships()`, `load_ship()`, `insert_ship()`, `update_ship_location()`

**Proposed:**
```cpp
class ShipRepository {
public:
    std::vector<ShipRow> findByOwner(int game_id, char owner);
    ShipRow findByCode(int game_id, char owner, const std::string& code);
    void save(int game_id, char owner, const ShipRow& ship);
    void updateLocation(int game_id, char owner, const std::string& code,
                        const std::string& hex, const std::string& system);
};
```

**Benefit:** Centralized data access. Easier to mock for testing.

---

### 6. Composite (Structural)
**Opportunity:** Path segments in movement

**Current:** `fullPath` is a vector of hex strings

**Proposed:**
```cpp
class PathSegment {
public:
    std::string from;
    std::string to;
    bool isWarpline;
    int cost;
};

class Path {
    std::vector<PathSegment> segments;
    int totalCost();
    std::string toDisplayString();
};
```

**Benefit:** Rich path representation. Cleaner display logic.

---

### 7. Chain of Responsibility (Behavioral)
**Opportunity:** Command validation/inhibits

**Current:** `StateMachine::check_inhibits()` handles all validation

**Proposed:**
```cpp
class CommandValidator {
protected:
    CommandValidator* next;
public:
    void setNext(CommandValidator* v) { next = v; }
    virtual bool validate(CommandID cmd, void* params, std::string& error) = 0;
};

class PhaseValidator : public CommandValidator { ... };
class TurnValidator : public CommandValidator { ... };
class ResourceValidator : public CommandValidator { ... };
```

**Benefit:** Validation logic distributed. Easy to add new validators.

---

### 8. Template Method (Behavioral)
**Opportunity:** Command execution flow

**Current:** Each command's `invoke()` has similar structure (check inhibits, execute, save state, telemetry)

**Proposed:**
```cpp
class BaseCommand : public ICmd {
protected:
    virtual bool checkPreconditions() = 0;
    virtual bool doExecute() = 0;
    virtual void postExecute() {}
public:
    bool invoke() final {
        if (!checkPreconditions()) return false;
        bool result = doExecute();
        if (result) postExecute();
        return result;
    }
};
```

**Benefit:** Consistent command flow. Reduced boilerplate.

---

## Priority Recommendations

| Priority | Pattern | Benefit |
|----------|---------|---------|
| High | **Strategy** (combat tactics) | Extensible combat system |
| High | **State** (game phases) | Cleaner phase transitions |
| Medium | **Repository** (ships) | Testable data access |
| Medium | **Template Method** (commands) | Reduced boilerplate |
| Low | **Observer** (events) | Decoupled notifications |
| Low | **Composite** (paths) | Cleaner path handling |

---

## Files Reviewed

### Source Files (42)
`Build.cpp`, `Combat.cpp`, `Move.cpp`, `Turn.cpp`, `statemachine.cpp`, `telemetry.cpp`, `ships.cpp`, `mapgraph.cpp`, `roommanager.cpp`, and 33 others.

### Header Files (54)
`icmd.h`, `statemachine.h`, `combat.h`, `ships.h`, `constraints.h`, and 49 command/utility headers.
