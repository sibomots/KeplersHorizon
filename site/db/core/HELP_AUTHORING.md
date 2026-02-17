# Help Topics Authoring Guide

This document provides guidelines for adding or editing help topics in Kepler's Horizon.

## File Locations
- `help_topics.csv` - Contains the actual help content (topic_id, topic_info)
- `help_lookup.csv` - Maps keywords to topic IDs (topic_keyword, help_topic_id)

## Topic Format
All help topics should follow this mini man-page format with NO BLANK LINES between sections:

```
TOPIC: [TOPIC_NAME]
SYNOPSIS:
  command [args...]    Brief description
  command [args...]    Brief description
DESCRIPTION:
  Multi-line description of what this command/feature does.
  Include important details and constraints.
EXAMPLES:
  > example command 1
  > example command 2
SEE ALSO: related, topics, here
```

## Rules
1. **No blank lines** between TOPIC/SYNOPSIS/DESCRIPTION/EXAMPLES/SEE ALSO sections
2. **Multi-line sections** are allowed within each section
3. **Indentation** - Use 2 spaces for content under each section header
4. **SEE ALSO** - Reference other topics by their lookup keywords
5. **Escape quotes** - Double-quote the entire entry, escape internal quotes

## Example CSV Entry
```csv
1,"TOPIC: BUILD
SYNOPSIS:
  BN W/S {name}    Build new Warpship or Systemship
  BS {ship} {specs}    Set ship specifications
  BC    Commit draft to fleet
  BX    Cancel current draft
  BD    Show pending drafts
DESCRIPTION:
  Create and configure new ships during the Build Ships phase.
  Specifications: PD=# P=# S=# L=# T=# H=#
  Cost is determined by total attribute points assigned.
EXAMPLES:
  > bn w Falcon
  > bs W1 pd=9 p=1
  > bc
SEE ALSO: fleet, deploy, specs"
```

## Multi-Word Resource Names
Resource names in the database use underscores (e.g., `RARE_EARTH`).
User input with spaces is automatically normalized: `rare earth` → `RARE_EARTH`
**Module design rule**: All multi-word item names in CSVs must use underscores.
