name: "MUD Area Specification (CircleMUD)"
description: "Define a new Zone/Area specifically for CircleMUD/tbaMUD architecture."
title: "[Zone]: <Name of Zone> (<VNUM Start>-<VNUM End>)"
labels: ["area-design", "status: triage", "engine: tbamud"]
assignees: ["head-builder"]
projects: ["org/world-roadmap"]
body:
 * type: markdown
   attributes:
   value: |
   ## CircleMUD Zone Design Document
   This form defines the technical and narrative scope for a new .zon file and its associated .wld, .mob, and .obj files.
   Refer to the(https://wiki.yourmud.com/building) for VNUM policies.
 * type: input
   id: zone_name
   attributes:
   label: Zone Name
   description: The display name of the zone (as seen in zone command).
   placeholder: e.g., "The Isles of No"
   validations:
   required: true
 * type: input
   id: vnum_range
   attributes:
   label: VNUM Assignment (Bottom - Top)
   description: The specific block of Virtual Numbers assigned to this zone.
   placeholder: e.g., 14000 - 14199
   validations:
   required: true
 * type: dropdown
   id: reset_mode
   attributes:
   label: Reset Mode
   description: How the zone behaves regarding repopulation.
   options:
   - "Mode 0: Never reset (Static/Storage)"
   - "Mode 1: Reset when no players are in zone (Standard)"
   - "Mode 2: Reset always (Time-based/Aggressive)"
   validations:
   required: true
 * type: dropdown
   id: level_range
   attributes:
   label: Target Player Level
   options:
   - "Low Level (1 - 15)"
   - "Mid Level (16 - 50)"
   - "High Level (51 - 90)"
   - "Hero/Legend (91 - 100+)"
   - "Utility/Non-Combat"
   validations:
   required: true
 * type: textarea
   id: theme_concept
   attributes:
   label: Theme & Atmosphere
   description: Describe the setting, weather (Sector Types), and mood.
   placeholder: |
   - Theme: Verticality, Wind, Isolation.
   - Climate: COLD/DRY.
   - Primary Sectors: MOUNTAIN (6), AIR (12), CLIMBING (11).
   validations:
   required: true
 * type: textarea
   id: mechanics
   attributes:
   label: Key Mechanics (Triggers & Spec_Procs)
   description: List required scripts (DG Scripts) or special procedure hardcode.
   value: |
   - [ ] Movement Logic: Players falling from 'Climbing' rooms if Move Points < 10.
   - [ ] Aggro: Mobs push players to previous rooms on hit.
   - [ ] Puzzle: Object VNUM X is required to open Door VNUM Y.
 * type: textarea
   id: content_manifest
   attributes:
   label: Content Manifest
   description: Rough count of entities needed to populate the .zon file.
   placeholder: |
   - Rooms: ~120 (Mostly vertical connections)
   - Mobs: 5 types (Harpies, Golems, Wind Spirits...)
   - Objects: 1 Key item (Vial of Grace), Standard Loot tables
   - Shops: 0
 * type: textarea
   id: acceptance_criteria
   attributes:
   label: Acceptance Criteria (Functional)
   description: Testable conditions for the area to be considered "Alpha".
   value: |
   - [ ] Zone compiles/loads without VNUM collisions.
   - [ ] All room exits link reciprocally (North <-> South) unless one-way is intended.
   - [ ] Zone Reset (Lifespan) triggers correctly in logs.
   - [ ] No "Death Traps" (DT) without at least one warning room warning.
   - [ ] Max Level set correctly to prevent low-level farming.
 * type: checkboxes
   id: dod
   attributes:
   label: Definition of Done (Quality Gate)
   description: Confirm adherence to CircleMUD building standards.
   options:
   - label: Descriptions are spell-checked and use proper line breaks/color codes.
   - label: All Mobs have valid flags (e.g., IS_NPC, SENTINEL/SCAVENGER).
   - label: Extra Descriptions (EXDESC) added for key keywords in rooms.
   - label: Gold/Exp rewards balanced against MUD economy standards.
   - label: Zone file (.zon) commands end strictly with S.

