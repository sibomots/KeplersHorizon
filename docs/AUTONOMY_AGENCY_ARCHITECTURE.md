# AutonomyAgency Architecture Analysis

## Executive Summary

This document evaluates the proposed `AutonomyAgency` design against the stated goals in `docs/AUTONOMY.md` and the existing AI implementation in `site/server/ai/`.

## Current State

### Existing AI Components

| File | Purpose | Pattern |
|------|---------|---------|
| `aiagent.h/cpp` | Orchestrator; event-driven turn control | Meyers Singleton |
| `ailogic.h/cpp` | Decision engine; phase-specific logic | Meyers Singleton |
| `aigamestate.h/cpp` | DB query wrapper for AI decisions | Stateless per-call |
| `ai_command_injector.h/cpp` | Injects commands via same parser as human | Meyers Singleton |
| `ai_db_mutex.h/cpp` | Synchronization for AI/DB operations | Meyers Singleton |

**Current Flow:**
```
StateMachine → AIAgent.on_turn_start() → AILogic.decide_*() → AICommandInjector.inject()
         ↑                                                           ↓
     TaskRunner ←←←←←←←←←←←←←←← Task ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
```

The existing design is synchronous and event-driven. `AIAgent` responds to `on_turn_start()`, `on_phase_advance()`, etc. and pushes `Task` objects into `TaskRunner`. No dedicated thread.

### Proposed AutonomyAgency (autonomy_agency.h/cpp)

Currently disabled via `#ifdef HAS_BEEN_REFACTORED`. Proposes:

1. **Dedicated worker thread** with condition-variable pump
2. **Interface-based DI** for collectors, cookers, planners, renderers
3. **ECL/Lisp DSL** integration stub for the planner
4. **Mealy State Machine** with Raw → Cook → Plan → Render → Telemeter pipeline
5. **Combat tight-loop** with yield timing and round caps

**Missing Definitions:**
- `GameSnapshot` struct (referenced but not defined)
- `CookedInputs` struct (referenced but not defined)

---

## Gap Analysis: AUTONOMY.md vs Implementation

| AUTONOMY.md Requirement | Current AIAgent | Proposed AutonomyAgency |
|-------------------------|-----------------|-------------------------|
| Threaded while(!done) runner | No thread; event-driven | Yes (`thread_main()`) |
| Mealy State Machine | Implicit via TurnState enum | Explicit MSS stages |
| Gather Inputs | Via `AIGameState` queries | `IRawCollector` interface + `ICooker` |
| Calculations (ECL/DSL) | `AILogic` C++ only | `IPlanner` stub for ECL |
| Render/Inject | `AICommandInjector::inject()` | `IRenderer` + `ICommandInjector` |
| Telemeter | Not implemented | `ITelemetrySink` interface |
| Atomic phase gate | Not implemented | `IAtomicPhaseGate` interface |
| Combat loop | `on_combat_detected()` stub | `combat_loop_if_needed()` |

---

## Proposed Refined Architecture

### Design Principles

1. **Retain Meyers Singletons** for cross-cutting concerns (logging, DB access)
2. **Interface DI** for pluggable logic (planner, renderer, telemetry)
3. **Single Return Policy** per CLAUDE.md
4. **Explicit Bracing, No Namespaces** per CLAUDE.md
5. **Return bool/int status; pass data by reference** per CLAUDE.md

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           AutonomyAgency (Thread Owner)                     │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        Mealy State Machine Loop                     │    │
│  │   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────────┐    │    │
│  │   │ Gather   │──▶│  Cook    │──▶│ Calculate│──▶│   Render     │    │    │
│  │   │ Raw      │   │ Inputs   │   │ Plan     │   │ + Inject     │    │    │
│  │   └──────────┘   └──────────┘   └──────────┘   └──────────────┘    │    │
│  │         │                             │                │           │    │
│  │         ▼                             ▼                ▼           │    │
│  │   ┌──────────┐                  ┌──────────┐    ┌──────────────┐   │    │
│  │   │StateMach │                  │ IPlanner │    │ICommandInject│   │    │
│  │   │ + DB     │                  │ (ECL)    │    │ (existing)   │   │    │
│  │   └──────────┘                  └──────────┘    └──────────────┘   │    │
│  │                                                        │           │    │
│  │                                              ┌─────────▼────────┐  │    │
│  │                                              │  ITelemetrySink  │  │    │
│  │                                              └──────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│             ▲                                                               │
│             │ pump(PumpReason)                                              │
│             │                                                               │
│  ┌──────────┴──────────┐                                                    │
│  │   Message Queue     │◀─── UI poll / player cmd / server state change     │
│  └─────────────────────┘                                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Struct Definitions (Missing from Current Code)

```cpp
struct GameSnapshot
{
    uint32_t session_id;
    uint32_t turn_number;

    enum class Phase : uint8_t
    {
        BuildShips,
        Movement,
        Combat,
        PickupDrop,
        EndTurn
    };
    Phase phase;

    bool aa_is_active_player;
    bool game_done;
    bool in_combat;

    // Contested hex IDs (for combat resolution)
    std::vector<std::string> contested_hexes;
};

struct CookedInputs
{
    uint32_t session_id;
    uint32_t turn_number;
    GameSnapshot::Phase phase;

    bool aa_is_active_player;
    bool game_done;
    bool in_combat;

    bool movement_atomic_now;
    bool combat_atomic_now;

    std::vector<std::string> contested_hexes_sorted;
    uint32_t monotonic_cycle_id;

    // Extended cooked data (credits, ships, enemy intel, etc.)
    int credits;
    int tech_level;
    std::string base_hex;
};
```

### Integration with Existing Code

**Keep:**
- `AICommandInjector` as the concrete `ICommandInjector`
- `AIDBMutex` for DB synchronization
- `AIGameState` queries (wrap in `IRawCollector` impl)

**Replace:**
- `AIAgent` orchestration → `AutonomyAgency.thread_main()`
- `AILogic` decision methods → `IPlanner` implementation (ECL-backed later)

**Adapter Pattern:**
```cpp
// Wrap existing AICommandInjector for ICommandInjector interface
class CommandInjectorAdapter : public ICommandInjector
{
public:
    void inject(const InjectedCommand& cmd) override
    {
        // Extract game_id/player from context
        AICommandInjector::inject(m_game_id, m_ai_player, cmd.text);
    }
private:
    int m_game_id;
    char m_ai_player;
};
```

### Combat Loop Refinement

The `combat_loop_if_needed()` in proposed code is correct in structure but needs:

1. **Round-robin fairness** if multiple combats occur
2. **Yield to human player** for damage assignment (per rules)
3. **State machine sub-states**: ORDER → COMMIT → ASSIGN_DAMAGE

```cpp
enum class CombatSubState : uint8_t
{
    IDLE,
    DRAFTING_ORDER,
    AWAITING_COMMIT,
    ASSIGNING_DAMAGE,
    ROUND_COMPLETE
};
```

### ECL Integration Point

The `IPlanner::decide()` method is the ECL boundary:

```cpp
// Future ECL planner implementation
class EclPlanner : public IPlanner
{
public:
    bool init(const std::vector<std::string>& lisp_paths);

    Plan decide(const CookedInputs& cooked, const Slate& slate) override
    {
        // 1. Marshal CookedInputs → Lisp S-expression
        // 2. cl_eval() the decision function
        // 3. Unmarshal result → Plan struct
    }
private:
    // ECL environment handle
};
```

---

## Recommended Next Steps

1. **Define `GameSnapshot` and `CookedInputs`** in `autonomy_agency.h`
2. **Implement `IRawCollector`** wrapping `AIGameState` + `StateMachine` queries
3. **Create `CommandInjectorAdapter`** bridging `ICommandInjector` to existing `AICommandInjector`
4. **Stub `ITelemetrySink`** (log-to-console initially)
5. **Remove `#ifdef HAS_BEEN_REFACTORED`** guard and resolve compile errors
6. **Wire into server** via singleton accessor or initialization hook
7. **ECL integration** as a later phase (keep `IPlanner` as C++ stub first)

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Thread safety with StateMachine | Use `AIDBMutex` for all DB/state access |
| ECL complexity | Defer; pure C++ `IPlanner` first |
| Combat state race | `IAtomicPhaseGate` locks movement/combat phases |
| Over-engineering | Keep interfaces minimal; no speculative features |

---

## Files to Modify/Create

| Action | File |
|--------|------|
| Update | `site/server/ai/inc/autonomy_agency.h` (add missing structs) |
| Update | `site/server/ai/src/autonomy_agency.cpp` (remove ifdef guard) |
| Create | `site/server/ai/inc/raw_collector.h` |
| Create | `site/server/ai/src/raw_collector.cpp` |
| Create | `site/server/ai/inc/command_injector_adapter.h` |
| Create | `site/server/ai/src/command_injector_adapter.cpp` |
| Create | `site/server/ai/inc/telemetry_sink.h` |
| Create | `site/server/ai/src/telemetry_sink.cpp` |
| Deprecate | `site/server/ai/inc/aiagent.h` (after AutonomyAgency stable) |
| Deprecate | `site/server/ai/src/aiagent.cpp` (after AutonomyAgency stable) |

---

## Summary

The `AutonomyAgency` design aligns with `AUTONOMY.md` goals. The existing `AIAgent` is a simpler event-driven approach that works but lacks threading, telemetry, and ECL hooks. The proposed architecture introduces proper MSS, interface-based DI, and a clear path to ECL integration. Missing struct definitions must be added before the code compiles. Adapter pattern bridges old `AICommandInjector` to new interfaces.
