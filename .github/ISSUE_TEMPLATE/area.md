---
name: MUD Area Specification (CircleMUD) about: Define a new Zone/Area specifically for CircleMUD/tbaMUD architecture. title: '[Zone]: <Name of Zone> (<VNUM Start>-<VNUM End>)' labels: area-design, status: triage, engine: tbamud assignees: head-builder
---
​CircleMUD Zone Design Document
​Zone Name
The display name of the zone (as seen in zone command).
​
​VNUM Assignment (Bottom - Top)
The specific block of Virtual Numbers assigned to this zone (e.g., 14000 - 14199).
​
​Reset Mode
Specify the reset behavior. Keep only the intended option:
​Mode 0: Never reset (Static/Storage)
​Mode 1: Reset when no players are in zone (Standard)
​Mode 2: Reset always (Time-based/Aggressive)
​Target Player Level
Intended level range for the area:
​
​Theme & Atmosphere
Describe the setting, weather (Sector Types), and mood.
Example: Theme: Verticality/Wind. Climate: COLD/DRY. Primary Sectors: MOUNTAIN (6), AIR (12).
​
​Key Mechanics (Triggers & Spec_Procs)
List required DG Scripts or hardcoded procedures.
Example: Movement Logic (falling triggers), Aggro behaviors, Puzzle object interactions.
​
​Content Manifest
Rough count of entities needed to populate the.zon file.
Example: Rooms: ~120, Mobs: 5 types, Objects: 1 Key item.
​
​Acceptance Criteria (Functional)
Conditions for the area to be considered testable.
​[ ] Zone compiles/loads without VNUM collisions.
​[ ] All room exits link reciprocally (North <-> South) unless one-way is intended.
​[ ] Zone Reset (Lifespan) triggers correctly in logs.
​[ ] No "Death Traps" (DT) without at least one warning room.
​Definition of Done (Quality Gate)
CircleMUD building standards check.
​[ ] Descriptions are spell-checked and use proper line breaks/color codes.
​[ ] All Mobs have valid flags (e.g., IS_NPC, SENTINEL/SCAVENGER).
​[ ] Extra Descriptions (EXDESC) added for key keywords in rooms.
​[ ] Gold/Exp rewards balanced against MUD economy standards.
​[ ] Zone file (.zon) commands end strictly with S.
