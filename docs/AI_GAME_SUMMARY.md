# AI Agent Game Summary

## Turn Phases (in order)

| # | Phase | AI Action |
|---|-------|-----------|
| 0 | Count VP | Server handles (1 VP per enemy base occupied) |
| 1 | Build Ships | `bn`, `bs`, `bc`, `ds`, `rp`, `rs`, then `NEXT` |
| 2 | Movement | `m SHIP hXXYY`, then `NEXT` |
| 3 | Resolve Combat | `co`, `cc`, `ca`, `retreat`, then `NEXT` |
| 4 | Pick/Drop | `pick`, `drop`, then `NEXT` |
| 5 | End Turn | `DONE` |

## Ship Building Costs

| Attribute | Cost | Purpose |
|-----------|------|---------|
| WG | 5 BP | Warp Generator (makes it a warpship) |
| PD | 1 BP/unit | Power/Drive - movement AND combat power |
| B | 1 BP/unit | Beam weapon strength |
| S | 1 BP/unit | Screen defense (absorbs hits = power + tech) |
| T | 1 BP/tube | Missile launcher (needs 1 PD to fire) |
| M | 1 BP/3 | Missiles (2 base damage + tech level) |
| SR | 1 BP/rack | Carries 1 systemship |

**Basic Fighter**: WG(5) + PD(5) + B(3) + S(2) + T(1) + M(3) = 17 BP

## Movement

- 1 PD = 1 hex or 1 warpline jump
- **Must stop** at enemy-occupied star hexes (triggers combat)
- Pick/drop systemship costs 1 PD each
- Command: `m SHIPNAME hXXYY` (hex) or `m SHIPNAME STARNAME` (star)

## Combat

### Tactics
| Tactic | Code | Use When |
|--------|------|----------|
| Attack | `a` | Stronger than enemy |
| Dodge | `d` | Equal strength |
| Retreat | `r` | Outmatched (warpships only) |

### Power Allocation
- `d=N` Drive (maneuver)
- `b=N` Beam (weapon)
- `s=N` Screen (defense)
- `t=N` Tubes (missiles)

**Constraint**: d + b + s + t <= current PD
**Constraint**: Beams/Screens and Missiles cannot be used same round

### Combat Order Example
```
co W1 a S25 d=2 b=3 s=0
cc
```
W1 attacks S25 with Attack tactic, 2 drive, 3 beam, 0 screen.

### Damage Application
After resolution, assign damage to attributes:
```
ca W1 pd=2 b=1
```
Ship destroyed when PD + B + S + T = 0.

### Stalemate Rule
3 consecutive rounds with no unscreened damage = attacker must withdraw all ships.

## Victory

- **Advanced Scenario**: 3 VP to win
- 1 VP = occupying enemy base star at start of YOUR turn
- Each side has 3 base stars (ARVEN/BELIX/CAYRU vs ZAREK/ASTREX/BRION)

## Tech Levels

- Turns 1-4: Level 0
- Turns 5-8: Level 1
- Turns 9-12: Level 2
- etc.

Tech adds to:
- Beam damage
- Missile damage
- Screen absorption

## Repair/Resupply (at base only)

- `rp SHIP pd=2 b=1` - Repair (1 BP per attribute point)
- `rs SHIP m=6` - Resupply missiles (1 BP per 3 missiles)
- Cannot exceed original build values

## AI Strategy Summary

### Build Phase
1. Commit pending drafts
2. Deploy uncommitted ships
3. Build if: credits > (ship_cost + reserve) AND fleet_size < limit
4. Repair damaged ships at base
5. Resupply missiles
6. `NEXT`

### Movement Phase
1. Find ships with remaining PD (base PD - pd_spent)
2. Compute path to enemy base
3. Move one ship at a time
4. `NEXT` when no ships can move

### Combat Phase
1. For each ship in combat:
   - Select tactic based on strength ratio
   - Select target (weakest or most dangerous)
   - Allocate power
2. `cc` to commit
3. After resolution: `ca` to assign damage (preserve PD)
4. `retreat SHIP hXXYY` if escape succeeded
5. `NEXT` when combat resolved

### End Turn
- `DONE`
