# Kepler's Horizon - Player Strategy Reference

## Victory

3 VP wins. VP awarded at start of your turn for each enemy base-star you occupy uncontested.

## Economics

| Turn | BP |
|------|----|
| 1 | 20 |
| 2+ | 10 |

BP may accumulate. Warp generator costs 5 BP. Systemships omit WG, gain 5 BP for combat stats.

## Tech Levels

| Turns | Tech |
|-------|------|
| 1-4 | 0 |
| 5-8 | 1 |
| 9-12 | 2 |

Tech adds to beam damage, missile damage, screen absorption.

## CRT Quick Reference

Drive differential = your_drive - target_drive.

**Attack:**
- +0/+1 vs Attack: Hit+2 (best)
- +2 vs Attack: Hit+1
- +3/+4 vs Dodge/Retreat: Hit
- +5 or higher: Miss

**Dodge:**
- -2 to +2 vs Attack: Hit
- Cannot stop retreats

**Retreat:**
- Escapes unless ALL attackers hit at +3/+4

## Core Principles

**VP Spreading:** N ships on N bases = N VP. N ships on 1 base = 1 VP.

**Drive Cap:** Never allocate drive that exceeds target by +5. Cap at +4.

**Focus Fire:** All ships target same enemy. Weakest first (lowest total stats).

**Force Calculation:**
```
your_damage = sum(beam + tech) per ship
enemy_absorb = sum(screen + tech) per enemy
```
Attack only if your_damage > enemy_absorb.

**Stalemate:** 3 rounds with no net damage forces initiative player (attacker) to retreat.

**Tech Timing:** Save BP 1-2 turns before tech increase. Tech 1+ ships outclass Tech 0.

## Ship Builds

| Role | Stats | BP | Notes |
|------|-------|-----|-------|
| Brawler | PD=6 B=4 S=3 | 17W | Main combat |
| Interceptor | PD=5 B=2 S=2 | 14W | Fast, base contest |
| Fortress | PD=8 B=6 S=5 | 19S | Base defense, no retreat |
| Defender | PD=6 B=4 S=3 | 13S | Light base defense |
| Missile Boat | PD=4 T=2 M=6 | 15W | Alpha strike |

W = warpship (includes 5 BP WG). S = systemship.

## Phase Decisions

**Build:**
1. Repair damaged ships at base
2. Build if credits >= ship_cost + 10 reserve
3. Systemship defender if bases threatened
4. Wait for tech if 1-2 turns from increase

**Movement:**
1. Turn 1: Cannot enter enemy bases
2. Spread ships to different undefended enemy bases
3. Defend home if enemy within 3 hexes of base

**Combat:**
1. Check force balance before engaging
2. Read enemy's prior order if available
3. Counter: Attack->Dodge, Dodge->Attack+3, Retreat->Attack+4
4. Allocate drive to achieve target differential
5. Preserve PD when assigning damage (enables retreat)

## Counter-Play

| Enemy Did | You Do | Drive Target |
|-----------|--------|--------------|
| Attack | Dodge | moderate |
| Dodge | Attack | +3/+4 |
| Retreat | Attack | +3/+4 |
| Unknown | Attack | +2 |

## Damage Priority

When assigning damage, sacrifice in order: Tubes > Beams > Screens > PD.

PD enables retreat. Screens + tech absorb future damage.

## Retreat

Retreat toward own bases. Systemships cannot retreat.

All enemy ships must miss for escape. One hit = pinned.
