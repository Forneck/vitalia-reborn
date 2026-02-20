# Vitalia Reborn – Comprehensive TODO Audit

**Generated:** 2026  
**Purpose:** Cross-reference all planned, documented, and tracked features against actual
source-code to determine true implementation status and produce an actionable plan.  
**Methodology:** Documentation files read + `grep`/`wc` checks against `src/*.c`, `src/*.h`,
`lib/world/trg/`, and `src/interpreter.c`.

---

## Table of Contents

1. [Category A – Already Implemented (Docs Need Updating)](#a)
2. [Category B – Partially Implemented](#b)
3. [Category C – Truly Pending (Planned but Not Implemented)](#c)
4. [Category D – Backlog / Legacy World-Content Items](#d)
5. [Proposed Plan of Action](#plan)
6. [Quick-Reference Matrix](#matrix)

---

## <a name="a"></a>Category A – Already Implemented (Docs Need Updating)

These features are **fully implemented in source code**.  
Where docs say "TODO", "planned", or "Phase N", the reality is that the code already
ships the feature. Documentation should be updated to reflect the current state.

---

### A-1 · Hybrid Emotion System v2.0 (20-Dimensional)

| Aspect | Status |
|--------|--------|
| 20 emotion fields in `mob_ai_data` | ✅ `src/structs.h:1267-1297` |
| `get_effective_emotion_toward()` | ✅ `src/utils.c` |
| `get_relationship_emotion()` + memory buffer | ✅ `src/structs.h:1189`, `src/utils.c` |
| Combat flee / anger / horror / pain effects | ✅ `src/fight.c` |
| Shop trust/greed per-player | ✅ `src/shop.c` |
| Quest trust/reward modifier | ✅ `src/quest.c` |
| Love-based following | ✅ `src/mobact.c` |
| Group loyalty/envy emotions | ✅ `src/mobact.c`, `src/fight.c` |
| Extreme-state berserk / paralysis | ✅ `src/structs.h` (berserk_timer, paralyzed_timer) |
| Mood system (`overall_mood`) | ✅ `src/structs.h:1300` |
| Weather/time-of-day mood influence | ✅ `src/weather.c`, `src/mobact.c` |
| Emotion display per-viewer (stat mob) | ✅ `src/act.wizard.c` |

**Evidence:** `src/emotion_projection.c` (561 lines), `src/structs.h:1267-1344`.  
**Action required:** Update `EMOTION_SYSTEM_TODO.md` sections 1.x, 2.x, 3.x, 5.x, 9.1 that
still show `**DONE**` inline but whose surrounding headings or roadmap bullets still list them
as "Phase 3/4 long-term."

---

### A-2 · Shadow Timeline / Cognitive Future Simulation (RFC-0001/0003)

| Aspect | Status |
|--------|--------|
| `src/shadow_timeline.c` (2 072 lines) | ✅ Exists & compiles |
| `MOB_SHADOWTIMELINE` flag | ✅ `src/structs.h:270` |
| Movement/combat/social/item/spell projections | ✅ (10 `generate_*_projection` functions) |
| RFC-0003 compliance (MUST requirements) | ✅ All 10 audited in `RFC_0003_COMPLIANCE_AUDIT.md` |
| `attention_bias` + `recent_prediction_error` | ✅ `src/structs.h:1268` (shadow_timeline fields) |

**Action required:** `SHADOW_TIMELINE.md` may still describe this as "Implementation Plan".
Update status header to "Production" and reference RFC compliance document.

---

### A-3 · Moral Reasoner (Shultz & Daley Rule-Based Model)

| Aspect | Status |
|--------|--------|
| `src/moral_reasoner.c` (1 531 lines) | ✅ Exists |
| `moral_evaluate_action_cost()` | ✅ Confirmed by `src/mobact.c` calls |
| Memory-based learning (`moral_record_action`, `moral_get_learned_bias`) | ✅ in moral_reasoner.c |
| Regret-based avoidance | ✅ `moral_regret_level` in `src/structs.h` |

**Action required:** `MORAL_REASONING.md` status header still says "Status: Production Ready"
(correct) but `RFC-1001_NPC_PSYCHOLOGY_CHECKLIST.md` still lists moral reasoning as
"✅ implemented" without linking to the compliance audit — no content action needed.

---

### A-4 · Emotion Projection (4D Relational Decision Space)

| Aspect | Status |
|--------|--------|
| `src/emotion_projection.c` (561 lines) | ✅ Exists |
| `emotion_4d_state` struct | ✅ `src/structs.h:1132` |
| `last_4d_state` stored per mob | ✅ `src/structs.h:1344` |

**Action required:** No standalone design document for this component; ensure
`HYBRID_EMOTION_SYSTEM.md` references the 4D projection layer.

---

### A-5 · Group Moral Dynamics

| Aspect | Status |
|--------|--------|
| `group_data.moral_reputation` field | ✅ `src/structs.h:1396` |
| `moral_get_peer_pressure()` | ✅ `src/moral_reasoner.c:1182` |
| `moral_would_dissent_from_group()` | ✅ `src/moral_reasoner.c` |
| `moral_evaluate_group_action()` | ✅ `src/moral_reasoner.c` |
| Integration in `mobact.c` | ✅ `src/mobact.c:604-622` |
| `collective_guilt_count` | ✅ `src/structs.h`, `src/moral_reasoner.c:1163,1424` |

**Action required:** `GROUP_MORAL_DYNAMICS.md` is accurate; confirm "Integration Date: 2026-02-17"
is reflected in changelog.

---

### A-6 · Emotion Contagion System

| Aspect | Status |
|--------|--------|
| `update_mob_emotion_contagion()` called in mobact.c | ✅ `src/mobact.c:577` |
| Peer/leader influence transfer | ✅ integrated with group dynamics |

**Action required:** `EMOTION_CONTAGION.md` may still list contagion as "planned future."
Update to "implemented."

---

### A-7 · Big Five (OCEAN) – Phase 1: Neuroticism + Phase 2: Conscientiousness

| Aspect | Status |
|--------|--------|
| `struct mob_personality` with all 5 fields | ✅ `src/structs.h:1034-1055` |
| Neuroticism gain-amplifier active | ✅ cedit config (`neuroticism_gain_*`) |
| `apply_conscientiousness_impulse_modulation()` | ✅ `src/mobact.c:6084` |
| `apply_conscientiousness_reaction_delay()` | ✅ `src/mobact.c:6115` |
| cedit Phase 1 & 2 config fields | ✅ `src/structs.h:2083-2110` |

**Action required:** `BIG_FIVE_PERSONALITY_SYSTEM.md` correctly marks Phase 1 complete and
Phase 2 as "✅"; double-check the "⏳ Phase 2" line in the roadmap — should be updated to ✅.

---

### A-8 · Reputation System (Players + Mobs)

| Aspect | Status |
|--------|--------|
| `player_specials.saved.reputation` + `GET_REPUTATION()` | ✅ `src/structs.h:1524` |
| `calculate_player_reputation()` | ✅ `src/players.c` (called from fight.c/magic.c/quest.c) |
| `modify_player_reputation()` | ✅ `src/fight.c:1381`, `src/magic.c:294`, `src/quest.c:2687` |
| Mob reputation (`ai_data->reputation`) | ✅ `src/structs.h:1260`, initialized to 40 |
| Shop/quest behavior using reputation | ✅ `src/quest.c:1527`, `src/shop.c:591` |

---

### A-9 · Evil Reputation System

| Aspect | Status |
|--------|--------|
| `CLASS_REP_DARK_MAGIC` (12) | ✅ `src/magic.c:291` |
| `CLASS_REP_HARM_SPELL` (13) | ✅ `src/magic.c:516` |
| Evil warrior kills | ✅ `src/fight.c:1381-1410` |
| Cursing objects gives damage bonus | ✅ `src/magic.c:1067` |
| `CONFIG_DYNAMIC_REPUTATION` gate | ✅ `src/config.c` |

---

### A-10 · Escort Quests (AQ\_MOB\_ESCORT, type 9)

| Aspect | Status |
|--------|--------|
| `AQ_MOB_ESCORT` constant | ✅ `src/quest.h` |
| `spawn_escort_mob()` | ✅ `src/quest.c:714` |
| `check_escort_quest_completion()` | ✅ `src/quest.c:799` |
| `fail_escort_quest()` | ✅ `src/handler.c:1121` |
| `escort_mob_id` player field | ✅ `src/structs.h:1514` |
| Persistence in player file (`Qesc:`) | ✅ `src/players.c` |
| 2× penalty on escort death | ✅ `src/quest.c` |

---

### A-11 · Magik Triggers (DG Script replacements)

| Aspect | Status |
|--------|--------|
| Zone 12 triggers 1200-1203 | ✅ `lib/world/trg/12.trg` |
| Zone 45 triggers 4510-4517 | ✅ `lib/world/trg/45.trg` (file exists) |
| Zone 69 triggers 6902-6911 | ✅ `lib/world/trg/69.trg` (file exists) |
| Zone 114 triggers 11400-11401 | ✅ `lib/world/trg/114.trg` |
| Zone 176 triggers 17600-17601 | ✅ `lib/world/trg/176.trg` |

**Note:** `MAGIK_TRIGGERS.md` states attachment must be done separately; verify rooms in each
zone file have the `T <trigger_vnum>` lines.

---

### A-12 · Voice Casting

| Aspect | Status |
|--------|--------|
| `check_voice_cast()` in `src/spell_parser.c:426` | ✅ Exists |
| Integration in `say` command (`src/act.comm.c:53`) | ✅ Confirmed |
| Syllable-to-spell mapping | ✅ In spell_parser.c |
| NPC exclusion | ✅ `IS_NPC(ch)` guard |

---

### A-13 · Sector System (SECT\_ROAD, SECT\_DESERT, all 17 types)

| Aspect | Status |
|--------|--------|
| 17 sector types (0-16) | ✅ `src/structs.h` |
| `movement_loss[17]` array | ✅ `src/constants.c:500-504` |
| Map symbols + legend for all sectors | ✅ `src/asciimap.c` |
| Weather movement modifiers | ✅ `src/spells.c` |

---

### A-14 · Multiline Aliases (semicolon separator)

| Aspect | Status |
|--------|--------|
| `ALIAS_SEP_CHAR` defined | ✅ `src/interpreter.c` |
| `perform_complex_alias()` function | ✅ `src/interpreter.c:801` |
| Variable substitution (`$1`, `$*`) | ✅ `src/interpreter.c` |

---

### A-15 · Percentual Load (negative arg2 in zone files)

| Aspect | Status |
|--------|--------|
| M/O/G/E/P commands accept negative arg2 | ✅ `src/db.c:3101-3260` |
| Percentage display in zedit | ✅ `src/zedit.c:469-514` |
| Clamp to -100 max | ✅ `src/zedit.c:1061` |

---

### A-16 · Weather Spell Enhancement (Mana Density)

| Aspect | Status |
|--------|--------|
| `calculate_mana_density()` | ✅ `src/spells.c:1602` |
| Power scaling in `mag_damage` | ✅ `src/spell_parser.c:256` |
| Mana cost scaling | ✅ `src/spell_parser.c:241` |
| Favored/unfavored school display in `weather` command | ✅ `src/act.informative.c:2019` |
| Mana density description in score/room | ✅ `src/act.informative.c:1143` |
| `mana_density_boost` + expiry on zone struct | ✅ `src/structs.h:1811-1812` |
| Control Weather boost integration | ✅ `src/spells.c:930` |

---

### A-17 · Spell Variant Chains (spedit + experiment command)

| Aspect | Status |
|--------|--------|
| `prerequisite_spell` + `discoverable` fields | ✅ `src/structs.h`, `src/spedit.c` |
| `spedit_show_variant_chain()` | ✅ `src/spedit.c:676` |
| Circular dependency prevention | ✅ `src/spedit.c:512` |
| `do_experiment` command | ✅ `src/act.other.c:891` |
| `experiment` in command table | ✅ `src/interpreter.c:169` |
| Prerequisite mana-cost inheritance | ✅ `src/spell_parser.c:193` |

---

### A-18 · Emotion Configuration System (cedit)

| Aspect | Status |
|--------|--------|
| `cedit_disp_emotion_menu()` | ✅ `src/cedit.c:30` |
| Per-emotion display thresholds | ✅ `src/cedit.c:178-189` |
| Emotion presets | ✅ `cedit_load_emotion_preset()` |
| All config flags exposed | ✅ `src/structs.h:2069-2117` |

---

## <a name="b"></a>Category B – Partially Implemented

Features that are **structurally present but incomplete or deliberately disabled**.

---

### B-1 · Rebegin / Remort Feature

**What exists:**
- Full state machine: `CON_RB_SKILL` → `CON_RB_NEW_CLASS` → `CON_RB_REROLL` →
  `CON_RB_QATTRS` → `CON_RB_QHOMETOWN` in `src/act.other.c:1934-2092`
- `do_rebegin()` implemented, `can_rebegin()` implemented
- `retained_skills[]` and `was_class[]` arrays in player struct and saved to file
- Menu option shown when `can_rebegin()` is true

**What is missing:**
- **The command is commented out in `src/interpreter.c:294`:**  
  `/* {"rebegin", "rebegin", POS_STANDING, do_rebegin, 0, 0, CMD_NOARG},*/`
- File `lib/text/rebegin` (rebegin rules text) — not confirmed to exist
- Balance testing and game-design sign-off

**Action:** Uncomment the command table entry; create/verify `lib/text/rebegin`; run playtesting.

---

### B-2 · Big Five (OCEAN) – Phases 3 & 4

**What exists:**
- Struct fields `extraversion`, `agreeableness`, `openness` declared in
  `struct mob_personality` (`src/structs.h:1049-1052`)
- Fields are initialized from genetics in loading code
- `conscientiousness_initialized` flag present

**What is missing:**
- Phase 3 (Extraversion: social reward gain, interaction frequency; Agreeableness:
  aggression modulation) — NOT integrated into mobact.c behavior
- Phase 4 (Openness: exploration, quest curiosity) — NOT implemented anywhere
- No cedit config items for Phases 3 & 4

**Action:** These are known future phases. `BIG_FIVE_PERSONALITY_SYSTEM.md` correctly
marks them "⏳ reserved." Implement Phase 3 after Conscientiousness is validated;
Phase 4 afterwards.

---

### B-3 · Clan System

**What exists:**
- `src/clan.c` exists but contains only the file header boilerplate (10 lines — no
  actual implementation)
- `clan.h` presumably declares structures referenced in `structs.h`

**What is missing:**
- All clan commands (`clan create`, `clan join`, `clan who`, `clan info`, etc.)
- Clan data storage
- WHO display integration (noted as bug in TODO: "Fix CLAN WHO look & feel")

**Action:** Clan system is effectively a stub. This is one of the largest genuinely
pending features. Requires full design + implementation sprint.

---

### B-4 · Hometown System

**What exists:**
- `GET_HOMETOWN(ch)` macro and `hometown` field in player struct
- `char_to_room` uses `r_hometown_1..4` for recall
- Rebegin flow includes `CON_RB_QHOMETOWN`

**What is missing:**
- Main TODO notes "Hometown em 113 - WIP"
- Menu-driven hometown selection at character creation is incomplete
- World zone 113 hometown content is work-in-progress

**Action:** Complete hometown selection menu in character creation (CON_QHOMETOWN);
finish zone 113 content; wire `r_hometown_*` to correct room vnums in config.

---

### B-5 · Guild System (Zone 134)

**What exists:**
- Zone 134 referenced in TODO as "guildas em 134"
- Class-based training NPCs likely exist (tbaMUD baseline)

**What is missing:**
- Dedicated guild rooms/content in zone 134 not confirmed complete
- Guild membership/rank system beyond basic trainer interaction

**Action:** Audit zone 134 content; create missing guild rooms; consider whether
a coded guild-rank system is needed beyond existing clan.c scaffolding.

---

## <a name="c"></a>Category C – Truly Pending (Planned but Not Implemented)

These items appear in documentation as TODOs, phase plans, or future considerations
and have **no confirming implementation found in source code**.

---

### C-1 · Communication-Based Emotion Triggers

From `EMOTION_SYSTEM_TODO.md §2.1`:
- Being insulted/cursed at → anger/shame update
- Being praised/thanked → happiness/pride update
- Being ignored (player walks away) → sadness/anger
- Receiving tells from strangers → curiosity/suspicion

**Files to modify:** `src/act.comm.c` (say/tell handlers), `src/utils.c` (emotion update).

---

### C-2 · Reputation → Emotion Initialization at First Meeting

From `EMOTION_SYSTEM_TODO.md §3.2`:
- Player reputation initializes mob emotions at first encounter
- High-reputation players start with higher trust/friendship
- Low-reputation players start with higher suspicion/fear
- Emotion levels influence reputation gains/losses

**Files to modify:** `src/utils.c` (emotion initialization), `src/players.c`
(reputation-to-emotion seeding on first mob memory write).

---

### C-3 · Emotion Visual Cues in `look` / Mob Descriptions

From `EMOTION_SYSTEM_TODO.md §4.1`:
- Emotion indicators in mob's `look` description (e.g., "the goblin trembles with fear")
- Color coding for emotional states
- Periodic spontaneous emotion emotes

**Files to modify:** `src/act.informative.c` (`do_look`), `src/utils.c`
(helper to generate emotion description string).

---

### C-4 · NPC Emotion-Driven Dialogue

From `EMOTION_SYSTEM_TODO.md §4.2`:
- Speech patterns reflect emotions (angry = aggressive, sad = withdrawn)
- High emotion levels trigger spontaneous statements
- Quest and shop dialogue adapts to emotional state

**Files to modify:** `src/act.comm.c` (NPC speech), `src/mobact.c` (periodic speech
trigger), `src/shop.c` (shopkeeper greeting based on emotion).

---

### C-5 · Emotion-Based Player Skills

From `EMOTION_SYSTEM_TODO.md §5.4`:
- **Intimidation** – increases target's fear
- **Charm** – increases target's happiness/love
- **Manipulation** – exploits emotional state
- **Empathy** – reveals mob's emotional state (read-only)

**Files to modify:** `src/class.c`, `src/spells.c` or `src/act.offensive.c` (skill
implementations), `src/utils.c` (emotion set functions).

---

### C-6 · Emotion Balance & Tuning Configuration

From `EMOTION_SYSTEM_TODO.md §6`:
- Per-emotion decay rate configuration in cedit
- Configurable extreme-emotion decay multiplier
- Per-mob emotional sensitivity settings
- Performance profiling / caching of emotion calculations

**Files to modify:** `src/cedit.c`, `src/structs.h` (new config fields), `src/utils.c`
(decay rate lookup).

---

### C-7 · Alignment ↔ Emotion Integration

From `EMOTION_SYSTEM_TODO.md §8.1`:
- Persistent anger/hatred should nudge alignment toward evil over time
- Persistent compassion/love should nudge toward good
- Alignment should influence baseline emotion values

**Files to modify:** `src/utils.c` or `src/limits.c` (periodic alignment shifts),
`src/config.c` (gate with `CONFIG_EMOTION_ALIGNMENT_SHIFTS` which exists but is
not wired to the shift logic).

---

### C-8 · Faction & Deity/Religion Emotion Integration

From `EMOTION_SYSTEM_TODO.md §8.2-8.3`:
- Faction enemies start with negative emotions
- Religious mobs have modified emotional responses
- Prayers/blessings affect emotions
- No faction system exists beyond alignment

**Note:** These depend on a faction system that is not currently implemented.
This is a long-term architectural item.

---

### C-9 · Emotional Immunity (Undead, Constructs, Psychopaths)

From `EMOTION_SYSTEM_TODO.md §9.2`:
- New `MOB_IMMUNE_EMOTION` flag (or similar) to bypass emotion updates
- Undead and constructs should not accumulate emotional state
- Psychopath personality preset with reduced empathy

**Files to modify:** `src/structs.h` (new mob flag), `src/utils.c` (guard in
`update_mob_emotion()`).

---

### C-10 · Player Emotion Visibility Tools

From `EMOTION_SYSTEM_TODO.md §7`:
- Help file explaining emotion system to players
- Command to sense mob emotional state (`sense_motive` or `empathy`)
- Builder documentation for emotion-aware quest design
- Wizard commands to manually set mob emotions for testing

**Files to modify:** `lib/text/help` (new HELP entries), `src/act.wizard.c`
(wizard emotion-set command), `src/interpreter.c` (new command entry).

---

### C-11 · Big Five Phases 3 (Extraversion / Agreeableness) and 4 (Openness)

Although struct fields exist (see B-2), no behavioral code uses them.

- **Phase 3 – Extraversion:** Social reward gain, interaction frequency bonus
- **Phase 3 – Agreeableness:** Interpersonal aggression modulation
- **Phase 4 – Openness:** Exploration drive, quest curiosity boost

**Files to modify:** `src/mobact.c` (behavior integration),
`src/cedit.c` (new config fields), `src/structs.h` (config definitions).

---

### C-12 · FANN Neural Network Integration

From `RFC-1001_NPC_PSYCHOLOGY_CHECKLIST.md` (Executive Summary):  
> ⚠️ **FANN neural networks** included but not integrated

FANN headers/library are present but no mob decision-making uses them.
This is flagged as a known gap. If neural-network-driven behavior is desired,
an integration layer connecting FANN output to mob action selection in
`shadow_timeline.c` is needed.

---

### C-13 · Monk and Paladin Classes

From main `TODO`:
> classes monge e paladinos

No `CLASS_MONK` or `CLASS_PALADIN` constants found in source.  
These are entirely missing from the class system.

**Files to modify:** `src/class.c`, `src/structs.h`, `src/spell_parser.c`,
`src/spells_assign.c`, relevant world files for trainer NPCs.

---

### C-14 · Climb Skill for Warriors/Thieves

From main `TODO`:
> Imp: Climb (guerreiros e ladrões)

`SECT_CLIMBING` exists as a sector type (used in asciimap and weather modifiers)
but there is no `do_climb` command or climb skill in `act.movement.c`.
Players attempting to enter SECT_CLIMBING rooms currently simply pay extra movement.

**Files to modify:** `src/class.c` (add skill), `src/act.movement.c` (skill check
before entering climbing rooms), `src/interpreter.c`.

---

### C-15 · Object Layering System

From main `TODO`:
> Sobreposição de objetos (Comando PUT OVER, flags NOUNDER, NOOVER, NOHIDE)
> Mover objetos (Comando PUSH, flag MOVEABLE)

No `NOUNDER`/`NOOVER`/`NOHIDE` item flags or `do_push`/`do_put_over` commands exist.

**Files to modify:** `src/structs.h` (new item flags), `src/act.item.c` (new commands).

---

### C-16 · Banking System (Banco de Vitalia)

From main `TODO`:
> Imp: Informatizar o banco de Vitalia

A bank NPC spec_proc likely exists (tbaMUD baseline) but a fully featured
"Vitalia Bank" with statements, interest, and a player-facing interface is planned
but not implemented.

**Files to modify:** `src/spec_procs.c` (bank spec), world files for zone 113/134.

---

### C-17 · Auction House (Casa de Leilões)

From main `TODO`:
> Casa de leilões

`src/auction.c` and `src/act.auction.c` exist (tbaMUD auction system) but a full
NPC-driven auction house (as opposed to player-to-player commands) with a dedicated
in-game building is not implemented.

---

### C-18 · Miscellaneous Bug Fixes and QoL Items (from main TODO)

| Item | Description |
|------|-------------|
| Gas pão CRASH | Bug: specific item causes crash — needs investigation |
| Gold save in corpses | Bug: gold not correctly persisted in PC corpses |
| REPLY forgotten on quit | Bug: `last_tell` not cleared cleanly |
| WHERE / STAT OBJ show invis gods | Bug: visibility checks missing |
| FLOOD control | Feature: anti-flood command throttling |
| Improved plrlist | Feature: search by clan, range levels, configurable output |
| pk_allowed / pk_flag audit | Fix: PK flags inconsistency |
| Immortal HP display | Fix: HP display for immortal-level characters |
| AFF flags wizard command | Feature: command to toggle AFF flags directly |
| Color profiles | Feature: per-player color scheme selection |
| Default wiznet level | Feature: configurable default wiznet visibility level |
| WHO clan formatting | Fix: `clan who` appearance to match `who` format |
| Two-handed modifier check | Fix: inventory modifier for two-handed weapons |
| `talk_dead` on move | Feature/Fix: dead mobs interacting during movement |
| Object-level restriction (2nd enchantment) | Feature: objects requiring 2+ enchants to wear |
| Consider command improvements | Feature: richer `consider` feedback |
| PUSH command / MOVEABLE flag | See C-15 |

---

## <a name="d"></a>Category D – Backlog / Legacy World-Content Items

These are **world-building tasks** (not code features) tracked in the main `TODO`.

| Item | Description |
|------|-------------|
| Mobs revistos até zon 126 | Accent/encoding review of mob names/descriptions in listed zones |
| Areas acentuadas | Encoding fix for room descriptions in listed zones |
| Objs acentuados | Encoding fix for object names/descriptions |
| Lojas (shops) | Complete shop configuration for missing zones (listed) |
| Mob sem load (750, 2399) | Two mobs defined but never placed in zone reset commands |
| Falta nome obst em 111 | Missing exit/obstacle names in zone 111 |
| Porta 11427 west hidden pickproof | Door attribute fix |
| Typo 4048 (east exit desc) | Text correction |
| 13436 desc undefined | Room missing description |
| completar objs 65 | Objects in zone 65 (STRANGELOVE ORIG) need completion |
| Guildas em 134 | Guild rooms in zone 134 (also see B-5) |
| Falta loja catedral de cristal | Shop missing from crystal cathedral location |
| Falta 221 & para @ | Zone 221 encoding issue |

---

## <a name="plan"></a>Proposed Plan of Action

### Immediate Actions (Week 1-2)

1. **Update stale documentation** for all Category A items:
   - Change `EMOTION_SYSTEM_TODO.md` Phase 3/4 roadmap items that are actually done
   - Update `SHADOW_TIMELINE.md` status to "Production"
   - Update `BIG_FIVE_PERSONALITY_SYSTEM.md` Phase 2 roadmap from ⏳ to ✅
   - Update `EMOTION_CONTAGION.md` status from planned to implemented

2. **Enable Rebegin command (B-1):**
   - Uncomment line 294 in `src/interpreter.c`
   - Verify `lib/text/rebegin` exists
   - Run end-to-end playtesting of the remort flow

### Short-Term (Month 1)

3. **Bug fixes from C-18:** Prioritize Gold-in-corpse save, Gas pão crash, REPLY-on-quit,
   and WHERE/STAT visibility checks (all are isolated bugs with contained scope).

4. **Emotion Visual Cues (C-3):** High-visibility player-facing feature; adds immersion
   without risk to game balance.

5. **Emotional Immunity flag (C-9):** Required for undead/construct lore accuracy; small
   isolated change to `update_mob_emotion()`.

6. **Magik trigger attachment audit (A-11):** Verify all five zones have `T <vnum>` lines
   in their room definitions.

### Medium-Term (Months 2-3)

7. **Communication Emotion Triggers (C-1):** Extend `do_say`/`do_tell` to fire emotion
   updates; low-risk incremental change.

8. **Reputation → Emotion seeding (C-2):** Wire first-encounter emotion initialization
   to player reputation; completes the reputation ↔ emotion feedback loop.

9. **Big Five Phase 3 (C-11):** Implement Extraversion (social reward) and Agreeableness
   (aggression modulation) in `mobact.c`; add cedit knobs.

10. **Climb skill (C-14):** Add climbing skill for Warriors/Thieves with SECT_CLIMBING
    gate in `act.movement.c`.

11. **World-content encoding sweep (Category D):** Batch-fix mob/area/object accents;
    complete shop assignments for listed zones.

### Long-Term (Quarter 2+)

12. **Clan System (B-3):** Full implementation sprint — largest pending feature.

13. **Monk & Paladin classes (C-13):** New classes require class table, skills, spells,
    trainer NPCs, balance work.

14. **Emotion-based player skills (C-5):** Intimidation, Charm, Manipulation, Empathy.

15. **NPC Emotion-Driven Dialogue (C-4):** Requires dialogue table or trigger-based
    approach; coordinate with DG Scripts team.

16. **Banking System (C-16) + Auction House (C-17):** Companion features for economy;
    design before implementing.

17. **Big Five Phase 4: Openness (C-11):** After Phase 3 is stable.

18. **FANN neural network integration (C-12):** Research task — needs feasibility study
    before committing to implementation.

---

## <a name="matrix"></a>Quick-Reference Matrix

| Feature | Doc File | Code Status | Category |
|---------|----------|-------------|----------|
| Hybrid Emotion System (20-dim) | EMOTION_SYSTEM_TODO.md | ✅ Implemented | A-1 |
| Emotion memory / relationship layer | HYBRID_EMOTION_SYSTEM.md | ✅ Implemented | A-1 |
| Shadow Timeline (RFC-0001/0003) | SHADOW_TIMELINE.md | ✅ Implemented | A-2 |
| Moral Reasoner (Shultz & Daley) | MORAL_REASONING.md | ✅ Implemented | A-3 |
| Emotion Projection (4D) | — | ✅ Implemented | A-4 |
| Group Moral Dynamics | GROUP_MORAL_DYNAMICS.md | ✅ Implemented | A-5 |
| Emotion Contagion | EMOTION_CONTAGION.md | ✅ Implemented | A-6 |
| Big Five Phase 1: Neuroticism | BIG_FIVE_PERSONALITY_SYSTEM.md | ✅ Implemented | A-7 |
| Big Five Phase 2: Conscientiousness | BIG_FIVE_PERSONALITY_SYSTEM.md | ✅ Implemented | A-7 |
| Reputation System (players + mobs) | REPUTATION_SYSTEM.md | ✅ Implemented | A-8 |
| Evil Reputation System | EVIL_REPUTATION_SYSTEM.md | ✅ Implemented | A-9 |
| Escort Quests (AQ_MOB_ESCORT) | ESCORT_QUESTS.md | ✅ Implemented | A-10 |
| Magik Triggers (DG Script) | MAGIK_TRIGGERS.md | ✅ Implemented | A-11 |
| Voice Casting | VOICE_CASTING.md | ✅ Implemented | A-12 |
| Sector System (Road/Desert) | SECTOR_SYSTEM.md | ✅ Implemented | A-13 |
| Multiline Aliases | MULTILINE_ALIASES.md | ✅ Implemented | A-14 |
| Percentual Load (zone files) | PERCENTUAL_LOAD.md | ✅ Implemented | A-15 |
| Weather / Mana Density System | WEATHER_SPELL_ENHANCEMENT.md | ✅ Implemented | A-16 |
| Spell Variant Chains + experiment | SPELL_VARIANT_CHAINS.md | ✅ Implemented | A-17 |
| Emotion Config (cedit) | CEDIT_EMOTION_CONFIGURATION_GUIDE.md | ✅ Implemented | A-18 |
| Rebegin / Remort | REBEGIN_FEATURE.md | ⚠️ Command disabled | B-1 |
| Big Five Phases 3 & 4 | BIG_FIVE_PERSONALITY_SYSTEM.md | ⚠️ Struct only | B-2 |
| Clan System | — | ⚠️ Stub (10-line file) | B-3 |
| Hometown System | TODO | ⚠️ Partial (WIP) | B-4 |
| Guild System (zone 134) | TODO | ⚠️ Partial | B-5 |
| Communication emotion triggers | EMOTION_SYSTEM_TODO.md §2.1 | ❌ Not implemented | C-1 |
| Reputation → emotion seeding | EMOTION_SYSTEM_TODO.md §3.2 | ❌ Not implemented | C-2 |
| Emotion visual cues in look | EMOTION_SYSTEM_TODO.md §4.1 | ❌ Not implemented | C-3 |
| NPC emotion-driven dialogue | EMOTION_SYSTEM_TODO.md §4.2 | ❌ Not implemented | C-4 |
| Emotion-based player skills | EMOTION_SYSTEM_TODO.md §5.4 | ❌ Not implemented | C-5 |
| Emotion balance/tuning config | EMOTION_SYSTEM_TODO.md §6 | ❌ Not implemented | C-6 |
| Alignment ↔ emotion integration | EMOTION_SYSTEM_TODO.md §8.1 | ❌ Not implemented | C-7 |
| Faction / Deity emotion integration | EMOTION_SYSTEM_TODO.md §8.2-8.3 | ❌ Not implemented | C-8 |
| Emotional immunity (undead/constructs) | EMOTION_SYSTEM_TODO.md §9.2 | ❌ Not implemented | C-9 |
| Player emotion visibility tools | EMOTION_SYSTEM_TODO.md §7 | ❌ Not implemented | C-10 |
| Big Five Phase 3: Extraversion/Agree. | BIG_FIVE_PERSONALITY_SYSTEM.md | ❌ Struct only | C-11 |
| Big Five Phase 4: Openness | BIG_FIVE_PERSONALITY_SYSTEM.md | ❌ Struct only | C-11 |
| FANN neural network integration | RFC-1001 | ❌ Included, not integrated | C-12 |
| Monk class | TODO | ❌ Not implemented | C-13 |
| Paladin class | TODO | ❌ Not implemented | C-13 |
| Climb skill (warriors/thieves) | TODO | ❌ No do_climb command | C-14 |
| Object layering (PUT OVER, PUSH) | TODO | ❌ Not implemented | C-15 |
| Vitalia Banking System | TODO | ❌ Not implemented | C-16 |
| Auction House (NPC-driven) | TODO | ❌ Not implemented | C-17 |
| Misc bug fixes (gold corpse, etc.) | TODO | ❌ Open bugs | C-18 |
| World encoding (mobs/areas/objs) | TODO | 🔨 Content work | D |
| Zone shop completions | TODO | 🔨 Content work | D |

---

*End of TODO Audit — Vitalia Reborn*
