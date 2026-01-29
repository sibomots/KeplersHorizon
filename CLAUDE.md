# Engineering Rules & Style Guide
## Architecture & Logic
- Single Return Policy: Bias towards a single return point. Avoid early returns.
- All code is re-formatted with "clang-format -i FILE" before depositing in the repo.
- Explicit Bracing: Brace everything. if (p) q(); is forbidden. Use the braced format with appropriate newlines.
- No Namespaces: Do not use namespaces. Use std:: prefix for STL. Never "using namespace std;".
- Include Guards: No #pragma once. Use #ifndef HEADER_H guards.
- Forward Decls: Avoid. Include the header unless a circular dependency is unavoidable.
- Data Flow: Return bool or int for status/result. Pass data via references. No returning fat containers (e.g. std::map).
- Singletons: Use Meyers Singletons with a dedicated compilation unit (.cpp) so the linker doesn't go nuts.
- Enums: Use enums for flags/states. Never use strings as logic flags.
## Naming & Casing
- Style: Use pszHungarian or CamelCase.
- Banned: No snake_case. No single-letter variables (except i, j, k for loop counters).
- Const: Use it reasonably; don't be a jerk.
## Hygiene
- Line Endings: LF only. If a ^M (CRLF) is detected, stop immediately and report file corruption. Do not proceed until resolved.
- Technical Debt: If you spot duplicated logic, screw-ups, or technical debt, mark it with a // BUGBUG comment. Don't stop to fix it unless it breaks the current task; just flag it for rainy days when the token-quota is saturated.
## Interaction Protocol
- Tone: Terse. Engineering-focused.
- No Fluff: Do not tell me why my idea is good. Do not apologize.
- Authority: If you are certain, execute. If you are stuck, ask.
- Pushback: If an instruction is technically disastrous, call me out directly. No shine.
- Proactive Follow-up: Once you've saved that, run cat CLAUDE.md in your TTY 1 to make sure no hidden ^M snuck in during the paste, then tell Claude to get to work.


