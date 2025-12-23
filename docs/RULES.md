# Kepler’s Horizon — Ruleset

This document defines the complete rules of Kepler’s Horizon.

These rules are **canonical**.  
All implementations must conform to this document.

---

## 1. Overview

Kepler’s Horizon is a turn-based, multiplayer strategy game played on an
abstract representation of space. Players control fleets and attempt to
achieve dominance through lawful movement and positional control.

There is:
- no randomness,
- no hidden information,
- and no tactical combat resolution.

All outcomes follow directly from player decisions and the game rules.

---

## 2. Game Components

### 2.1 Players
- Two or more players.
- All players have identical capabilities.

### 2.2 Space Map
- The map consists of a finite set of **regions**.
- Regions are connected by **transit links**.
- Transit links define which regions may be traversed.

The map is static for the duration of a game.

### 2.3 Fleets
- A fleet is a unit owned by a player.
- Fleets occupy exactly one region at any given time.
- Fleets are indivisible and identical in capability.

---

## 3. Initial Setup

1. The map is initialized.
2. Each player is assigned one or more **home regions**.
3. Each player begins with a predefined number of fleets placed in their home regions.
4. Turn counter is set to Turn 1.

---

## 4. Turn Structure

Each turn consists of four phases:

1. **Planning Phase**
2. **Movement Phase**
3. **Resolution Phase**
4. **End Phase**

All players act simultaneously unless otherwise specified.

---

## 5. Planning Phase

During the Planning Phase, each player issues orders for their fleets.

### 5.1 Orders
For each fleet, a player may issue exactly one of the following:

- **Move Order**: Attempt to move to an adjacent region.
- **Hold Order**: Remain in the current region.

Orders are submitted simultaneously and revealed together.

---

## 6. Movement Phase

During the Movement Phase, all move orders are evaluated.

### 6.1 Valid Movement
A move order is valid if:
- A transit link exists between the origin and destination regions.

Invalid move orders automatically become hold orders.

### 6.2 Simultaneity
All valid moves are considered simultaneous.  
No move benefits from priority or initiative.

---

## 7. Resolution Phase

After movement, region control is resolved.

### 7.1 Region Occupancy
A region may contain fleets from zero or more players.

### 7.2 Conflict Resolution
- If fleets from only one player occupy a region, that player controls the region.
- If fleets from multiple players occupy a region, a **conflict** occurs.

### 7.3 Conflict Outcome
Conflicts are resolved deterministically:

- All fleets involved in the conflict are removed from the game.
- The region becomes unoccupied.

There are no partial victories.

---

## 8. End Phase

During the End Phase:

- Eliminated fleets are removed.
- Victory conditions are checked.
- The turn counter increments.

---

## 9. Victory Conditions

A game ends when one of the following conditions is met:

### 9.1 Dominance Victory
A player controls more than a predefined fraction of regions.

### 9.2 Elimination Victory
All opposing players have no remaining fleets.

### 9.3 Turn Limit
If a predefined maximum number of turns is reached:
- Victory is awarded to the player controlling the most regions.
- Ties result in a draw.

---

## 10. Constraints and Invariants

The following are always true:

- No randomness is introduced at any stage.
- All information is public.
- Rules do not change mid-game.
- Equal inputs yield equal outcomes.

---

## 11. Extensions (Non-Canonical)

The following are explicitly *not* part of the base rules, but may be layered on:

- Variable transit costs
- Delayed movement
- Asymmetric starting positions
- Non-lethal conflict resolution
- Dynamic map topology

Such extensions must be documented separately and must not contradict this ruleset.

---

## 12. Design Intent

These rules are intentionally minimal.

The strategic depth of Kepler’s Horizon emerges from:
- constrained movement,
- simultaneous decision-making,
- and irreversible consequences.

The rules favor clarity over complexity.

