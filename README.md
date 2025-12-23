# Kepler’s Horizon

[Kepler's Horizon Logo](docs/img/keplerhorizon-logo-crop.png)


**Kepler’s Horizon** is a turn-based, abstract strategy game set in deep space.  
Players compete to control regions of space by moving fleets according to
strict, law-governed constraints rather than tactical combat mechanics.

The game is inspired by classic space strategy systems, most notably *WarpWar*
(1977), but is designed from the ground up to be cleaner, more extensible,
and more explicit about its underlying rules.

At its core, Kepler’s Horizon is a game about **motion, limits, and structure**:
what can be reached, when it can be reached, and how constraints shape power.

---

## Project Status

This repository is under active development.

- Rules are defined first and treated as canonical.
- Code is an implementation of the rules, not the other way around.
- Mechanics may be extended, but the foundational model is intentionally simple.

Expect iteration.

---

## Core Concepts

- **Abstract Space**  
  Space is represented as a discrete set of regions (nodes) connected by
  allowed transit relationships.

- **Law-Governed Movement**  
  Fleets move according to predefined constraints. There is no free movement
  and no hidden information.

- **No Tactical Combat**  
  Conflict is resolved deterministically by presence and timing, not dice,
  hit points, or micro-scale battles.

- **Strategic Time**  
  The game proceeds in discrete turns with simultaneous planning and
  deterministic resolution.

## Running the Game

Exact commands depend on the current implementation.  
At a minimum, the following modes are expected to exist or be stubbed:

- Start a new game
- Load a saved game
- Execute a single turn
- Run in headless / simulation mode

Refer to implementation-specific documentation under `docs/` as it evolves.

---

## Philosophy

Kepler’s Horizon deliberately avoids feature creep at the rules level.

If a mechanic cannot be:
- stated clearly,
- resolved deterministically,
- and reasoned about analytically,

it does not belong in the core game.

Extensions are expected to build *on top of* the base rules, not replace them.


---

## License

Licensed under BSD 3-Clause.  See LICENSE.

---

Copyright 2025 sibomots. All rights reserved

