# Engineering Rules & Style Guide

If you don't understand these rules, stop now and ask me the question.

## CLAUDE CODE RULES

-  When editing files, apply changes directly and do not show diffs or verbose summaries.


## Git Rules

- never make a branch unless you ask first.
- never commit to a branch unless you ask first.
- never push, period
- never commit with messages that are longer than 30 characters
- never squash commits
- if you need to commit a change, make lots of small commits that are contextually aligned rather than big (few) commits
- don't mix cosmetic commits with functionally change commits
- never merge a branch yourself.
- branch names are `dev/issue-slug`      Be smart and make the branch names short as possible and if you must compound words, use dash (-).  I cannot see a reason for more than 3 compounded words in a branch name.
- git flow:  `main` is the branch that is running in a test-mode.   ALL work is done on branches and then merged into `main` after that proposal has been vetted.
- there is not an intermediate release-candidate branch before merge to main, but there ought to be. I'll think about it.

## coding style

- Do NOT put comments on the same line as code.

Disallowed:

```
for(int i = 0; i < N; i++) {
   do_something(); // whatever
}
```

Allowed:

```
for(int i = 0; i < N; i++)
{
    // do the thing
    do_something();
}
```


- Avoid in-predicate function calls

Example:

```
// Disallowed
if ( int c = handle_the_thing(arg) ) {
   // do the thing
   do_thing();
}

// Even worse:
if ( handle_other_thing(arg)) {
    // whatever
    whatever();
}

// Acceptable:
int c = handle_the_thing(arg);
while(c)
{
    do_thing();
    c = handle_the_thing(arg);
}
```

- I never want to see functions return std::string unless absolutely necessary (and then ask me first).  If you must "return a string" do so via pass-by-reference.

- new classes, unless they are Singleton, and in some other cases that follow Design Patterns, should consider if they need the basic c'tor, d'tor, copy assignment, etc.. I'm not saying all classes do, but classes that are really public structs that are
used in algorithms (generics) ought to make sure they are properly equipped with the c'tor, d'tor, copy-assignment, move-assignment etc.. methods.

- use of `auto` is to be very special.   I hate seeing it, but I know it's a nice
way to avoid complicated declarations.  Still, if you know the type of what it is, 
then use it (or at least comment it) so later on the software can be refactored (which it will be).   But excessive use of `auto` is a blight, frankly.

- std::string is always initialized to empty so DO NOT initialze them with this pedantic  `std::string foo = "";`  nonsense.   In class constructors, there is no point
in initializing STL strings like this. They are already initialized.  Further
if the STL container is designed to **already be initialized** then don't bother
re-re-initializing it, please.  Of course when we do need to, we should.

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


