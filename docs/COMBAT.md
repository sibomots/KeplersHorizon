# Combat Decision Tree

## Combat Stages
- **Stage 0**: ORDERS - Awaiting combat orders
- **Stage 1**: RESOLVE_READY - Ready to resolve (all committed)
- **Stage 2**: DAMAGE_PENDING - Awaiting damage assignment
- **Stage 3**: RETREAT_PENDING - Awaiting retreat commands (stalemate)

---

## Scenario 1: Normal Combat Round (Damage Dealt)

```mermaid
sequenceDiagram
    participant A as Player A
    participant B as Player B
    participant CE as CombatEngine
    participant DB as Database

    Note over CE: Stage 0 (ORDERS)
    A->>CE: combat order W1 S20 A d=3 b=2
    CE->>DB: INSERT combat_orders (committed=0)
    A->>CE: combat commit
    CE->>DB: UPDATE committed=1
    CE-->>B: "Player A committed"
    
    B->>CE: combat order S20 W1 A d=4 b=1
    CE->>DB: INSERT combat_orders (committed=0)
    B->>CE: combat commit
    CE->>DB: UPDATE committed=1
    
    Note over CE: all_orders_committed() = true
    CE->>CE: resolve_round()
    CE->>CE: Beam fire loop (CRT calc)
    CE->>CE: Missile fire loop
    CE->>DB: UPDATE missiles (deduct fired)
    CE->>CE: Calculate net damage
    
    Note over CE: total_net_damage > 0
    CE->>DB: UPDATE stage=2
    CE-->>A: "DAMAGE REPORT: W1 took 3 damage"
    CE-->>B: "DAMAGE REPORT: S20 took 2 damage"
    
    Note over CE: Stage 2 (DAMAGE_PENDING)
    A->>CE: combat apply W1 pd=2 b=1
    CE->>CE: Validate (assigned==needed)
    CE->>DB: UPDATE ships SET pd=pd-2, beam=beam-1
    
    B->>CE: combat apply S20 pd=2
    CE->>DB: UPDATE ships SET pd=pd-2
    
    Note over CE: All damage assigned
    CE->>DB: UPDATE stage=0, round++
    CE-->>A: "Round 2 begins"
    CE-->>B: "Round 2 begins"
```

---

## Scenario 2: Successful Retreat

```mermaid
sequenceDiagram
    participant A as Player A
    participant B as Player B
    participant CE as CombatEngine
    participant MG as MapGraph
    participant DB as Database

    Note over CE: Stage 0 (ORDERS)
    A->>CE: combat order W1 S20 R d=5
    Note right of A: Tactic 'R' = Retreat
    A->>CE: combat commit
    
    B->>CE: combat order S20 W1 A d=2 b=3
    B->>CE: combat commit
    
    CE->>CE: resolve_round()
    CE->>CE: Beam fire: S20 fires at W1
    CE->>CE: CRT result: W1 ESCAPES (drive diff favorable)
    Note over CE: escape_attempts=1, escape_successes=1
    
    CE->>CE: Retreat Logic: W1 tactic='R'
    Note over CE: escape_successes == escape_attempts
    CE->>DB: UPDATE ships SET escape_pending=1 (W1)
    CE->>MG: get_adjacent_hexes("0505")
    MG-->>CE: ["0504", "0506", "0405"]
    CE-->>A: "W1 successfully retreats!"
    CE-->>A: ">> Issue 'retreat W1 <hex>'"
    CE-->>A: "Valid destinations: 0504 0506 0405"
    
    Note over A: USER MUST ACT (turn blocked until complete)
    A->>CE: retreat W1 0504
    CE->>MG: Validate 0504 is adjacent
    CE->>DB: UPDATE ships SET at_hex='0504', escape_pending=0
    CE-->>A: "RETREAT: W1 withdraws to 0504"
```

---

## Scenario 3: Failed Retreat

```mermaid
sequenceDiagram
    participant A as Player A
    participant B as Player B
    participant CE as CombatEngine
    participant DB as Database

    Note over CE: Stage 0 (ORDERS)
    A->>CE: combat order W1 S20 R d=2
    Note right of A: Low drive power
    A->>CE: combat commit
    
    B->>CE: combat order S20 W1 A d=5 b=4
    Note right of B: High drive power
    B->>CE: combat commit
    
    CE->>CE: resolve_round()
    CE->>CE: Beam fire: S20 fires at W1
    CE->>CE: CRT result: W1 HIT (drive diff unfavorable)
    Note over CE: escape_attempts=1, escape_successes=0
    CE->>CE: W1.damage_received += damage
    
    CE->>CE: Retreat Logic: W1 tactic='R'
    Note over CE: escape_successes != escape_attempts
    CE-->>A: "W1 failed to retreat."
    CE-->>A: "Ship remains in combat."
    
    Note over CE: W1 still in hex, participates in Round 2
```

---

## Scenario 4: Stalemate (3 Rounds No Damage)

```mermaid
sequenceDiagram
    participant Init as Initiative Player
    participant CE as CombatEngine
    participant MG as MapGraph
    participant DB as Database

    Note over CE: Round 1: stalemate_counter=1
    Note over CE: Round 2: stalemate_counter=2
    Note over CE: Round 3: stalemate_counter=3
    
    CE->>CE: resolve_round()
    CE->>CE: total_net_damage = 0
    CE->>CE: stalemate_counter >= 3
    
    CE->>DB: UPDATE stage=3 (RETREAT_PENDING)
    CE-->>Init: "STALEMATE: 3 rounds with no damage!"
    CE-->>Init: "You must withdraw all ships from hex"
    
    Note over CE: Stage 3 (RETREAT_PENDING)
    Note over Init: USER MUST retreat ALL ships
    
    Init->>CE: retreat W1 0504
    CE->>MG: Validate adjacent
    CE->>DB: UPDATE at_hex, escape_pending=0
    
    Init->>CE: retreat W2 0506
    CE->>MG: Validate adjacent
    CE->>DB: UPDATE at_hex, escape_pending=0
    
    Note over CE: All initiative ships retreated
    CE->>DB: DELETE combat_state
    CE-->>Init: "Combat ends - opponent controls hex"
```

---

## Scenario 5: Ship Destruction (Overkill)

```mermaid
sequenceDiagram
    participant A as Player A
    participant CE as CombatEngine
    participant DB as Database

    Note over CE: Stage 2 (DAMAGE_PENDING)
    Note over A: W1 has 5 HP, took 8 damage
    
    A->>CE: combat apply W1 pd=3 b=2
    Note right of A: assigned=5, needed=8
    
    CE->>CE: Validate assignment
    CE->>CE: assigned(5) < needed(8)
    CE->>CE: BUT: assigned(5) >= current_hp(5)
    Note over CE: Ship will be destroyed - VALID
    
    CE->>DB: UPDATE ships SET pd=0, beam=0
    CE->>CE: Check destruction: pd=b=s=t=0
    CE->>DB: UPDATE ships SET destroyed_at=NOW()
    CE->>DB: UPDATE games SET vp_B += 1
    CE-->>B: "VICTORY: +1 VP for destroying W1"
    CE-->>A: "Damage Applied. W1 destroyed."
```

---

## Scenario 6: Turn Blocked by Pending Retreat

```mermaid
sequenceDiagram
    participant A as Player A
    participant Done as DoneCommand
    participant DB as Database

    Note over A: Has escape_pending=1 ships
    
    A->>Done: done
    Done->>DB: SELECT ships WHERE escape_pending=1
    DB-->>Done: [W1, W2]
    
    Done-->>A: "TACTICAL: Retreat pending!"
    Done-->>A: "2 ship(s) must complete withdrawal:"
    Done-->>A: "  - W1"
    Done-->>A: "  - W2"
    Done-->>A: "Use: retreat <ship> <hex>"
    
    Note over A: Turn NOT advanced
```

---

## Decision Tree

```
┌─────────────────────────────────────────────────────────────────────┐
│                     COMBAT TRIGGERED                                 │
│  System: create_combat() inserts combat_state with stage=0          │
│  System: Notifies both players of combat in hex                     │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     STAGE 0: ORDERS                                  │
├─────────────────────────────────────────────────────────────────────┤
│  USER A: 'combat order <ship> <target> <tactic> [power alloc]'      │
│  USER A: 'combat commit' (marks orders committed)                    │
│  USER B: 'combat order <ship> <target> <tactic> [power alloc]'      │
│  USER B: 'combat commit' (marks orders committed)                    │
├─────────────────────────────────────────────────────────────────────┤
│  System checks: all_orders_committed()?                              │
│    NO  → Stay in Stage 0, notify waiting player                     │
│    YES → Call resolve_round()                                        │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                     all_orders_committed() == true
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     resolve_round() EXECUTION                        │
├─────────────────────────────────────────────────────────────────────┤
│  Ship Loading: SELECT ... WHERE escape_pending=0                    │
│  (Ships pending retreat are EXCLUDED from combat)                   │
├─────────────────────────────────────────────────────────────────────┤
│  1. BEAM FIRE LOOP (for each ship in hex)                           │
│     ├─ IF power_b > 0 AND has target:                               │
│     │     System: Calculate CRT result                              │
│     │     System: Track escape_attempts/escape_successes on TARGET  │
│     │     System: Apply damage to target.damage_received            │
│     ├─ ELSE IF tactic == 'R':                                       │
│     │     System: Log "holds fire (Retreating)"                     │
│     └─ ELSE:                                                        │
│           (ship not attacking, no special action)                   │
├─────────────────────────────────────────────────────────────────────┤
│  2. MISSILE FIRE LOOP (for each ship in hex)                        │
│     ├─ IF missiles_data not empty AND power_t >= count:             │
│     │     System: Calculate CRT for each missile                    │
│     │     System: Track escape_attempts/escape_successes on TARGET  │
│     │     System: Apply damage to target.damage_received            │
│     │     System: DEDUCT missiles from ships.missiles               │
│     └─ ELSE:                                                        │
│           (no missiles fired)                                       │
├─────────────────────────────────────────────────────────────────────┤
│  3. NET DAMAGE CALCULATION (for each ship)                          │
│     System: net = damage_received - (power_s * shield_absorb)       │
│     System: Sum total_net_damage                                    │
├─────────────────────────────────────────────────────────────────────┤
│  4. RETREAT LOGIC (for each ship with tactic == 'R')                │
│     ├─ IF escape_attempts == 0 (unopposed):                         │
│     │     escape = true                                             │
│     ├─ ELSE IF escape_successes == escape_attempts (eluded all):    │
│     │     escape = true                                             │
│     └─ ELSE:                                                        │
│           escape = false                                            │
│                                                                      │
│     ├─ IF escape == true:                                           │
│     │     System: SET escape_pending=1                              │
│     │     System: Use MapGraph to list valid hexes                  │
│     │     System: Log "successfully retreats! Issue 'retreat...'"  │
│     │     ──► USER MUST: 'retreat <ship> <hex>'                    │
│     │         (Turn blocked until complete - enforced in Turn.cpp) │
│     │                                                                │
│     └─ ELSE (escape == false):                                      │
│           System: Log "failed to retreat. Ship remains in combat." │
│           (Ship continues in next round)                            │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     BRANCH: total_net_damage > 0?                    │
├──────────────────────────────┬──────────────────────────────────────┤
│           YES                │              NO                       │
│    Stage → 2 (DAMAGE)        │    stalemate_counter++                │
│    Reset stalemate           │                                       │
│                              │    IF stalemate >= 3:                 │
│                              │       Stage → 3 (RETREAT_PENDING)     │
│                              │       System: Notify initiative       │
│                              │         player must withdraw all      │
│                              │    ELSE:                              │
│                              │       next_round++                    │
│                              │       Stage → 0 (back to ORDERS)      │
│                              │       System: Broadcast next round    │
└──────────────────────────────┴──────────────────────────────────────┘
                │                              │
                ▼                              │
┌───────────────────────────────┐              │
│  STAGE 2: DAMAGE_PENDING      │              │
├───────────────────────────────┤              │
│  USER A (if has damage):      │              │
│    'combat apply <ship>       │              │
│     pd=N b=N s=N t=N m=N'     │              │
│                               │              │
│  USER B (if has damage):      │              │
│    'combat apply <ship>       │              │
│     pd=N b=N s=N t=N m=N'     │              │
├───────────────────────────────┤              │
│  VALIDATION:                  │              │
│  ├─ assigned > needed:        │              │
│  │   ├─ assigned >= HP: OK    │              │
│  │   │   (overkill accepted)  │              │
│  │   └─ assigned < HP: ERROR  │              │
│  │                            │              │
│  ├─ assigned < needed:        │              │
│  │   ├─ assigned >= HP: OK    │              │
│  │   │   (ship destroyed)     │              │
│  │   └─ assigned < HP: ERROR  │              │
│  │                            │              │
│  └─ assigned == needed: OK    │              │
├───────────────────────────────┤              │
│  System: Apply to attributes  │              │
│  System: Check destruction    │              │
│    IF pd=b=s=t=0:             │              │
│       destroyed_at=NOW()      │              │
│       Award VP to enemy       │              │
├───────────────────────────────┤              │
│  All damage assigned?         │              │
│    NO  → Stay Stage 2         │              │
│    YES → Check combat end     │              │
│      ├─ One side eliminated:  │              │
│      │   DELETE combat_state  │              │
│      │   Combat ENDS          │──────────────┼────► EXIT
│      └─ Both sides remain:    │              │
│          next_round++         │              │
│          Stage → 0            │◄─────────────┘
└───────────────────────────────┘

                                  │  (from stalemate >= 3)
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     STAGE 3: RETREAT_PENDING (Stalemate)            │
├─────────────────────────────────────────────────────────────────────┤
│  Initiative player MUST retreat ALL ships from hex                  │
│                                                                      │
│  USER (initiative): 'retreat <ship> <hex>' for EACH ship            │
│                                                                      │
│  System: Validates via MapGraph::get_adjacent_hexes()               │
│  System: Moves ship, clears escape_pending                          │
├─────────────────────────────────────────────────────────────────────┤
│  All initiative ships retreated?                                    │
│    YES → Combat ENDS (other player controls hex)                    │
│    NO  → Turn blocked (enforced in DoneCommand)                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Enforcement Mechanisms (All Implemented)

| Gap | Fix Location | Implementation |
|-----|--------------|----------------|
| Turn advance with pending retreat | `Turn.cpp` DoneCommand | Query `escape_pending=1`, block if found |
| Ship in combat with pending retreat | `Combat.cpp` resolve_round | Ship load query includes `AND escape_pending=0` |

---

## Summary: User vs System Responsibilities

| State | User Action | System Action |
|-------|-------------|---------------|
| Combat Triggered | - | create_combat(), notify players |
| Stage 0 | `combat order`, `combat commit` | Validate, store, check all committed |
| Resolution | - | CRT, damage calc, retreat detection |
| Stage 2 | `combat apply <ship> <assignments>` | Validate assignment, apply, check destruction |
| Retreat Success | `retreat <ship> <hex>` | Validate hex (MapGraph), move ship |
| Stage 3 | `retreat <ship> <hex>` (all ships) | Validate, end combat |
| Turn Advance | `done` or `next` | **BLOCK if escape_pending** |
