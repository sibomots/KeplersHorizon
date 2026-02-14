# Machine Learning: AI Agent Feedback Loop & Latent Learning

This document captures a design discussion about closing the feedback loop
between command execution and AI agent decision-making, and a longer-term
vision for build-time machine learning driven by game diaries.

---

## 1. The Problem: Open-Loop Command Execution

The AI agent's MPC (model-predictive control) loop is currently open-loop
at the RENDER step. The cycle is:

    GATHER -> CALCULATE -> RENDER -> (assume success) -> GATHER

A human player closes this loop naturally -- they see "Invalid move" on
screen and adjust. The AI agent has no equivalent feedback channel.

When `invoke()` returns false on a command, the TaskRunner knows the
command failed and logs the error. But `notify_task_complete()` is a bare
signal -- it carries no payload. The agent wakes up from
`wait_for_task_completion()` knowing only "the task finished," not whether
it succeeded or failed, or why.

This causes blind retry loops: the agent re-issues the same failing
command because it never learns it failed. The game state hasn't changed,
so `CALCULATE` derives the same command from the same inputs.

## 2. The Gap Is Narrow

The information already exists at the right place. In `taskrunner.cpp`,
after executing an AI task:

```cpp
int result = internal_command_handler_body(item->ai_command, errmsg);
```

The TaskRunner has both `result` (success/failure) and `errmsg` (the
reason). It logs them, then calls `notify_task_complete()` which discards
both. The plumbing for the blocking handshake between TaskRunner and AI
agent thread already exists via condition variable -- it just signals a
bool when it could signal a result structure.

## 3. Proposed Feedback Mechanism

### Task Meta-Data

Each Task carries meta-data that is populated by the grammar rule
involved in the `invoke()` call. This is not just a bool -- it includes
contextual information about *why* the command failed:

- The command that was attempted
- Success or failure
- Structured failure reason (not a human-readable string)

### Notification With Payload

When `notify_task_complete()` fires, the meta-data blob is conveyed to
the AI agent. The agent only cares about failures -- successful commands
need no special handling since the world state changes and the next
GATHER reflects reality.

### Persistence in the Slate

Failed-command meta-data persists in the agent's awareness and becomes
part of the slate during the next GATHER step. The Lisp code can then
see: "my last MOVE w6 2223 failed because X" and choose differently.

## 4. Failure Reasons as Ontology Facts

Rather than carrying free-form error strings that Lisp would have to
parse, failure reasons should be closed under the ontology dictionary.
Every failure cause maps 1:1 to an ontology fact:

- "insufficient credits" -> `:precondition-violated :credits-below-threshold`
- "hex contested" -> `:precondition-violated :hex-in-combat`
- "not your turn" -> `:precondition-violated :phase-mismatch`

This means the Lisp code reasons about failures the same way it reasons
about everything else -- through the strategy plist, using the same
subsystem functions. Failures become just another structured input to
CALCULATE, not a special side channel.

The closed-set property guarantees the Lisp code can handle every failure
because the ontology is finite and known at compile time.

## 5. Failure Lifetime: When to Forget

### Time-Based Expiry Is Wrong

A fixed TTL (e.g., "forget after 3 rounds") is either too long -- the
agent avoids a now-valid command for rounds after the blocker cleared --
or too short -- the agent retries a still-invalid command because the
timer expired.

### State-Change Invalidation Is Ideal But Hard

The theoretically correct approach: a failure stays relevant until the
slate shows the precondition that caused it has changed. But this
requires mapping each failure reason to the specific state that caused
it, which is a complex, fragile mapping.

### Phase-Scoped Lifetime Is Pragmatic

Clear the failure list at each phase transition. Within a single phase,
game state only changes through the agent's own actions (and the other
player's during combat). A command that fails in BUILD phase round 5
won't succeed later in BUILD phase round 5 -- the preconditions are
stable within a phase.

Exception: combat, where state changes within a phase as orders resolve,
damage is assigned, and ships escape. Combat may need round-scoped or
stage-scoped clearing.

## 6. Latent Machine Learning: The Diary Concept

### Two Timescales

Separate the learning feedback loop into two timescales:

- **Runtime (within a game):** Static Lisp, fixed ontology, metrics
  memory. No self-modification. Deterministic and debuggable.

- **Build-time (between releases):** Ingest accumulated game diaries,
  produce updated Lisp parameters. The agent evolves, but only through
  a controlled, auditable build step.

### The Diary

Extend the existing `aa_metrics` persistence to capture richer
game-over summaries: what round the game ended, who won, fleet
composition at key moments, which strategies were attempted and when.
Structured data, not prose.

### Translation Approaches (Build-Time Ingestion)

Three approaches, ordered by complexity:

**Approach 1: Parameter Tables (Recommended Starting Point)**

The diary feeds a statistical pass that produces a config file --
thresholds, weights, timing windows. Example: "average earliest viable
missile build = round 5.2 across 47 games" becomes a `defparameter`
value. No new Lisp code is generated. The build step is data
aggregation. Lisp code has `defparameter` declarations that get values
from a generated config.

Simplest, safest, and likely captures 80% of the benefit.

**Approach 2: Rule Selection**

Write multiple strategy variants by hand -- aggressive early-build,
conservative turtle, missile-rush, etc. Diary analysis determines which
variants performed best against different opponent profiles. The build
step produces a selection table. No generated Lisp, just generated
configuration selecting among pre-written variants.

**Approach 3: Lisp Code Generation**

An external tool reads the diary, identifies patterns, and emits new
Lisp functions or modifies existing ones. Requires a validation step --
generated Lisp must type-check against the ontology, return well-formed
plists, and not introduce infinite loops. This is a research project
more than an engineering task.

### Build System Integration

For approaches 1 and 2: a pre-build script reads the diary database,
runs the analysis, writes a `.lisp` file of `defparameter` declarations
or a strategy-selection table, and the normal build includes that file.
No special tooling beyond a script that queries the metrics database and
emits s-expressions.

### On Self-Modifying Lisp

The power of the current architecture is that Lisp functions are pure:
slate in, decisions out. They carry no hidden state, don't mutate
themselves, don't evolve at runtime. That makes them debuggable,
predictable, and auditable. Self-modifying code at runtime sacrifices
all three properties. The diary/build-time approach preserves them by
restricting evolution to a controlled, versioned build step.

---

*Captured from design discussion, February 2026.*
