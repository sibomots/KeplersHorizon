# Kepler's Horizon Strategy Guide

## Victory Conditions

- **3 VP to win** (Advanced Scenario)
- VP earned at **start of your turn** for each enemy base-star hex occupied uncontested
- Key insight: You must **spread forces** across multiple enemy bases to maximize VP per turn

---

## Economic Strategy

### Build Point Flow (Advanced)
- Turn 1: 20 BP
- Turn 2+: 10 BP per turn
- BP can be saved for later use

### Early Game Investment Priorities

1. **Warp Generator Tax**: Each warpship costs 5 BP just to exist. Early game, this is 25-50% of your budget.

2. **The Systemship Gambit**: Systemships save 5 BP but can't move alone or retreat. Consider:
   - Building a "carrier" warpship (high PD, high SR, minimal weapons) to ferry combat systemships
   - Systemships as base defenders (they don't need to move if enemies come to you)

3. **Tech Level Timing**: Tech advances every 4 turns (turns 1-4 = Level 0, 5-8 = Level 1, etc.)
   - Ships built at higher tech add that level to beam damage, missile damage, AND screen absorption
   - **Strategic choice**: Build heavy early (more ships now) vs. save BP for tech 1+ ships (fewer but stronger)

### BP Efficiency Table

| Attribute | Cost | Notes |
|-----------|------|-------|
| PD | 1 BP | Most versatile - movement AND combat |
| Beam | 1 BP | Reusable, adds to hit damage |
| Screen | 1 BP | Absorbs damage (+ tech level if powered) |
| Tube | 1 BP | Fires 1 missile/round, needs 1 PD to power |
| Missiles (3) | 1 BP | Consumable, own drive rating |
| SR | 1 BP | Carry 1 systemship |
| Warp Gen | 5 BP | Required for warpship mobility |

---

## Ship Design Archetypes

### The Brawler (17-22 BP Warpship)
```
PD=6-7, B=4-5, S=3-4, T=0-1, M=0-3, SR=0
```
- High PD for drive advantage in CRT
- Strong beam for consistent damage
- Good screens for survivability
- Purpose: Main battle line, can take and deal punishment

### The Interceptor (12-14 BP Warpship)
```
PD=5-6, B=2-3, S=2, T=0, M=0, SR=0
```
- Fast, can chase retreating enemies
- Light weapons but decent screen
- Purpose: Hunt damaged ships, contest bases, harass supply lines

### The Carrier (15-18 BP Warpship)
```
PD=5-6, B=0-2, S=2, T=0, M=0, SR=2-3
```
- High SR for systemship transport
- Minimal combat capability
- Purpose: Deliver combat systemships to contested hexes

### The Fortress (15-20 BP Systemship)
```
PD=8-10, B=6-7, S=5-6, T=0, M=0
```
- No warp generator saves 5 BP
- All stats go to combat power
- **Cannot retreat** - must be picked up by carrier or fights to death
- Purpose: Base defense, overwhelming firepower in key battles

### The Missile Boat (8-12 BP Systemship or Warpship)
```
PD=3-4, B=0, S=0-1, T=2-3, M=9-12
```
- Multiple tubes for missile volleys
- Missiles have independent drive (PD + Tech Level max)
- Purpose: Alpha strike damage, missile drive can be set high to hit dodgers

---

## Combat Results Table (CRT) Analysis

### Drive Differential = Your Drive - Target Drive

The CRT is deterministic. Learn it.

### Attack Tactic
| Diff | vs Attack | vs Dodge | vs Retreat |
|------|-----------|----------|------------|
| <=-3 | Miss | Miss | Escapes |
| -2,-1 | Hit | Miss | Escapes |
| 0,+1 | **Hit+2** | Miss | Miss (pinned) |
| +2 | Hit+1 | Hit+1 | Miss |
| +3,+4 | Miss | Hit | **Hit** |
| >=+5 | Miss | Miss | Miss |

**Attack Insights**:
- Sweet spot is **0 to +2** differential vs attackers
- Need **+3/+4** to hit dodgers or retreating ships
- Over-investing in drive (+5) causes misses

### Dodge Tactic
| Diff | vs Attack | vs Dodge | vs Retreat |
|------|-----------|----------|------------|
| <=-4 | Miss | Miss | Escapes |
| -3,-2 | Miss | Hit | Escapes |
| -1,0 | Hit | Hit | Escapes |
| +1,+2 | Hit | Miss | Escapes |
| >=+3 | Miss | Miss | Escapes |

**Dodge Insights**:
- Defensive but still lands hits at close range (-2 to +2 vs attack)
- **Cannot stop retreating ships** - always escapes
- Good counter to aggressive attackers

### Retreat Tactic
| Diff | vs Attack | vs Dodge | vs Retreat |
|------|-----------|----------|------------|
| <=-2 | Miss | Miss | Escapes |
| -1,0 | Hit | Miss | Escapes |
| >=+1 | Miss | Miss | Escapes |

**Retreat Insights**:
- Can only hit attackers at -1/0 differential (weak counterattack)
- **Must escape ALL ships firing on you** to successfully retreat
- Systemships **cannot retreat** - warpships only

---

## Tactical Principles

### 1. The Drive Differential Game

Combat is won by predicting enemy drive allocation and countering:

- **If enemy attacks with high drive**: Dodge with moderate drive (they miss, you hit at close range)
- **If enemy attacks with low drive**: Attack with +2 drive advantage (Hit+1 or Hit+2)
- **If enemy dodges**: Attack with high drive (+3/+4 to hit)
- **If enemy retreats**: Attack with high drive (+3/+4 to prevent escape)

### 2. Focus Fire

Multiple ships should target the same enemy:
- Damage is cumulative after screen absorption
- Destroying one enemy ship removes its return fire
- VP isn't gained until bases are uncontested

### 3. The Stalemate Trap

Three consecutive combat rounds with no damage (after screens) = **initiative player must retreat**.

**Offensive implications**:
- If you move into a hex, you're the initiative player
- If combat stalemates, YOU retreat, not the defender
- Must commit enough force to guarantee damage

**Defensive implications**:
- High-screen ships can force stalemates
- Enemy wastes a turn retreating
- You maintain hex control

### 4. Retreat Denial

To prevent enemy retreat, you must hit with **every ship** firing on them:
- Coordinate attacks to ensure no ship misses
- High drive attackers (+3/+4) are needed vs retreating targets
- One miss = enemy escapes

### 5. Tech Level Advantage

A tech level 2 ship vs tech level 0:
- Beam damage: +2 hits
- Each missile: +2 hits
- Screen absorption: +2 hits

**Example**: Tech 2 ship with B=4 does 6 damage per hit. Tech 2 ship with S=3 absorbs 5 damage.

Later-built ships are significantly more powerful. Consider:
- Saving BP for tech 1+ construction
- Keeping early ships alive for PD/movement, building new ships for damage

---

## Movement Strategy

### Warpline Efficiency
- 1 PD to enter star hex + 1 PD to traverse warpline = 2 PD for long distance
- Compare: 6 hexes normal movement = 6 PD
- **Use warplines whenever possible**

### Must-Stop Rule
- Warpships must stop on star hexes containing enemies
- Cannot bypass defended positions
- Plan routes to avoid or engage as desired

### First Turn Restriction
- Cannot move onto enemy base stars on turn 1
- Use first turn for positioning and force concentration

### Spreading vs Concentrating

**Spread for VP**: Multiple ships on multiple bases = multiple VP per turn
**Concentrate for combat**: Overwhelming force in one hex = guaranteed victory

Balance: Enough force per hex to win, spread across enough hexes to score VP.

---

## Phase-by-Phase Decision Framework

### Build Phase

1. **Assess resources**: How many BP available?
2. **Check tech level**: Is it worth waiting for tech advancement?
3. **Repair/resupply check**: Ships at base that need it?
4. **Build priority**:
   - First: Enough combat power to contest objectives
   - Second: Mobility to reach objectives
   - Third: Specialization (missiles, carriers, etc.)

### Movement Phase

1. **VP opportunity**: Can I reach undefended enemy bases?
2. **Defensive needs**: Are my bases threatened?
3. **Force concentration**: Do I need to mass ships for a key battle?
4. **Retreat route**: If I attack and fail, where do I go?

### Combat Phase

1. **Assess force balance**: Who has advantage?
2. **Predict enemy tactics**: Based on their ship designs and situation
3. **Counter-tactic selection**:
   - Outmatched → Retreat (if warpship)
   - Even fight → Dodge for defense + opportunistic hits
   - Advantage → Attack with optimal drive differential
4. **Power allocation**:
   - Calculate target drive differential
   - Balance beam power vs screen vs drive
5. **Target priority**: Weakest enemy first (faster kills)

---

## Strategic Tempo

### Aggressive Posture
- Push early before enemy tech advantage develops
- Accept ship losses if it means VP
- Force multiple combats to stretch enemy forces

### Defensive Posture
- Build tech-superior ships later
- Use systemships for base defense (save 5 BP each)
- Make enemy attack into your strength
- Abuse stalemate rule to force their retreat

### Transition Points
- **Early game**: Establish map control, probe enemy strength
- **Mid game**: Tech disparity emerges, key battles for base control
- **Late game**: VP accumulation, endgame positioning

---

## Common Mistakes

1. **Stacking ships on one base**: 2 ships on 1 base = 1 VP. 2 ships on 2 bases = 2 VP.

2. **Over-investing in drive**: +5 or higher differential causes misses. +2 to +4 is the sweet spot.

3. **Ignoring tech levels**: A tech 2 ship beats a tech 0 ship of equal BP cost.

4. **Forgetting stalemate rule**: Attacker must retreat after 3 no-damage rounds.

5. **Systemships without carriers**: They can't retreat or move. Plan extraction.

6. **Neglecting screens**: Screen + tech level absorption is powerful. Unscreened ships die fast.

7. **Single-ship attacks**: One ship can be dodged. Multiple ships coordinating ensure hits.

---

## AI Strategic Priorities (For Implementation)

1. **VP Maximization**: Spread ships across undefended enemy bases
2. **Tech Awareness**: Value later-built ships higher in combat decisions
3. **CRT Optimization**: Calculate expected outcomes based on drive differential
4. **Retreat Judgment**: Know when to preserve ships vs fight to death
5. **Focus Fire**: Coordinate multiple ships on single targets
6. **Stalemate Awareness**: Track consecutive no-damage rounds
7. **Economic Efficiency**: Don't overbuild; BP saved can be used later at higher tech

---

## Advanced Tactical Concepts

### Retreat Denial Mathematics

To **prevent** an enemy from retreating, every ship firing at them must HIT.

**Attack tactic hit rates vs Retreat**:
| Drive Diff | Result |
|------------|--------|
| <=-3 | Escapes |
| -2,-1 | Escapes |
| 0,+1 | Miss (pinned but no hit) |
| +2 | Miss |
| +3,+4 | **HIT** |
| >=+5 | Miss |

**Key insight**: To deny retreat, you need +3 or +4 drive differential with Attack tactic.
- If enemy has drive 3, you need drive 6-7 (not 8+, that's +5 = miss)
- ALL ships must coordinate at +3/+4 differential

### The Screen + Tech Stacking

Screens become powerful at higher tech:
- Tech 0 with S=3 powered: absorbs 3 damage
- Tech 2 with S=3 powered: absorbs 5 damage
- Tech 3 with S=4 powered: absorbs 7 damage

**Implication**: Late-game ships with modest screens are surprisingly durable.

### Force Concentration Formula

How many ships to commit to a hex?
- Need to **deal more damage than enemy screens can absorb**
- Per ship: beam_power + tech_level = potential damage (on hit)
- Enemy defense: screen_power + tech_level = absorption

**Example**: Enemy has S=3, Tech 1 = absorbs 4 damage per round.
Your ships: B=4, Tech 0 = 4 damage each (on hit).
Need: At least 2 ships to guarantee net damage.

### Initiative and Stalemate Control

The **initiative player** (who moved into the hex) must retreat after 3 stalemates.

**Offensive**: If you can't guarantee damage, don't attack. Stalemates favor defender.

**Defensive**: High-screen builds can force stalemates. Enemy wastes turn retreating.

### Missile Alpha Strike

Missiles have unique properties:
- Independent drive (up to PD + Tech of firing ship)
- Deal 2 base damage + tech level
- Consumed on use

**Alpha strike strategy**:
1. Build missile boats with T=3, M=12, high PD
2. First combat round: fire all missiles at high drive (+4)
3. Overwhelm enemy screens with burst damage
4. Follow up with beam ships

**Counter**: High screen ships absorb missile volleys

### Systemship Economy

Systemships save 5 BP (no warp generator) but:
- Cannot move independently
- Cannot retreat in combat
- Must be ferried by carriers

**When to use**:
- Base defense (they don't need to move)
- Combat support (carrier delivers them)
- 20% more combat stats per BP spent

**Example**: 17 BP warpship vs 17 BP systemship
- Warpship: WG(5) + PD=5 + B=4 + S=3 = 17 BP
- Systemship: PD=7 + B=6 + S=4 = 17 BP (40% more combat power)
