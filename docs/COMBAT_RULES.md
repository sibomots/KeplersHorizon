# Combat Rules (Borealis Tactical)

## 1. Overview
Combat occurs only at star hexes and is resolved in discrete combat rounds. Each contested star hex is resolved independently.
Combat is deterministic except for player choices; there is no randomness.

## 2. When Combat Occurs
- Combat must occur when, at the end of a player’s movement, ships belonging to both players occupy the same star hex.
- Each star hex with opposing ships is treated as a separate combat.
- The active player chooses the order in which multiple star-hex combats are resolved.
- One star hex’s combat is fully resolved before the next begins.

## 3. Combat Round Structure
Combat proceeds in rounds. One combat round consists of:
1. **Order Writing** (Simultaneous)
2. **Order Reveal**
3. **Weapon Resolution via CRT**
4. **Damage Application**
5. **Retreat Resolution**
6. Repeat if necessary

Combat continues until one of the termination conditions is met.

## 4. Orders (Written Simultaneously)
Each ship at the contested star hex writes a concealed order. An order contains:

### (a) Combat Tactic
One of:
- **Attack**
- **Dodge**
- **Retreat** (Systemships may not select Retreat)

### (b) Beam Fire Target (if any)
### (c) Missile Fire
- Target for each missile
- Drive setting for each missile

### (d) Power Allocation
Power is allocated from **PD** (Power/Drives) to:
- **Drive (D)**
- **Beams (B)**
- **Screens (S)**
- **Tubes (T)**

**Constraints:**
- Allocated power may not exceed remaining PD.
- No attribute may be powered beyond its undamaged capacity.
- Warp generator does not require power.

### (e) Systemship Pickup/Drop (Warpships only)
Subject to special rules.

## 5. Combat Results Table (CRT)
**Drive Difference** = `Firing Drive Allocation` − `Target Drive Allocation`
*(For missiles: Drive is the missile's own drive setting)*

### CRT Table
**Columns** = Target Ship’s Tactic (Attack, Dodge, Retreat)
**Rows** = Firing Ship’s Tactic + Drive Difference

#### Firing Tactic: ATTACK
| Drive Difference | Attack | Dodge | Retreat |
| :--- | :--- | :--- | :--- |
| −3 or less | Miss | Miss | Escapes |
| −1 or −2 | Hit | Miss | Escapes |
| 0 or +1 | Hit +2 | Miss | Miss |
| +2 | Hit +1 | Hit +1 | Miss |
| +3 or +4 | Miss | Hit | Hit |
| +5 or more | Miss | Miss | Miss |

#### Firing Tactic: DODGE
| Drive Difference | Attack | Dodge | Retreat |
| :--- | :--- | :--- | :--- |
| −4 or less | Miss | Miss | Escapes |
| −2 or −3 | Miss | Hit | Escapes |
| 0 or −1 | Hit | Hit | Escapes |
| +1 or +2 | Hit | Miss | Escapes |
| +3 or more | Miss | Miss | Escapes |

#### Firing Tactic: RETREAT
| Drive Difference | Attack | Dodge | Retreat |
| :--- | :--- | :--- | :--- |
| −2 or less | Miss | Miss | Escapes |
| −1 or 0 | Hit | Miss | Escapes |
| +1 or more | Miss | Miss | Escapes |

### Meaning of Results
- **Miss**: Weapon does no damage.
- **Hit**: Damage = `Base` + `Tech Level`.
- **Hit +1 / +2**: Add +1 or +2 to base damage.
- **Escapes**: Target successfully retreats. Must escape against **every** enemy firing at it.

## 6. Weapon Damage
- **Beam**: Damage = `Beam Power` + `Tech Level` + `CRT Modifier`.
- **Missile**: Damage = `2 (Base)` + `Tech Level` + `CRT Modifier`.
  - Missiles always use **Attack** tactic.
  - Missiles act as one-round ships and are removed after firing.

## 7. Damage Absorption (Screens)
Total hits from all weapons are summed.
**Absorption** = `Screen Power` + `Tech Level` (if powered).
Remaining hits are **Effective Hits**.

## 8. Damage Assignment
Effective hits are applied by the owning player, who chooses which attributes take damage.
- **1 hit = 1 BP cost to repair** (reduces attribute capacity).
- **Missiles**: If ship has ≥3 missiles, 1 hit removes 3 missiles. If 1-2 remain, they absorb 1 hit.
- **Warp Gen**: Never takes hits.
- **Destruction**: When all other attributes = 0, ship explodes.

## 9. Retreat Resolution
Ships that successfully escaped are moved to any adjacent hex.

## 10. Termination
Combat ends when:
1. All ships of one player are destroyed.
2. All ships of one player retreat.
3. Three consecutive rounds occur with no unabsorbed damage (Attacker must withdraw).
