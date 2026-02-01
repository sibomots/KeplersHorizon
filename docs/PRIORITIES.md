# Priorities
(last curated Sun Feb  1 02:38:05 UTC 2026)

1.  Claude is tasked to add 5 Priorities to this list that it deems worthy to consider.

2.  The AI-Agent is able to use Missiles properly in combat.  Tubes and Missiles.

3.  The AI-Agent is able to demonstrate use of the Systemship Rack to place ships, pick up ships, and choose to do so in order to satisfy governing dynamics of the game goal defined in Lisp.

4.  The commands that deal with the Milieu extras.  resources, survey, politics, economy, fabrication, resupply, repair are implemented richly and interestingly.

5.  The effect (environmental) on combat and movement due to rare, but possible conditions is demonstrable. (This may involve a special command to put the game in debug mode to prefer or set probabibilities of this effect, eg: gamdev environment 99 would mean if set, by a player the effect of environment on movement, or combat is 99% sure to happen).  Propose other "gamedev" parameters that would be needed to hook up rapid testing of these rare occurances.

---

## Claude's Proposed Priorities (Added Feb 1 2026)

Based on codebase analysis against the game rules, the following represent gaps between current implementation and code-complete status:

### 6. War of Attrition Victory Condition (Errata Rule #2)

The game rules specify: "If neither player can achieve 3 VP after 20 turns, the player who has destroyed the most enemy ships wins." This alternate victory condition is not implemented.

**Required work:**
- Add destroyed-ship counter per player to GameState
- Increment counter when ships are destroyed (combat resolution)
- Check at turn 20+ if primary VP condition is unachievable
- Declare winner based on kill count
- Add pre-game setting toggle for this rule overlay

**Complexity:** Low-medium. Mostly state tracking and victory condition logic.

---

### 7. AI Leverages Server Pathfinding for Movement

The server's MapUtil already implements optimal route finding that leverages warplines. The AI movement logic (`aa-movement.lisp`) should trust and use this pathfinding rather than computing routes itself.

**Movement interception rule:** If a ship's computed path passes through a star hex occupied by enemy ships, the ship STOPS at that star hex. Movement ends there. Combat will occur after the movement phase concludes. The player (or AI) can continue moving other ships during the movement phase, but the intercepted ship is committed to that hex.

**Required work:**
- Ensure AI movement commands pass destination only, letting server compute optimal path
- AI should be aware that paths through enemy-occupied stars trigger interception
- Consider whether interception is acceptable or if alternate routes exist
- AI should consider movement cost when deciding destination priority

**Complexity:** Low. Server already does the heavy lifting. AI integration only.

---

### 8. AI Resupply/Repair Decision Logic

The AI build phase has stub repair logic (`should-repair-p` checks if PD < 5) but does not:
- Track original ship stats to know actual damage
- Issue missile resupply commands (ships can run out of missiles)
- Prioritize repair vs new builds strategically
- Return damaged ships to base for repair

**Required work:**
- Pass original ship stats through slate for damage comparison
- Add `ship-original-pd`, `ship-original-missile` accessors
- Implement `should-resupply-p` for missile-armed ships
- Strategic decision: repair veteran high-tech ship vs build new low-tech ship

**Complexity:** Medium. Requires slate enrichment and strategic heuristics.

---

### 9. Multi-Theater Combat Triage

When AI ships are engaged in combat at multiple star hexes simultaneously (e.g., W2 at Koral and W5 at Sydra), the AI currently issues orders for each hex in isolation. A smarter AI would assess all active theaters together before issuing orders for any of them.

**Example situation:**
- W2 (Tech 0, PD=4) fights 2 enemy ships at Koral (non-VP hex)
- W5 (Tech 2, PD=6) fights 1 enemy ship at Sydra (enemy base, VP hex)

**Current behavior:** AI fights both, likely loses W2.

**Desired behavior:** AI recognizes:
- W2 is outmatched and lower value (Tech 0)
- Koral isn't a VP hex - losing there costs nothing strategic
- W5 has advantage at the VP hex that matters
- Decision: W2 retreats (Escape tactic) to preserve the hull for later reinforcement; W5 presses attack

**Required work:**
- Before issuing any combat order, enumerate all active combats
- Score each theater: (force ratio) x (hex VP value) x (ship tech value)
- For losing theaters at non-critical hexes, prefer Escape tactic
- For winning theaters at critical hexes, press Attack
- High-tech ships are worth preserving over low-tech ships

**Complexity:** Medium. Analysis before action, no new game mechanics. The tactical options (Attack/Dodge/Escape) already exist - this is about choosing wisely across theaters.

---

### 10. Stub Command Implementation (CRT, HEX, GALAXY)

Per `Next-Steps.md`, these commands have tokens but only log:
- `crt` - Display Combat Results Table reference
- `hex <loc>` - Show hex information (ships, facilities, resources)
- `galaxy` - Show strategic map overview

These are player-facing information commands that would complete the UI.

**Required work:**
- `crt`: Format and return the CRT as structured JSON for UI display
- `hex`: Query ships, facilities, resources, anomalies at location
- `galaxy`: Aggregate system status (controlled bases, fleet positions, VP)

**Complexity:** Low. Query and format operations, no game state mutation.

---

## Priority Ranking Rationale

The existing priorities 2-5 focus on AI capability and Milieu enrichment. The proposed additions fill:

- **Rule compliance gap** (#6 War of Attrition)
- **AI strategic depth** (#7 Pathfinding, #8 Repair/Resupply, #9 Theater triage)
- **Player experience** (#10 Info commands)

Suggested execution order after existing 2-5:
1. #7 (Low complexity, server pathfinding integration)
2. #9 (Smarter AI combat decisions across theaters)
3. #8 (AI repair/resupply decisions)
4. #10 (Info commands for UX)
5. #6 (War of Attrition rule compliance)
