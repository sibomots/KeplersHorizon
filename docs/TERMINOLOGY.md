# Kepler’s Horizon — Terminology

This document defines the canonical terminology used throughout the
Kepler’s Horizon project.

All rules, documentation, code, comments, and discussion should use these
terms consistently. When ambiguity arises, this document takes precedence.

---

## 1. Core Concepts

### Space
The abstract domain in which the game is played.  
Space is not continuous and does not imply physical realism.

Space is represented solely by the map structure.

---

### Map
The complete set of regions and their transit relationships.

The map is static for the duration of a game unless an explicit extension
states otherwise.

---

### Region
A discrete location within the map.

- Regions are indivisible.
- Regions may be occupied by zero or more fleets.
- Regions may be controlled by at most one player after resolution.

Synonyms to avoid: *sector*, *system*, *hex*, *node* (unless explicitly
introduced by an extension).

---

### Transit Link
A defined connection between two regions indicating that movement between
those regions is permitted.

- Transit links are binary (either present or not).
- Transit links are not directional unless explicitly stated.
- Transit links do not imply distance, cost, or time beyond a single turn.

Synonyms to avoid: *lane*, *path*, *edge* (except in internal graph theory
discussion).

---

## 2. Player-Related Terms

### Player
An entity that issues orders and controls fleets.

All players:
- operate under identical rules,
- have equal access to information,
- and act simultaneously.

---

### Home Region
A region assigned to a player during initial setup.

Home regions have no inherent properties beyond their role in setup unless
explicitly defined by an extension.

---

## 3. Fleet Terminology

### Fleet
The fundamental unit controlled by a player.

- Fleets are indivisible.
- Fleets are identical in capability.
- Fleets occupy exactly one region at any time.
- Fleets do not possess attributes such as strength, health, or experience.

Synonyms to avoid: *ship*, *unit*, *army*, *token* (except in UI contexts).

---

### Fleet Ownership
The association between a fleet and the player who controls it.

Ownership does not change during a game unless explicitly defined by an
extension.

---

## 4. Orders and Actions

### Order
An instruction issued by a player for a single fleet during the Planning Phase.

Orders are binding for the duration of the turn.

---

### Move Order
An order directing a fleet to attempt movement to an adjacent region.

A move order:
- specifies exactly one destination region,
- may succeed or fail based on map constraints.

---

### Hold Order
An order directing a fleet to remain in its current region.

Hold orders always succeed.

---

### Invalid Order
An order that violates the rules (e.g., movement to a non-adjacent region).

Invalid orders are automatically converted into hold orders.

---

## 5. Temporal Structure

### Turn
The primary unit of time in the game.

A turn consists of:
1. Planning Phase
2. Movement Phase
3. Resolution Phase
4. End Phase

---

### Planning Phase
The phase in which players issue orders for their fleets.

All planning is simultaneous and private until orders are revealed.

---

### Movement Phase
The phase in which valid move orders are applied simultaneously.

No resolution of conflicts occurs during this phase.

---

### Resolution Phase
The phase in which region occupancy and conflicts are resolved.

All conflict outcomes are deterministic.

---

### End Phase
The phase in which:
- fleets are removed,
- victory conditions are evaluated,
- and the turn counter advances.

---

## 6. Control and Conflict

### Occupancy
The presence of one or more fleets in a region.

Occupancy alone does not imply control.

---

### Control
The state in which a region is exclusively occupied by fleets belonging to
a single player after resolution.

Control has no effect unless referenced by victory conditions or extensions.

---

### Conflict
A condition in which fleets belonging to more than one player occupy the
same region after movement.

---

### Conflict Resolution
The deterministic process by which conflicts are resolved.

In the base ruleset:
- all fleets involved in a conflict are removed,
- the region becomes unoccupied.

There are no partial outcomes.

---

## 7. Game Outcomes

### Victory Condition
A formally defined condition that ends the game and determines the winner.

---

### Dominance Victory
A victory achieved by controlling more than a specified fraction of regions.

---

### Elimination Victory
A victory achieved when all opposing players have no remaining fleets.

---

### Draw
An outcome in which no player satisfies a victory condition within the
allowed turn limit.

---

## 8. Constraints and Properties

### Deterministic
A property indicating that identical game states and orders always produce
identical outcomes.

---

### Perfect Information
A property indicating that all players have access to the same information
at all times.

---

### Canonical
A designation indicating that a rule, term, or document is authoritative.

Canonical documents override implementation details.

---

## 9. Extensions

### Extension
An optional ruleset layered on top of the canonical rules.

Extensions may:
- add mechanics,
- refine resolution,
- or alter victory conditions.

Extensions must not:
- contradict canonical definitions,
- redefine existing terms without explicit renaming.

---

## 10. Terminology Stability

Once a term is defined here, it should not be redefined casually.

If a new concept cannot be expressed using existing terms, it should be:
1. Explicitly introduced,
2. Added to this document,
3. Referenced consistently thereafter.

Terminology drift is considered a design error.

---


