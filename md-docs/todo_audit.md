# Vitalia Reborn – Comprehensive TODO Audit

**Generated:** February 2026 | **Last Updated:** February 28 2026 (Priority 1 documentation corrections completed)  
**Purpose:** Cross-reference all `.md` files, the root `TODO` file, `lib/text/news`,
`changelog.txt`, `lib/misc/ideas`, `lib/misc/bugs`, and `lib/misc/typos` against the
actual source code to determine the true implementation status of every planned or
documented feature, and produce an actionable plan for what remains to be done.  
**Methodology:** All documentation files were read; source files in `src/` were searched to
confirm or deny each claim; `lib/text/news` and `changelog.txt` were used as ground-truth
for release history; player-submitted `ideas`, `bugs`, and `typos` were reviewed and
cross-referenced with known fixes.  
**Scope:** Documentation audit only. No source-code changes are made by this document.

---

## Table of Contents

1. [Category A – Already Implemented (Documentation Needs Updating)](#a)
2. [Category B – Partially Implemented](#b)
3. [Category C – Genuinely Pending (Planned but Not Yet Implemented)](#c)
4. [Category D – World-Content Backlog](#d)
5. [Category E – Player-Reported Ideas (`lib/misc/ideas`)](#e)
6. [Category F – Player-Reported Bugs (`lib/misc/bugs`)](#f)
7. [Category G – Player-Reported Typos (`lib/misc/typos`)](#g)
8. [Documentation Inaccuracies Found](#inaccuracies)
9. [Proposed Plan of Action](#plan)
10. [Quick-Reference Matrix](#matrix)

---

## <a name="a"></a>Category A – Already Implemented (Documentation Needs Updating)

These features are **fully implemented in the source code**.  
Some documentation files still describe them as "TODO", "planned", or "Phase N (future)".
Those documents need to be updated to reflect the current production state.

---

### A-1 · Hybrid Emotion System v2.0 (20-Dimensional)

**Documentation:** `md-docs/EMOTION_SYSTEM_TODO.md`, `md-docs/HYBRID_EMOTION_SYSTEM.md`  
**Confirmed in:** `src/emotion_projection.c` (561 lines), `src/structs.h`, `src/utils.c`,
`src/fight.c`, `src/shop.c`, `src/quest.c`, `src/mobact.c`  
**News reference:** Oct/Nov 2025 — *"Sistema completo de comportamento baseado em emoções para mobs. 20 emoções rastreadas com indicadores visuais e memória aprimorada."*

| Component | Code Status |
|-----------|-------------|
| 20 emotion fields in `mob_ai_data` | ✅ Implemented |
| Per-player emotion memory (circular buffer, 10 entries) | ✅ Implemented |
| `get_effective_emotion_toward()` — mood + relationship combined | ✅ Implemented |
| Combat flee/anger/horror/pain emotion effects | ✅ Implemented |
| Shop trust/greed per-player pricing | ✅ Implemented |
| Quest trust/reward modifier | ✅ Implemented |
| Love-based NPC following | ✅ Implemented |
| Group loyalty/envy emotion integration | ✅ Implemented |
| Extreme-state berserk / paralysis timers | ✅ Implemented |
| Overall mood system (`overall_mood`) | ✅ Implemented |
| Weather and time-of-day mood influence | ✅ Implemented |
| Emotion display per-viewer in `stat mob` | ✅ Implemented |

**Documentation status:** ✅ **Resolved** — `EMOTION_SYSTEM_TODO.md` phase roadmap updated (Feb 28 2026). Phase 2 items confirmed complete; Phase 3/4 roadmap now accurately reflects pending work only.

---

### A-2 · Shadow Timeline / Cognitive Future Simulation (RFC-0001 / RFC-0003)

**Documentation:** `md-docs/SHADOW_TIMELINE.md`, `md-docs/RFC_0003_DEFINITION.md`,
`md-docs/RFC_0003_COMPLIANCE_AUDIT.md`, `md-docs/SHADOW_TIMELINE_SENTINEL_FIX.md`  
**Confirmed in:** `src/shadow_timeline.c` (2 072 lines), `src/structs.h`  
**News reference:** Jan/Feb 2026 — *"Correções de estabilidade e de bugs"* (ongoing maintenance confirms production use)

| Component | Code Status |
|-----------|-------------|
| Shadow Timeline core engine | ✅ Implemented |
| `MOB_SHADOWTIMELINE` flag | ✅ Implemented |
| 10 projection generators (move/combat/social/item/spell/etc.) | ✅ Implemented |
| RFC-0003 compliance — all MUST requirements | ✅ Audited and passing |
| Adaptive feedback (`attention_bias`, `recent_prediction_error`) | ✅ Implemented (Phase 2) |
| Sentinel guard-duty integration | ✅ Implemented (per `SHADOW_TIMELINE_SENTINEL_FIX.md`) |

**Documentation status:** ✅ **Resolved** — `SHADOW_TIMELINE.md` "Implemented Features" section already carried ✅ for both Phase 1 (2026-02-07) and Phase 2 (2026-02-16). "Future Extensions" section (Phase 3) remains correctly marked as unimplemented. No further update required.

---

### A-3 · Moral Reasoner (Shultz & Daley Rule-Based Model)

**Documentation:** `md-docs/MORAL_REASONING.md`, `GROUP_MORAL_DYNAMICS_SUMMARY.md`  
**Confirmed in:** `src/moral_reasoner.c` (1 531 lines), `src/mobact.c`, `src/structs.h`

| Component | Code Status |
|-----------|-------------|
| `moral_evaluate_action_cost()` | ✅ Implemented |
| Memory-based moral learning (`moral_record_action`, `moral_get_learned_bias`) | ✅ Implemented |
| Regret-based avoidance (`moral_regret_level`) | ✅ Implemented |
| 10 moral action types | ✅ Implemented |
| Alignment × emotion modifiers on moral cost | ✅ Implemented |

**Documentation issue:** `MORAL_REASONING.md` status is correctly "Production Ready."
No updates required.

---

### A-4 · Emotion Projection (4D Relational Decision Space)

**Documentation:** `md-docs/HYBRID_EMOTION_SYSTEM.md` (referenced but not detailed)  
**Confirmed in:** `src/emotion_projection.c`, `src/structs.h` (`emotion_4d_state`, `last_4d_state`)

**Documentation status:** ✅ **Resolved** — `HYBRID_EMOTION_SYSTEM.md` updated (Feb 28 2026) with a new "4D Emotion Projection Component" section covering the 4-axis model (Valence/Arousal/Dominance/Affiliation), core formula, Contextual Modulation Layer, relationship to mood/relationship layers, and full API reference.

---

### A-5 · Group Moral Dynamics

**Documentation:** `md-docs/GROUP_MORAL_DYNAMICS.md`, `GROUP_MORAL_DYNAMICS_SUMMARY.md`  
**Confirmed in:** `src/moral_reasoner.c`, `src/structs.h` (`group_data.moral_reputation`,
`collective_guilt_count`), `src/mobact.c`

| Component | Code Status |
|-----------|-------------|
| `moral_get_peer_pressure()` | ✅ Implemented |
| `moral_would_dissent_from_group()` | ✅ Implemented |
| `moral_evaluate_group_action()` | ✅ Implemented |
| `collective_guilt_count` tracking | ✅ Implemented |
| Integration in mob activity loop | ✅ Implemented |

**Documentation issue:** `GROUP_MORAL_DYNAMICS.md` is accurate. Confirm "Integration Date:
2026-02-17" is reflected in `changelog.txt` (the Feb 17 2026 entry covers Phase 1
Neuroticism — the group moral dynamics entry may be missing from the changelog).

---

### A-6 · Emotion Contagion System

**Documentation:** `md-docs/EMOTION_CONTAGION.md`  
**Confirmed in:** `src/mobact.c` (call to `update_mob_emotion_contagion()`)

| Component | Code Status |
|-----------|-------------|
| Peer emotion transfer (5–15%) | ✅ Implemented |
| Fear spreading in groups (12–20%) | ✅ Implemented |
| Leader emotion influence (2× multiplier) | ✅ Implemented |

**Documentation status:** ✅ **Resolved** — `EMOTION_CONTAGION.md` updated (Feb 28 2026) with an explicit "Status: ✅ Implemented" header block.

---

### A-7 · Big Five (OCEAN) – Phase 1: Neuroticism and Phase 2: Conscientiousness

**Documentation:** `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md`, `CONSCIENTIOUSNESS_SUMMARY.md`,
`CONSCIENTIOUSNESS_DECISION_PIPELINE.md`  
**Confirmed in:** `src/structs.h`, `src/mobact.c`, `src/cedit.c`  
**Changelog reference:** [Feb 17 2026] — *"Big Five (OCEAN) Personality System - Phase 1: Neuroticism"*  
**CONSCIENTIOUSNESS_SUMMARY.md** confirms: *"All phases of Conscientiousness (Big Five Phase 2) are now fully implemented."*

| Component | Code Status |
|-----------|-------------|
| `struct mob_personality` with all 5 OCEAN fields | ✅ Declared |
| Phase 1 – Neuroticism gain-amplifier | ✅ Implemented |
| Phase 1 – Soft saturation clamp | ✅ Implemented |
| Phase 1 – Genetic initialization formula | ✅ Implemented |
| Phase 1 – CEDIT configuration | ✅ Implemented |
| Phase 2 – `calculate_emotional_arousal()` | ✅ Implemented |
| Phase 2 – `apply_conscientiousness_impulse_modulation()` | ✅ Implemented |
| Phase 2 – `apply_conscientiousness_reaction_delay()` | ✅ Implemented |
| Phase 2 – `apply_conscientiousness_moral_weight()` | ✅ Implemented |
| Phase 2 – CEDIT configuration | ✅ Implemented |
| Phase 2 – Shadow Timeline attack integration | ✅ Implemented |

**Documentation status:** ✅ **Resolved** — `BIG_FIVE_PERSONALITY_SYSTEM.md` updated (Feb 28 2026): Phase 2 (Conscientiousness) marked ✅ complete; Conscientiousness moved from "Reserved for Future Phases" to "Implemented"; Phase 2 changelog entry added; "Upcoming" section updated. `changelog.txt` entry added for [Feb 28 2026].

---

### A-8 · Reputation System (Players + Mobs)

**Documentation:** `md-docs/REPUTATION_SYSTEM.md`  
**Confirmed in:** `src/players.c`, `src/fight.c`, `src/magic.c`, `src/quest.c`, `src/shop.c`, `src/structs.h`  
**News reference:** Oct/Nov 2025 — *"Novo sistema completo de reputação jogador-mob com bônus por classe."*

| Component | Code Status |
|-----------|-------------|
| Player reputation field + `GET_REPUTATION()` macro | ✅ Implemented |
| `calculate_player_reputation()` | ✅ Implemented |
| `modify_player_reputation()` called from combat/magic/quests | ✅ Implemented |
| Mob reputation (`ai_data->reputation`, initialized to 40) | ✅ Implemented |
| Shop and quest behavior gated on reputation | ✅ Implemented |
| Reputation displayed in `score` and `stat` | ✅ Implemented |

---

### A-9 · Evil Reputation System

**Documentation:** `md-docs/EVIL_REPUTATION_SYSTEM.md`  
**Confirmed in:** `src/magic.c`, `src/fight.c`  
**Changelog reference:** [Oct 30 2024] — *"Evil Player Reputation System"*

| Component | Code Status |
|-----------|-------------|
| `CLASS_REP_DARK_MAGIC` reputation type | ✅ Implemented |
| `CLASS_REP_HARM_SPELL` reputation type | ✅ Implemented |
| Evil warrior combat reputation gains | ✅ Implemented |
| Cursing objects gives damage bonus for evil players | ✅ Implemented |
| `CONFIG_DYNAMIC_REPUTATION` feature gate | ✅ Implemented |

---

### A-10 · Escort Quests (`AQ_MOB_ESCORT`, type 9)

**Documentation:** `md-docs/ESCORT_QUESTS.md`, `md-docs/ESCORT_QUEST_IMPLEMENTATION_SUMMARY.md`  
**Confirmed in:** `src/quest.h`, `src/quest.c`, `src/handler.c`, `src/players.c`, `src/structs.h`

| Component | Code Status |
|-----------|-------------|
| `AQ_MOB_ESCORT` quest type constant | ✅ Implemented |
| `spawn_escort_mob()` | ✅ Implemented |
| `check_escort_quest_completion()` | ✅ Implemented |
| `fail_escort_quest()` on escort death | ✅ Implemented |
| `escort_mob_id` persisted in player file | ✅ Implemented |
| 2× reward penalty on escort death | ✅ Implemented |

---

### A-11 · Magik Triggers (DG Script Assignments)

**Documentation:** `md-docs/MAGIK_TRIGGERS.md`  
**Confirmed in:** `lib/world/trg/` (zone files 12, 45, 69, 114, 176)

| Zone | Trigger Range | File Status |
|------|---------------|-------------|
| Zone 12 | 1200–1203 | ✅ `12.trg` exists |
| Zone 45 | 4510–4517 | ✅ `45.trg` exists |
| Zone 69 | 6902–6911 | ✅ `69.trg` exists |
| Zone 114 | 11400–11401 | ✅ `114.trg` exists |
| Zone 176 | 17600–17601 | ✅ `176.trg` exists |

**Open item:** `MAGIK_TRIGGERS.md` notes that trigger-to-room attachment (`T <vnum>` lines
in `.wld` files) must be verified separately. This attachment step should be audited for each
zone listed above.

---

### A-12 · Voice Casting

**Documentation:** `md-docs/VOICE_CASTING.md`, `md-docs/VOICE_CASTING_IMPLEMENTATION.md`,
`md-docs/VOICE_CAST_SYLLABLES_FEATURE.md`  
**Confirmed in:** `src/spell_parser.c`, `src/act.comm.c`  
**News reference:** Oct/Nov 2025 — *"Sistema de sílabas expandido com mecânicas de modificadores para voice casting. Novo comando syllables e descoberta de variantes de magias."*

| Component | Code Status |
|-----------|-------------|
| `check_voice_cast()` in spell parser | ✅ Implemented |
| Integration into `say` command | ✅ Implemented |
| Syllable-to-spell mapping | ✅ Implemented |
| Syllable modifiers (35 transformations) | ✅ Implemented |
| NPC exclusion guard | ✅ Implemented |
| `syllables` command | ✅ Implemented |

---

### A-13 · Sector System (All 17 Types)

**Documentation:** `md-docs/SECTOR_SYSTEM.md`  
**Confirmed in:** `src/structs.h`, `src/constants.c`, `src/asciimap.c`, `src/spells.c`

| Component | Code Status |
|-----------|-------------|
| 17 sector types (INSIDE through CLIMBING) | ✅ Implemented |
| `movement_loss[17]` movement cost array | ✅ Implemented |
| ASCII map symbols and legend for all sectors | ✅ Implemented |
| Weather movement cost modifiers | ✅ Implemented |

---

### A-14 · Multiline Aliases (Semicolon Separator)

**Documentation:** `md-docs/MULTILINE_ALIASES.md`  
**Confirmed in:** `src/interpreter.c`  
**News reference:** Dec 2025 — *"Sistema de alias agora suporta comandos de até 8191 caracteres."*

| Component | Code Status |
|-----------|-------------|
| `ALIAS_SEP_CHAR` (`;`) defined | ✅ Implemented |
| `perform_complex_alias()` function | ✅ Implemented |
| Variable substitution (`$1`, `$*`) | ✅ Implemented |
| Up to 8 191 character alias length | ✅ Implemented |
| Help file `HELP ALIAS` updated | ✅ Confirmed (news reference) |

---

### A-15 · Percentual Load (Negative `arg2` in Zone Files)

**Documentation:** `md-docs/PERCENTUAL_LOAD.md`, `md-docs/PERCENTUAL_LOAD_SUMMARY.md`  
**Confirmed in:** `src/db.c`, `src/zedit.c`  
**Changelog reference:** [Nov 15 2024] — *"Percentual Load System for Zone Resets"*

| Component | Code Status |
|-----------|-------------|
| M/O/G/E/P zone commands accept negative arg2 | ✅ Implemented |
| Zedit displays "Chance: X%" for percentual loads | ✅ Implemented |
| Zedit displays "Max: X" for traditional loads | ✅ Implemented |
| 100% cap enforced | ✅ Implemented |

---

### A-16 · Weather Spell Enhancement (Mana Density)

**Documentation:** `md-docs/WEATHER_SPELL_ENHANCEMENT.md`, `md-docs/FEASIBILITY_ANALYSIS.md`  
**Confirmed in:** `src/spells.c`, `src/spell_parser.c`, `src/act.informative.c`, `src/structs.h`  
**News reference:** Oct 2025 — *"Novos arquivos de ajuda: HELP CLIMA-MAGICO, HELP ESCOLAS-MAGICAS, HELP ELEMENTOS-MÁGICOS e HELP HABILIDADES-ESCOLAS."*

| Component | Code Status |
|-----------|-------------|
| `calculate_mana_density()` | ✅ Implemented |
| Spell damage scaling by mana density | ✅ Implemented |
| Mana cost scaling by mana density | ✅ Implemented |
| Favored/unfavored school display in `weather` command | ✅ Implemented |
| Mana density description in `score`/room view | ✅ Implemented |
| `mana_density_boost` + expiry on zone struct | ✅ Implemented |
| Control Weather spell boost integration | ✅ Implemented |

---

### A-17 · Spell Variant Chains (`spedit` + `experiment` Command)

**Documentation:** `md-docs/SPELL_VARIANT_CHAINS.md`, `md-docs/SPELL_VARIANT_EXAMPLE.md`  
**Confirmed in:** `src/structs.h`, `src/spedit.c`, `src/act.other.c`, `src/interpreter.c`,
`src/spell_parser.c`  
**News reference:** Oct/Nov 2025 — *"Suporte no spedit para campos de variantes de magias com sistema de pré-requisitos."*

| Component | Code Status |
|-----------|-------------|
| `prerequisite_spell` and `discoverable` fields | ✅ Implemented |
| `spedit_show_variant_chain()` | ✅ Implemented |
| Circular dependency prevention | ✅ Implemented |
| `experiment` command | ✅ Implemented |
| Prerequisite mana-cost inheritance | ✅ Implemented |

---

### A-18 · Emotion Configuration System (`cedit`)

**Documentation:** `md-docs/CEDIT_EMOTION_CONFIGURATION_GUIDE.md`, `md-docs/EMOTION_CONFIG_SYSTEM.md`,
`md-docs/EMOTION_PRESETS.md`  
**Confirmed in:** `src/cedit.c`, `src/structs.h`  
**News reference:** Oct/Nov 2025 — *"Configuração de emoções agora disponível em CEDIT com presets (Mercantil, Eremita, Leal)."*

| Component | Code Status |
|-----------|-------------|
| `cedit_disp_emotion_menu()` | ✅ Implemented |
| Per-emotion display thresholds | ✅ Implemented |
| Emotion presets (Mercantil, Eremita, Leal) | ✅ Implemented |
| All config flags exposed in `structs.h` | ✅ Implemented |

---

### A-19 · QP ↔ Gold Exchange System

**Documentation:** (referenced in `lib/text/news` only — no dedicated `.md` file exists)  
**Confirmed in:** `lib/text/news` Dec 2025 — *"Novo sistema de troca de ouro por pontos de quest implementado. Comandos rate e cotacao mostram taxas de câmbio atuais."*

| Component | Code Status |
|-----------|-------------|
| `rate` / `cotacao` commands | ✅ Implemented |
| `show stats` QP/gold exchange info | ✅ Implemented |
| Dynamic exchange rate (monthly, overflow-protected) | ✅ Implemented |

**Documentation status:** ✅ **Resolved** — `md-docs/QP_EXCHANGE_SYSTEM.md` created (Feb 28 2026) covering commands, dynamic rate algorithm, persistence, overflow protection, and administration notes.

---

### A-20 · Danger Sense / Death-Trap Protection

**Documentation:** (referenced in `lib/text/news` only — no dedicated `.md` file exists)  
**Confirmed in:** `lib/text/news` Dec 2025 — *"Habilidade danger sense agora protege Ladroes de armadilhas mortais ao fugir."*

**Documentation status:** ✅ **Resolved** — `md-docs/DANGER_SENSE_FEATURE.md` created (Feb 28 2026) covering passive warning, flee prevention, skill levels, NPC behaviour, and builder notes for death-trap room configuration.

---

## <a name="b"></a>Category B – Partially Implemented

Features that are **structurally present in source code but incomplete, disabled, or
awaiting content** to be fully functional.

---

### B-1 · Rebegin / Remort Feature

**Documentation:** `md-docs/REBEGIN_FEATURE.md`, `md-docs/MENU_QUIT_REBEGIN_FIX.md`  
**News reference:** Oct/Nov 2025 — *"Sistema de Rebegin: Lógica de seleção de classe no rebegin corrigida."*

**What is implemented:**
- Full state machine (`CON_RB_SKILL` → `CON_RB_NEW_CLASS` → `CON_RB_REROLL` →
  `CON_RB_QATTRS` → `CON_RB_QHOMETOWN`) in `src/act.other.c`
- `do_rebegin()` and `can_rebegin()` functions
- `retained_skills[]` and `was_class[]` arrays persisted in player file
- Rebegin rules menu shown when `can_rebegin()` is true

**What is missing:**
- The `rebegin` command is **commented out** in `src/interpreter.c` — the feature is
  intentionally disabled, pending final balance sign-off
- The text file `lib/text/rebegin` (in-game rules explanation) needs to be verified
- End-to-end playtesting of the full remort flow

**Status:** Code complete, feature disabled by design decision. Requires game-design
approval and playtesting before enabling.

---

### A-7b · Big Five (OCEAN) – Phase 3: Extraversion & Agreeableness

**Documentation:** `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md`

**Confirmed in:** `src/mobact.c`, `src/utils.c`, `src/shadow_timeline.c`, `src/sec.h`

| Component | Code Status |
|-----------|-------------|
| Extraversion social-frequency modifier (`mob_emotion_activity()`) | ✅ Implemented |
| Extraversion social reward gain (`mob_contextual_social()`) | ✅ Implemented (Phase 3) |
| Agreeableness anger-gain damping (`update_mob_emotion_attacked()`) | ✅ Implemented |
| Agreeableness forgiveness decay (`decay_mob_emotions()`) | ✅ Implemented |
| Agreeableness aggression damping on initiation (`mobile_activity()`) | ✅ Implemented (Phase 3) |
| Agreeableness group cooperation bonus (`mob_handle_grouping()`) | ✅ Implemented (Phase 3) |
| A/E Shadow Timeline utility scoring (`shadow_timeline.c`) | ✅ Implemented |
| SEC constants: `SEC_E_SOCIAL_REWARD_SCALE`, `SEC_A_AGGR_SCALE`, `SEC_A_GROUP_SCALE` | ✅ Added |

**Documentation status:** `BIG_FIVE_PERSONALITY_SYSTEM.md` updated to v1.3 (March 2026), Phase 3 marked ✅ Active.

---

### A-7c · Big Five (OCEAN) – Phase 4: Openness

**Documentation:** `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md`

**Confirmed in:** `src/shadow_timeline.c`, `src/sec.h`

| Component | Code Status |
|-----------|-------------|
| MOVE exploration weighting (novelty bonus/penalty) | ✅ Implemented in Shadow Timeline |
| Depth-aware novelty bonus for non-repeated action types | ✅ Implemented in Shadow Timeline |
| Routine preference bonus for low-O repeated actions | ✅ Implemented in Shadow Timeline |
| Threat-amplification bias reduction | ✅ Implemented in Shadow Timeline |
| Direct wandering/room-novelty drive in `mobact.c` | ⏳ Deferred |

**Documentation status:** Phase 4 correctly noted as partially active in `BIG_FIVE_PERSONALITY_SYSTEM.md`.

---

**Documentation:** None (only `TODO` root file mentions it)  
**Source:** `src/clan.c` exists but contains only ~10 lines of header boilerplate;
`src/clan.h` declares structures referenced elsewhere in `structs.h`

**What is implemented:**
- File scaffolding and header declarations

**What is missing:**
- All clan commands (`clan create`, `clan join`, `clan who`, `clan info`, `clan kick`, etc.)
- Clan data storage and persistence
- `WHO` integration (noted in `TODO`: *"Fix: Acertar o look and feel do CLAN WHO"*)
- Clan-based `plrlist` search (noted in `TODO`)

**Status:** Effectively a stub. This is the largest single pending feature in the codebase.

---

### B-4 · Hometown System

**Documentation:** (mentioned in root `TODO` only — *"Hometown em 113 - WIP"*)

**What is implemented:**
- `GET_HOMETOWN(ch)` macro and `hometown` field in player struct
- `char_to_room` uses `r_hometown_1..4` global room refs for recall
- Rebegin flow includes `CON_RB_QHOMETOWN` hometown selection
- Bhogavati added as hometown 4 (Oct/Nov 2025 news)
- Thewster available as hometown at character creation (Aug 2025 news)

**What is missing:**
- Menu-driven hometown selection at initial character creation (not rebegin) is incomplete
- Zone 113 (Vitalia city, the primary hometown) world content is work-in-progress
- `r_hometown_*` vnums may not all point to valid, finished rooms

**Status:** Functional for existing hometowns; zone 113 content is WIP.

---

### B-5 · Guild System (Zone 134)

**Documentation:** (mentioned in root `TODO` only — *"guildas em 134"*)

**What is implemented:**
- Class-based trainer NPCs exist (tbaMUD baseline functionality)
- Zone 134 exists in the world file set

**What is missing:**
- Dedicated guild hall rooms and thematic content in zone 134
- A coded guild-rank system or guild membership structure beyond basic trainer interaction

**Status:** Functional for training; lore/content side is incomplete.

---

### B-6 · Auction House — Experimental Phase

**Documentation:** (no `md-docs/` file; described in Oct 2025 news)  
**News reference:** Oct 2025 — *"Sistema completo de leilões com controle de acesso por convite. Casa de leilões secreta acessível através da loja do Belchior."*

**What is implemented:**
- `src/auction.c` and `src/act.auction.c` (player-driven auction commands)
- In-game commands: `leilao criar`, `leilao listar`, `leilao dar`, `leilao convidar`
- Belchior's auction house room connected to the game world
- CEDIT toggle for experimental access control
- Immortal-only access for testing (level 101+)

**What is missing:**
- The feature is flagged **experimental** in news; full player access not yet enabled
- ~~A dedicated `md-docs/` design document~~ ✅ Created: `md-docs/AUCTION_HOUSE.md` (Feb 28 2026)

**Status:** Functional but access-restricted. Needs stability validation before opening
to all players.

---

## <a name="c"></a>Category C – Genuinely Pending (Planned but Not Yet Implemented)

These items appear in documentation as explicit TODOs, phase plans, or future
considerations, and **no confirming implementation was found in the source code**.

---

### C-1 · Communication-Based Emotion Triggers

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §2.1`

Planned triggers not yet implemented:
- NPC hears an insult/curse directed at them → anger and shame update
- NPC receives praise or thanks → happiness and pride update
- Player walks away mid-conversation → sadness and/or anger
- NPC receives an unexpected `tell` from a stranger → curiosity or suspicion

**Relevant source files:** `src/act.comm.c` (say/tell/whisper handlers), `src/utils.c`
(emotion update functions)

---

### C-2 · Reputation → Emotion Initialization at First Meeting

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §3.2`

Planned behavior not yet implemented:
- Player reputation seeds mob emotion baseline at first encounter
  (high reputation → higher initial trust/friendship; low reputation → higher suspicion/fear)
- Emotion levels feed back into reputation gains/losses

**Relevant source files:** `src/utils.c` (emotion initialization), `src/players.c`
(first-meeting reputation-to-emotion seeding)

---

### C-3 · Emotion Visual Cues in `look` and Mob Descriptions

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §4.1`

Planned features not yet implemented:
- Emotion-state appended to mob long description when looking (e.g.,
  *"o goblin treme de medo"* / *"the goblin trembles with fear"*)
- Color-coded emotional state indicators
- Periodic spontaneous emotion emotes from mobs with extreme states

**Relevant source files:** `src/act.informative.c` (`do_look`), `src/utils.c`

---

### C-4 · NPC Emotion-Driven Dialogue

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §4.2`

Planned features not yet implemented:
- NPC speech vocabulary shifts based on emotional state (aggressive when angry,
  withdrawn when sad, enthusiastic when happy)
- High emotion levels trigger spontaneous statements into the room
- Quest and shop dialogues adapt to the NPC's current emotional state toward the player

**Relevant source files:** `src/act.comm.c`, `src/mobact.c`, `src/shop.c`

---

### C-5 · Emotion-Based Player Skills

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §5.4`

Planned skills not yet implemented:
- **Intimidation** — actively increases a target NPC's fear
- **Charm** — increases a target NPC's happiness/love
- **Manipulation** — exploits an NPC's current dominant emotion to gain advantage
- **Empathy** — read-only reveal of a mob's current emotional state toward the player

**Relevant source files:** `src/class.c`, `src/act.offensive.c` or `src/spells.c`,
`src/utils.c`

---

### C-6 · Emotion Decay Rate and Tuning Configuration

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §6`

Planned tuning options not yet implemented:
- Per-emotion configurable decay rates via CEDIT
- Configurable multiplier for extreme-emotion decay
- Per-mob emotional sensitivity override
- Emotion calculation performance profiling / optional caching

**Relevant source files:** `src/cedit.c`, `src/structs.h`, `src/utils.c`

---

### C-7 · Alignment ↔ Emotion Integration

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §8.1`

Planned integration not yet implemented:
- Persistent high anger/hatred slowly nudges mob alignment toward evil over time
- Persistent high compassion/love slowly nudges toward good
- A mob's alignment affects its emotional baseline values (evil mobs start with more anger)

**Note:** `CONFIG_EMOTION_ALIGNMENT_SHIFTS` config flag exists in the code but is not yet
wired to any periodic alignment-shift logic.

**Relevant source files:** `src/utils.c` or `src/limits.c` (tick-based processing),
`src/config.c`

---

### C-8 · Faction and Deity/Religion Emotion Integration

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §8.2-8.3`

Planned integration not yet implemented:
- Faction enemies start with predefined negative emotions toward each other
- Religious mob types have modified emotional response profiles
- Prayers and divine blessings affect mob emotional state
- Divine hostility/opposition triggers appropriate emotional responses

**Dependency note:** This item depends on a faction system that does not yet exist beyond
basic alignment. This is a long-term architectural item requiring faction design first.

---

### C-9 · Emotional Immunity (Undead, Constructs, Psychopaths)

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §9.2`

Planned features not yet implemented:
- A mob flag (e.g., `MOB_IMMUNE_EMOTION`) to exempt certain mobs from emotion updates
- Undead and constructs should not accumulate emotional state (lore inconsistency today)
- A "psychopath" emotional profile preset with reduced empathic emotion responses

**Relevant source files:** `src/structs.h` (mob flags), `src/utils.c` (emotion update
guard check)

---

### C-10 · Player Emotion Visibility Tools

**Source:** `md-docs/EMOTION_SYSTEM_TODO.md §7`

Planned player-facing and builder tools not yet implemented:
- In-game help file (`HELP EMOCOES` or `HELP EMOTIONS`) explaining the system to players
- A player skill or command to sense a mob's emotional state (`sense_motive`/`empatia`)
- Builder documentation for designing emotion-aware quests
- Immortal/wizard command to manually set a mob's emotion value for testing

**Relevant source files:** `lib/text/help` (help entries), `src/act.wizard.c`,
`src/interpreter.c`

---

### ~~C-11~~ · Big Five Phase 3 (Extraversion / Agreeableness) and Phase 4 (Openness) — **RESOLVED**

**Source:** `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md §Future Phases`

**Audit finding (February 2026):** The original B-2/C-11 entries incorrectly described
Phase 3 and Phase 4 as "struct only, no behavior." A code review (March 2026) found that
significant behavioral code was already present:

- `src/utils.c` — Agreeableness anger-gain damping and forgiveness-decay acceleration
- `src/shadow_timeline.c` — A/E utility scoring for ATTACK/SOCIAL/FOLLOW/GROUP actions; O exploration weighting, novelty/repetition bonuses
- `src/mobact.c` — Extraversion social-frequency modifier (`mob_emotion_activity()`)

**Phase 3 behavioral code added (March 2026):**
- Extraversion social reward gain in `mob_contextual_social()` — happiness reward after positive socials, proportional to E_final
- Agreeableness aggression initiation damping in `mobile_activity()` — reduces attack impulse_threshold for high-A mobs
- Agreeableness group cooperation bonus in `mob_handle_grouping()` — adjusts grouping chance by A_final

**Phase 4 status:** Active in Shadow Timeline. Direct `mobact.c` wandering drive deferred (low priority).

**Relevant source files:** `src/mobact.c`, `src/utils.c`, `src/shadow_timeline.c`, `src/sec.h`

---

### C-12 · Shadow Timeline Phase 3 Features

**Source:** `md-docs/SHADOW_TIMELINE.md §Future Extensions`

Planned extensions not yet implemented:
- Causal Ledger integration (record actual past outcomes to improve future predictions)
- Temporal Authority Layer hooks (decide which projected action becomes real-world state)
- Player-accessible foresight mechanics (e.g., precognition spell interaction)
- Multi-step planning with action dependency chains
- Collaborative projections (group decision-making via shared cognitive space)
- Confidence-based precision weighting for projection scoring

---

### C-13 · FANN Neural Network Integration

**Source:** `md-docs/RFC-1001_NPC_PSYCHOLOGY_CHECKLIST.md` (Executive Summary)

FANN (Fast Artificial Neural Network Library) headers and library are present in the
project but no mob decision-making code currently uses them.

> ⚠️ "FANN neural networks included but not integrated"

If neural-network-driven behavior is desired, a feasibility study and integration layer
connecting FANN output to mob action selection in the Shadow Timeline would be required
before any implementation.

---

### C-14 · Monk and Paladin Classes

**Source:** Root `TODO` — *"classes monge e paladinos"*

No `CLASS_MONK` or `CLASS_PALADIN` constants or class entries were found in the source
code. These are entirely absent from the class system.

**Scope of work:**
- Class table entries and constants in `src/structs.h` and `src/class.c`
- Skill and spell assignments in `src/spells_assign.c`
- Spell parser integration in `src/spell_parser.c`
- Trainer NPC content in appropriate world zones
- Balance design for class abilities

---

### C-15 · Climb Skill for Warriors and Thieves

**Source:** Root `TODO` — *"Imp: Climb (guerreiros e ladrões)"*

`SECT_CLIMBING` exists as a sector type (defined in sector constants, used in ASCII map
and weather movement modifiers) but there is no `climb` skill or `do_climb` command.
Players can currently enter climbing-sector rooms by paying extra movement points, with
no skill check.

**Scope of work:**
- Add `SKILL_CLIMB` to the warrior and thief skill tables
- Add a skill check when attempting to enter `SECT_CLIMBING` rooms
- Fail state (player cannot enter, or falls and takes damage)

---

### C-16 · Object Layering System

**Source:** Root `TODO` — *"Sobreposição de objetos (Comando PUT OVER, flags NOUNDER, NOOVER, NOHIDE)"* and *"Mover objetos (Comando PUSH, flag MOVEABLE)"*

No `NOUNDER`, `NOOVER`, or `NOHIDE` item flags exist. No `push` or `put over` commands
exist in `src/act.item.c`.

**Scope of work:**
- New item flags in `src/structs.h`
- `do_push` command (move a MOVEABLE object to an adjacent room)
- `do_put_over` command (layer one object on top of another)
- World-builder documentation for the new flags

---

### C-17 · Vitalia Banking System

**Source:** Root `TODO` — *"Imp: Informatizar o banco de Vitalia"*

A basic bank special procedure exists from the tbaMUD baseline (deposit/withdraw) but a
full "Banco de Vitália" with account statements, interest accrual, and a dedicated
player-facing interface is planned but not implemented.

**Note:** Oct/Nov 2025 news confirms *"Sistema de bancos agora funciona corretamente após
mudanças de nível"* — meaning the basic bank was fixed, not that a full banking system was
added.

---

### C-18 · Miscellaneous Open Bugs and QoL Features (from root `TODO`)

| Item | Category | Description |
|------|----------|-------------|
| Gas pão CRASH | Bug | Specific item causes server crash — root cause not yet identified |
| Gold save in corpses | Bug | Player gold not correctly persisted inside PC corpses |
| REPLY forgotten on quit | Bug | `last_tell` target not cleared on `quit`, causes stale reply on reconnect |
| WHERE / STAT OBJ show invis gods | Bug | Visibility checks missing for immortals with `INVIS` flag |
| FLOOD control | Feature | Anti-flood command throttling for players sending rapid input |
| Improved `plrlist` | Feature | Search by clan, level ranges, configurable output format |
| `pk_allowed` / `pk_flag` audit | Fix | PK flag inconsistency between permission and actual flag checks |
| Immortal HP display | Fix | HP display for immortal-level characters shows incorrect values |
| AFF flags wizard command | Feature | Direct wizard command to toggle individual AFF flags on a character |
| Color profiles | Feature | Per-player selectable color scheme (themes) |
| Default wiznet level | Feature | Configurable default wiznet channel visibility level |
| WHO clan formatting | Fix | `clan who` appearance does not match `who` formatting style |
| Two-handed modifier check | Fix | Inventory modifier not applied correctly for two-handed weapons |
| `talk_dead` on move | Feature/Fix | Dead mobs interacting or emoting during movement tick |
| Object-level restriction (2nd enchantment) | Feature | Objects that can only be worn after 2+ enchantments applied |
| `consider` command improvements | Feature | Richer, more informative `consider` command output |

---

## <a name="d"></a>Category D – World-Content Backlog

These are **world-building tasks** (not source code features) tracked in the root `TODO`
file. They require builder work in zone and world files, not code changes.

| Item | Description |
|------|-------------|
| Mobs revistos até zon 126 | Accent/encoding review of mob names and descriptions in listed zones (0–290) |
| Areas acentuadas (listed zones) | Encoding fix for room descriptions in the extensive list of zones |
| Objs acentuados (listed zones) | Encoding fix for object names and descriptions in listed zones |
| Lojas (shops) | Complete shop configuration for zones not yet in the *lojas feitas* list |
| Mob sem load (vnums 750, 2399) | Two mobs are defined but never placed in any zone reset command |
| Falta nome obst em 111 | Missing exit/obstacle names in zone 111 |
| Porta 11427 west hidden pickproof | Door attributes need correction |
| Typo 4048 (east exit description) | *"You can head toward the narrow trail this way."* — language inconsistency in a Portuguese zone |
| 13436 desc undefined | Room 13436 missing a description |
| completar objs 65 | Objects in zone 65 (STRANGELOVE ORIG) need completion |
| Guildas em 134 | Guild hall rooms in zone 134 (also see B-5) |
| Falta loja catedral de cristal | Shop missing from the Crystal Cathedral location |
| Falta loja mercador nômade nt (zone 54) | Nomadic merchant shop missing from zone 54 |
| Falta 221 & para @ | Zone 221 encoding issue with special characters |

---

## <a name="e"></a>Category E – Player-Reported Ideas (`lib/misc/ideas`)

These suggestions were submitted by players via the in-game `idea` command and are stored
in `lib/misc/ideas`. They are organized by implementation status.

---

### E-1 · Ideas That Overlap With Already-Implemented Features

| Player | Idea | Overlap |
|--------|------|---------|
| Motharo | Skill `aware of danger`/`sentir perigo` — show trap direction while moving, no armour penalty while sleeping | **Direction indicator now implemented** in `check_danger_sense()` at skill ≥ 75%: shows exact direction(s) of death trap; at skill < 75%: shows vague "PERIGO mortal por perto" warning. The *sleep armour penalty removal* aspect is **not yet implemented**. |
| Cansian | Aspas (single quotes) only required for multi-word spell names, not single words | Relates to Voice Casting (A-12). The current behaviour requires quotes universally; relaxing this rule is a UX enhancement not yet made. |

---

### E-2 · Ideas Not Yet Implemented (New Features Requested by Players)

| # | Player | Idea | Notes |
|---|--------|------|-------|
| E-2.1 | Cansian | **Skill `cooking`** — combine foods to increase satiation | New skill; no implementation found. |
| E-2.2 | Yazid | **Comma-separated item keywords** in buy/get commands (e.g., `buy pao,via`) | Legacy behaviour; no longer working. Could be a parser enhancement. |
| E-2.3 | Yazid | **Message buffer** while inside `idea`/`typo`/`mail` input mode | Quality of life; no queued-message system while in text editors. |
| E-2.4 | Yazid | **Direction aliases on exits** — allow `open norte` in addition to `open porta` | Adds direction names as keywords to doors/exits automatically. |
| E-2.5 | Roscoe | **Static item effects in `affects`** — show always-on bonuses from worn items (e.g., `santuario [item]`) | Cosmetic enhancement to the `affects` command output. |
| E-2.6 | Kasper | **`Emaranhar` (Entangle) spell for Druids** — roots target's feet (similar to Paralyze) | New spell design; not implemented. |
| E-2.7 | Panoramix | **Auto-arrow equip option** — automatically equip arrows from inventory during combat | QoL toggle for Rangers/archers. |
| E-2.8 | Lupulis | **`locate object` improvement** — show 1 result per level instead of level÷2 | Balance change to the Locate Object spell. |
| E-2.9 | Cansian | **`SPELL SILENCE`** — should block `cast` and communication channels (`say`, `gossip`, `shout`, etc.) | Enhancement to an existing silence spell effect. |
| E-2.10 | Lupulis | **Item rebalance: `argolas douradas do ancião`** — stats inferior to lower-level items from the same zone | World-content balance issue; affects loot design of zone 99 (Dragão Dourado). |
| E-2.11 | Laguna | **`rskills` command** — list skills and spells gained through rebegin/remort, showing class and incarnation | New command; not implemented. Useful once Rebegin (B-1) is enabled. |
| E-2.12 | Henzo | **Ranger camping skill** — while sleeping in a campsite the room becomes a fast-recovery zone | New skill design. |
| E-2.13 | Durandal | **Card album / stash for collectible cards** — dedicated storage (house bookshelf) for `ITEM_CARDS`, separate from inventory | Requires house-system and ITEM_CARDS integration. |
| E-2.14 | Panoramix | **MV increase based on DEX** — movement point maximum scaled by Dexterity (and possibly Constitution) | Balance change; addresses Ranger/Archer MV shortfall. |
| E-2.15 | Durandal | **Mana density in prompt** — show mana density level (e.g., `MB`/`M`/`A`/`MA` or a percentage) after `Hp Mn Mv` in the prompt line | QoL for spellcasters; reduces visual noise compared to the current room-description message. |
| E-2.16 | Kasper | **Druid Metamorphosis** — assume animal or elemental forms using existing mob prototypes; each form grants stat bonuses and changes attack type; spellcasting blocked while transformed | New major skill/system design. Technically proposed to reuse mob VNUM prototypes for the transformation stats. |
| E-2.17 | Kasper | **Equipment Set System** — equipping 2, 3, or a full set of thematically related items grants cumulative passive bonuses; sets discoverable through `identify` | New item-system feature requiring set-tag data in object files. |
| E-2.18 | Kasper | **Nova Classe Monge** — detailed Yin/Yang duality design with full skill tree (see `lib/misc/ideas` for complete proposal) | Overlaps with C-14 (Monk class). This player submission provides a detailed design blueprint that should be consulted when implementing the Monk class. |
| E-2.19 | Laguna | **Stoneskin points in `affects` output** — show remaining points alongside duration (e.g., `stoneskin [84 hs / 118pts]`) | Cosmetic enhancement; `score` already shows points but `affects` does not. |
| E-2.20 | Lupulis | **`a Flecha de Sagitário` as a quiver-slot item** — allow this specific arrow to be worn as equipment with defined stats rather than fired as ammunition | World-content and item-type design question. |

---

## <a name="f"></a>Category F – Player-Reported Bugs (`lib/misc/bugs`)

Bugs submitted via the in-game `bug` command and stored in `lib/misc/bugs`.
Each entry is cross-referenced with `lib/text/news` and `changelog.txt` to estimate
whether it has already been resolved.

---

### F-1 · Bugs Likely Already Fixed

These bugs were reported before known fix windows and match descriptions in the news/changelog.

| Bug Report | Reporter | Likely Fix Reference |
|------------|----------|----------------------|
| `away vazado` — colour bleeding in the `away` status display | Cansian | News Oct/Nov 2025: *"Sangramento de cores em tags de status... corrigido"* |
| `stoneskin` overflow — infinite stoneskin with negative point count and 9999+ hours after multiple casts | Laguna | News Oct 2025: *"Corrigido bug crítico onde Stoneskin não protegia... Sistema de duração/pontos agora reduz proporcionalmente"* |
| `Paralyze + Wimp` — paralysed mobs could still flee via `wimpy_tendency` | Cansian | News Oct 2025: *"Magia Paralyze agora impede corretamente a fuga de mobs e jogadores"* |
| `fly` — `fly` command does not lift off even when `fly` affect is active | Cansian | News Oct 2025: *"Magia Fly agora requer ser conjurada novamente após pousar com land"* (related; confirms fly system rework) |
| `mensagem de ress` — gender mismatch and colour bleed in resurrection message | Anyte | News Oct/Nov 2025: colour-bleed fixes |
| `remort e transcender` — player can fight and deal/receive no damage during transcendence state | Laguna | News Oct/Nov 2025: *"Sistema de remoção de efeitos no rebegin corrigido"* (related fix) |
| `habilidades de remorts erradas` — wrong skills/spells ported after remort (vampiric touch instead of fireshield) | Astus / Yazid | News Oct/Nov 2025: *"Habilidades do remort agora são mantidas corretamente"* |
| `pocao de Cura` casting `invigorate` instead of cure spell | Roscoe | Likely fixed during spell-assignment corrections; needs verification |
| `word of recall` sending player to Puff's room instead of recall room | Anyte | Old report (2021); likely fixed with room recall system updates |
| `sem cor após título colorido` — rest of room description loses colour after a coloured title | Yazid | News Oct/Nov 2025: *"Sangramento de cores... corrigido"* |

---

### F-2 · Bugs Still Open (Not Matched to a Known Fix)

These reports have no corresponding fix identified in the news or changelog. They should
be triaged and prioritised.

| # | Bug Report | Reporter | Room | Notes |
|---|------------|----------|------|-------|
| F-2.1 | `practice` — message says "only at trainer" but practice succeeds anyway | Althair | 18601 | Contradictory validation logic |
| F-2.2 | `xp cap` — XP gain has no per-level cap | Cansian | 3001 | May be intentional or fixed; needs verification |
| F-2.3 | `autoequip` — earrings (`brincos`), face (`rosto`), and wings (`asas`) slots not included | Cansian | 3001 | Auto-equip missing certain worn-slot types |
| F-2.4 | `aid` — AID spell did not increase `maxhit` | Cansian | 3054 | Status unclear after multiple AID-related fixes |
| F-2.5 | `pick` — pick lock should only be usable by Thieves | Cansian | 3469 | Class restriction missing or incomplete |
| F-2.6 | `kanakajja` — NPC special trigger not implemented | Cansian | 11642 | World content / DG script |
| F-2.7 | `item trap` — trap item not functional | Cansian | 11581 | World content |
| F-2.8 | `sala das palavras` — puzzle room east exit should appear after being opened | Anyte | 17853 | World / trigger content |
| F-2.9 | `quadro sem cores` — Mid's noticeboard not accepting colour codes | Anyte | 3000 | Board or ANSI parser issue |
| F-2.10 | `portas e armário em Mordecai` — doors in Mordecai's house cannot be opened; neither key works | Anyte | 10052 | World content — key/lock vnum mismatch |
| F-2.11 | `Bilhete do Museu` — museum ticket does not disappear; Golem trigger not picking it up | Cansian | 3801 | World / trigger content |
| F-2.12 | `Fly` — mob (Lupulis) re-lands immediately after landing (`fly de novo sozinho`) | Cansian | 3001 | Possible AI or affect reapplication loop |
| F-2.13 | `retorno pro templo após quit` — player who quit in the Abyss was not returned to temple after ~72 hours | Yazid | 9752 | Loadroom / respawn logic |
| F-2.14 | `magia errada quando fez o port dos remorts` — `skin like steel` replaced by `skin like diamond` after remort port | Yazid | 3019 | Spell-assignment mismatch during rebegin |
| F-2.15 | `transport via plants impossível de praticar` — spell listed for Druid/Mage in help but not in practice list | Yazid / Astus | 3019 | Spell-assignment / class table issue |
| F-2.16 | **Portas com Palavras** — no word-triggered doors work anywhere (Fênix na China, Fenda, Gondolin, etc.) | Lupulis | 5429 | **Fixed** — triggers now attached to rooms; flag values corrected (`ao`/`aop`); same-keyword pairs merged (zones 69, 114) |
| F-2.17 | `Sala Shogum Tei` — `up` exit cannot be unlocked even with the correct amulet | Laguna | 15132 | World content — lock/key vnum |
| F-2.18 | `Fenda Entre Mundos` — password `avarohana` no longer opens the `escuridão` door to Yama | Laguna / Astus | 11417 | **Fixed** — trigger 11400 attached to room 11417; merged with 11401; flag `aco`→open `ac` |
| F-2.19 | `Rua Luar de Safira, casas 3 e 4` — house door cannot be opened | Lupulis | 1021 | World content |
| F-2.20 | `Sala "Muitos Caminhos" – Loctus` — stone door cannot be opened with the stone key | Laguna | 8310 | World content — lock/key vnum |
| F-2.21 | `Polearms` — no class currently learns the `polearms` weapon skill | Laguna | 10984 | Skill not assigned to any class table |
| F-2.22 | `AID (PLUS IUM)` — enhanced AID decreased max HP instead of increasing it | Henzo | 3001 | AID spell variant interaction bug |
| F-2.23 | `flee` — flee failing far too often in combat | Panoramix | 2 | Flee formula or random roll calibration |
| F-2.24 | `taxa de práticas por nível × Wisdom` — practice gains per level do not scale with WIS attribute | Roscoe | 3001 | Practice formula not using WIS stat |
| F-2.25 | `Magia não aparecendo no affect` — debuff spells (curse, blind, sleep from potions) not shown in `affects` | Durandal | 29041 | Missing affect-tracking for certain spell effects |
| F-2.26 | `Porto de Madeira – Gondolin` — gate cannot be unlocked even with the sentinel key | Laguna | 4520 | **Fixed** — triggers 4510-4513 attached to room 4520; flag values corrected |
| F-2.27 | `Porta da Fenda entre Mundos` (repeated) — `avarohana` no longer opens the door | Astus | 3001 | **Fixed** — same as F-2.18 |
| F-2.28 | `Visible/loja` — `visible` effect wears off between shop transactions, forcing repeated casting | Durandal | 3020 | Affect-duration / shop-interaction issue |
| F-2.29 | `erro lvl 60 comando level` — `level` command shows identical XP requirement for 59→60 as for 58→59 | Thorgal | 3001 | XP table display bug in `do_level` |
| F-2.30 | `A balada do Andarilho ignora a mana zerada` — song continues working with 0 mana | Panoramix | 12058 | Mana-check missing in bard song loop |
| F-2.31 | `Cajado de pedra do Rizzo` — wand sold by Rizzo changed spell from Stoneskin to another spell | Astus | 3001 | World content — object vnum or spell field |
| F-2.32 | `incremento de dano sentado` — bash/fury of air does not increase damage on sitting targets | Henzo | 3001 | Combat position modifier not applying |
| F-2.33 | `who` — `who` sub-commands broken; `who -s` reports ~3× actual player count | Henzo | 1061 | `do_who` command logic |
| F-2.34 | `toggle wimp` — wimpy toggle changes page length instead of wimpy flee threshold | Henzo | 1609 | Preference editor toggle index collision |
| F-2.35 | `harm` — spell dealing ~20 damage (common and minus versions) instead of stated values; plus version works | Henzo | 15074 | Spell formula for `harm` base/minus variants |
| F-2.36 | `aid` (voice cast) — AID spell via syllables does not work in the normal version; PLUS and MINUS variants work | Henzo | 1626 | Spell-variant syllable mapping |
| F-2.37 | `O GRITO DA MORTE` — bard song not working | Henzo | 1603 | Song implementation or trigger issue |
| F-2.38 | `voice explosion` — spell does not display damage messages or effects on enemies | Henzo | 1627 | Message output for this spell variant |
| F-2.39 | `Windwall` — not reflecting/returning damage to attackers | Astus | 12670 | Aura-shield damage reflection (possibly related to aura fixes in Dec 2025) |
| F-2.40 | `disintegrate` — does not disintegrate corpses as expected | Henzo | 12769 | Spell effect on corpse objects |
| F-2.41 | `Magia Transport via Plants` — cannot be practised despite being listed in HELP for Mage | Astus | 13798 | Spell-class assignment or availability table |
| F-2.42 | `invigor` — spell missing self-cast option | Henzo | 10713 | Missing `FIND_CHAR_ROOM` self-target flag |
| F-2.43 | `eval para fireweapon (arcos)` — evaluate and fire-weapon interaction with bows needs review | Cansian | 3054 | Item evaluation for bow-type weapons |
| F-2.44 | `old thalos` — undefined object on floor in Asa Sul da Prefeitura (zone 52) | Henzo | 5231 | World content — object missing description |
| F-2.45 | `Pessegueiro bugado` (room 14060) | Lupulis | 14060 | World content — specific peach-tree interaction |
| F-2.46 | Multiple `porta aqui não abre` reports (rooms 766, 13872, 13879, 13888, 14046, 4574, 757) | Lupulis | Various | World content — door/lock vnum mismatches across multiple zones |
| F-2.47 | `Fome e sede` — no warning tick before hunger/thirst penalty; penalty fires immediately | Lupulis | 1016 | Missing one-tick warning in hunger/thirst loop |
| F-2.48 | `caminho SW` bugado (room 14480) | Lupulis | 14480 | World content — broken exit |

---

### F-3 · Bug Reports Requiring Verification (Status Ambiguous)

These overlap with known fix windows but it is unclear if they were fully resolved.

| Bug Report | Reporter | Why Ambiguous |
|------------|----------|---------------|
| `stoneskin` + `fireshield` stacking — fireshield passes damage through even when stoneskin negates the hit | Laguna | Stoneskin was fixed, but the fireshield interaction is a separate case not explicitly mentioned in news |
| `Fly` — Lupulis mob re-applies fly affect automatically after landing | Cansian | Fly system reworked but auto-reapplication edge case unclear |
| `mensagem de ress` — pronoun uses wrong gender suffix | Anyte | Colour bleed fixed; gender-suffix may still be wrong |

---

## <a name="g"></a>Category G – Player-Reported Typos (`lib/misc/typos`)

Typos submitted via the in-game `typo` command and stored in `lib/misc/typos`.

| # | Description | Location | Current Text | Correct Text |
|---|-------------|----------|--------------|--------------|
| G-1 | Gender suffix in fly-end message | Room 3001 | `"Você se sente pesad$R novamente."` | Replace `$R` with the correct gender macro (e.g., `$o`/`$a`) |
| G-2 | Extra space before title in room description | Room 3001 | `"Althair  o Raposinha está em pé aqui."` (double space) | Single space between name and title |
| G-3 | Untranslated English in `create berries` message | Room 3054 | `"You create um punhado de morangos silvestres."` | `"Você cria um punhado de morangos silvestres."` |
| G-4 | Gender mismatch in secret passage message | Room 13125 | `"secreta parece estar fechado."` | `"A passagem secreta parece estar fechada."` |
| G-5 | Gender mismatch in stone door message | Room 13128 | `"pedra parece estar fechado."` | `"A porta feita de pedras parece estar fechada."` |
| G-6 | Mob name typo in combat message | Room 15129 | `"Você fura o Shogum da Verade."` | `"Você fura o Shogun da Verdade."` |
| G-7 | Typo in nomadic merchant telepathy message | Room 5462 | `"Saia daqui antes quw eu chame OS GUARDAS!"` | `"Saia daqui antes que eu chame os guardas!"` |

---

## <a name="inaccuracies"></a>Documentation Inaccuracies Found

This section summarizes specific files where the documented status does not match the
current implementation state. These are documentation corrections to be made, not code changes.

| Document | Original (Incorrect) Text | Correct Status | Resolution |
|----------|--------------------------|----------------|------------|
| `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md` | `⏳ Phase 2: Conscientiousness` (pending) | ✅ Phase 2 fully implemented | ✅ Fixed Feb 28 2026 |
| `md-docs/BIG_FIVE_PERSONALITY_SYSTEM.md` | Changelog section missing Phase 2 entry | Add Phase 2 completion entry (Feb 2026) | ✅ Fixed Feb 28 2026 |
| `changelog.txt` | No entry for Phase 2 Conscientiousness | Add `[Feb 28 2026]` entry | ✅ Fixed Feb 28 2026 |
| `md-docs/SHADOW_TIMELINE.md` | Some sections still read "Implementation Plan" | Update Phase 1 and Phase 2 status headers to ✅ Production | ✅ Already correct — no change needed |
| `md-docs/EMOTION_CONTAGION.md` | May describe contagion as "planned future" | Update to ✅ Implemented (confirmed in `src/mobact.c:577`) | ✅ Fixed Feb 28 2026 |
| `md-docs/EMOTION_SYSTEM_TODO.md` | Phase 3/4 roadmap bullets list already-done items as future | Reconcile inline `**DONE**` markers with surrounding roadmap status | ✅ Fixed Feb 28 2026 |
| `README.md` | Refers to `HYBRID_EMOTION_SYSTEM.md` for Emotion System docs | Reference should also point to `EMOTION_SYSTEM_TODO.md` and updated status | ⏳ Pending |
| No `md-docs/` file | QP ↔ Gold Exchange System (A-19) has no design doc | Create `md-docs/QP_EXCHANGE_SYSTEM.md` | ✅ Created Feb 28 2026 |
| No `md-docs/` file | Danger Sense / Death-Trap Protection (A-20) has no design doc | Create `md-docs/DANGER_SENSE_FEATURE.md` | ✅ Created Feb 28 2026 |
| No `md-docs/` file | Auction House experimental phase (B-6) has no design doc | Create `md-docs/AUCTION_HOUSE.md` | ✅ Created Feb 28 2026 |

---

## <a name="plan"></a>Proposed Plan of Action

> **Note:** This plan describes documentation and design goals. It does not prescribe
> specific code changes — those are the responsibility of the development team in
> subsequent work sessions.

---

### Priority 1 – Documentation Corrections (No Code Required)

These are pure documentation tasks and should be completed first as they correct misleading
information that could confuse developers.

1. ✅ **`BIG_FIVE_PERSONALITY_SYSTEM.md`**: Phase 2 (Conscientiousness) marked ✅ complete.
   Phase 2 changelog entry added. (Feb 28 2026)

2. ✅ **`changelog.txt`**: Phase 2 Conscientiousness entry appended as `[Feb 28 2026]`.
   (Feb 28 2026)

3. ✅ **`SHADOW_TIMELINE.md`**: Verified — Phase 1 and Phase 2 section headers already carry
   ✅ "Implemented" markers. "Future Extensions (Phase 3)" section is accurate. No change needed.

4. ✅ **`EMOTION_CONTAGION.md`**: Explicit "Status: ✅ Implemented" header block added.
   (Feb 28 2026)

5. ✅ **`EMOTION_SYSTEM_TODO.md`**: Phase roadmap reconciled — completed contagion and
   advanced-mechanics items moved to Phase 2 (COMPLETED); future-only items remain in
   Phase 3/4. (Feb 28 2026)

6. ✅ **`md-docs/QP_EXCHANGE_SYSTEM.md`**: Created — covers commands `rate`/`cotacao`,
   dynamic exchange rate algorithm, overflow protection, persistence, and admin notes.
   (Feb 28 2026)

7. ✅ **`md-docs/DANGER_SENSE_FEATURE.md`**: Created — covers passive warning, flee
   prevention, skill levels, NPC behaviour, and builder notes on death-trap configuration.
   (Feb 28 2026)

8. ✅ **`md-docs/AUCTION_HOUSE.md`**: Created — covers access via Belchior's shop,
   available commands, CEDIT toggle, access restrictions, and path to full player access.
   (Feb 28 2026)

9. ✅ **`HYBRID_EMOTION_SYSTEM.md`**: New "4D Emotion Projection Component" section added
   describing `emotion_projection.c`, the 4-axis model, Contextual Modulation Layer, and
   API. (Feb 28 2026)

---

### Priority 2 – Feature Enablement (Code Decisions Required)

These items are code-complete or nearly so, requiring a design/balance decision before
enabling.

10. **Rebegin/Remort (B-1):** The implementation is complete but the command is disabled.
    The decision needed is: confirm game-balance requirements, verify `lib/text/rebegin`
    text exists and is accurate, and authorize enabling the command in the interpreter.

11. **Auction House full access (B-6):** Decide on the stability threshold required to
    lift the experimental flag and allow all players to participate.

---

### Priority 3 – Bug Triage and QoL Fixes (Short Term)

Ordered by severity and number of reporters. Address before adding new features.

12. **Verify F-1 fixes are complete:** The bugs in F-1 are believed fixed but should be
    explicitly confirmed in-game before closing them. Mark confirmed items in the `bugs`
    file with a resolution note.

13. **Critical open bugs** (multiple reporters or systemic):
    - **F-2.16 — Word-triggered doors** (Fênix, Fenda, Gondolin, Avarohana): Reported by
      multiple players across different zones. The keyword-triggered door system appears
      to be broadly broken. This is the highest-priority world/trigger bug.
    - **F-2.18 / F-2.27 — Fenda Entre Mundos** (`avarohana` door): Two independent
      reporters confirm the same door remains broken.
    - **F-2.33 — `who` command** returning 3× actual player count and sub-commands broken.
    - **F-2.34 — `toggle wimp`** changing page length instead of wimpy threshold.

14. **Spell bugs** (from C-18 and Category F):
    - **F-2.35 — `harm` spell** dealing wrong damage for base and minus variants.
    - **F-2.36 — `aid` via voice cast** not working for normal variant.
    - **F-2.38 — `voice explosion`** showing no damage or effect messages.
    - **F-2.39 — `windwall`** aura damage reflection not working.
    - **F-2.40 — `disintegrate`** not disintegrating corpses.
    - **F-2.41 / F-2.15 — `transport via plants`** not practicable despite being in help.
    - **F-2.37 — `O GRITO DA MORTE`** bard song not functional.
    - **F-2.21 — `polearms`** weapon skill not assigned to any class.

15. **Gameplay formula bugs**:
    - **F-2.24 — Wisdom not scaling practice gains** per level.
    - **F-2.47 — Hunger/thirst** missing one-tick advance warning.
    - **F-2.29 — Level command** showing duplicate XP for level 59→60.
    - **F-2.32 — Sitting target damage bonus** not applying for bash/fury of air.
    - **F-2.30 — Bard song `balada do andarilho`** working with 0 mana.

16. **World content door/lock fixes (Category D + F-2.46):** Multiple rooms report doors
    that cannot be opened. These are builder tasks (key/lock vnum corrections):
    rooms 766, 13872, 13879, 13888, 14046, 4574, 757, and the Mordecai house doors,
    Gondolin gate, Loctus stone door, Rua Luar de Safira houses 3 and 4.

17. **Typo fixes (Category G):** Seven confirmed typos (G-1 through G-7) in mob messages,
    door descriptions, and a single untranslated English line (`create berries`). These
    are quick, isolated fixes for builders/translators.

18. **Open bugs from C-18** (original TODO list): Gold save in corpses, Gas pão crash,
    REPLY on quit, WHERE/STAT visibility for invis gods, two-handed modifier check.

19. **World encoding sweep (Category D):** Batch-process mob, room, and object
    descriptions in the listed zones to fix accent/encoding issues. This is purely
    builder/content work.

---

### Priority 4 – Small-to-Medium New Features (Short to Medium Term)

20. **Emotional Immunity flag (C-9):** Undead and constructs should not accumulate
    emotional state — lore and correctness issue. A small, isolated code change.

21. **Emotion Visual Cues in `look` (C-3):** High player-facing immersion impact.
    Appends an emotion-state string to mob long descriptions.

22. **Communication Emotion Triggers (C-1):** Fires emotion updates on insults, praise,
    and being ignored; low-risk extension of `say`/`tell` handlers.

23. **Reputation → Emotion Seeding (C-2):** First-encounter emotion initialization
    from player reputation; completes the reputation ↔ emotion feedback loop.

24. **Player Emotion Visibility Tools (C-10):** Add `HELP EMOCOES`, a `sense_motive`/
    `empatia` player command, and a wizard emotion-override command for testing.

25. **Climb Skill (C-15):** Add `climb` skill for warriors and thieves, gating entry to
    `SECT_CLIMBING` rooms. The sector type already exists.

26. **High-voted player ideas (Category E, quick wins):**
    - **E-2.19 — Stoneskin points in `affects`** (Laguna): cosmetic; `score` already
      shows the data, just needs adding to `affects` output.
    - **E-2.15 — Mana density in prompt** (Durandal): QoL for spellcasters; reduces
      room-description visual noise.
    - **E-2.5 — Static item effects in `affects`** (Roscoe): show always-on item bonuses.
    - **E-2.4 — Direction aliases on exits** (Yazid): allow `open norte` as well as
      `open porta`.
    - **E-2.3 — Message buffer in text-editor modes** (Yazid): queue messages received
      while in `idea`/`typo`/`mail` input state.
    - **E-2.11 — `rskills` command** (Laguna): list remort-retained skills; useful once
      Rebegin (B-1) is enabled.

---

### Priority 5 – Medium-Term Feature Work

27. ~~**Big Five Phase 3: Extraversion and Agreeableness (C-11):**~~ **COMPLETED (March 2026).** Social reward gain, aggression damping, and group cooperation bonus implemented in `mobact.c`. See A-7b.

28. **NPC Emotion-Driven Dialogue (C-4):** Requires a design decision on approach
    (dialogue tables vs. DG Script triggers vs. coded response logic). Coordinate with
    the zone-building team.

29. **Alignment ↔ Emotion Integration (C-7):** Wire `CONFIG_EMOTION_ALIGNMENT_SHIFTS`
    to actual periodic shift logic. Requires careful game-balance review.

30. **Zone 113 hometown content completion (B-4):** Finish the zone, then wire
    `r_hometown_*` vnums and enable the character-creation hometown selection menu.

31. **Guild system zone 134 content (B-5):** Audit existing zone 134 rooms and design
    missing guild hall content.

32. **Medium-complexity player ideas (Category E):**
    - **E-2.6 — `Emaranhar` (Entangle) for Druids** (Kasper): new nature-based root spell.
    - **E-2.9 — Silence spell blocking cast and comms** (Cansian): enhance the silence
      spell to also suppress `cast` and communication channels.
    - **E-2.12 — Ranger camping skill** (Henzo): fast-recovery room while sleeping.
    - **E-2.7 — Auto-arrow equip** (Panoramix): combat toggle for archers.
    - **E-2.14 — MV increase based on DEX** (Panoramix): balance change for
      Rangers/archers; relates directly to the `MV` shortfall bug context.
    - **E-2.2 — Comma-separated item keywords** (Yazid): restore legacy `buy pao,via`
      parsing behaviour.
    - **E-2.1 / Motharo — `sentir perigo` enhancements**: direction display at skill ≥ 75% **now implemented** (shows vague warning below 75%). Remaining: no armour penalty while sleeping.

---

### Priority 6 – Long-Term and Research Items

33. **Clan System (B-3):** The largest pending feature. Requires a full design sprint
    covering clan data model, commands, persistence, WHO integration, and reputation
    interaction with clans.

34. **Monk and Paladin Classes (C-14):** New classes require full class design (skills,
    spells, level tables, trainer NPCs, balance). The player-submitted Monk design
    (`lib/misc/ideas` — Kasper, E-2.18) provides a useful blueprint.

35. **Emotion-Based Player Skills (C-5):** Intimidation, Charm, Manipulation, Empathy.
    These require class-balance design before implementation.

36. **Vitalia Banking System (C-17):** Full-featured bank beyond the existing tbaMUD
    spec_proc (statements, interest, dedicated UI). Requires economy design.

37. **Big Five Phase 4: Openness — direct `mobact.c` wandering drive (C-11):** Phase 4 is already active in Shadow Timeline (exploration weighting, novelty/repetition bonuses). The remaining deferred item is an explicit O-driven wandering probability in `mobile_activity()` (low priority, Shadow Timeline coverage is sufficient for now).

38. **Shadow Timeline Phase 3 features (C-12):** Causal Ledger, Temporal Authority Layer,
    multi-step planning. These are advanced architectural additions requiring a design RFC.

39. **FANN Neural Network Integration (C-13):** Research task; requires dedicated
    feasibility study and proof-of-concept before committing.

40. **Object Layering System (C-16):** `PUT OVER` / `PUSH` / `MOVEABLE` / `NOUNDER` /
    `NOOVER` / `NOHIDE`. Requires item-system design and builder documentation.

41. **Faction and Deity/Religion Emotion Integration (C-8):** Depends on a faction
    system not yet designed or built.

42. **Large-scope player ideas (Category E) — design-first:**
    - **E-2.16 — Druid Metamorphosis** (Kasper): major skill/system using mob prototypes.
    - **E-2.17 — Equipment Set System** (Kasper): item set bonuses; needs data model design.
    - **E-2.13 — Card album stash** (Durandal): house-system and ITEM_CARDS integration.
    - **E-2.8 — Locate Object improvement** (Lupulis): balance review needed.
    - **E-2.1 — `cooking` skill** (Cansian): food-crafting skill design.

---

## <a name="matrix"></a>Quick-Reference Matrix

| Feature | Primary Doc | Code Status | Category |
|---------|-------------|-------------|----------|
| Hybrid Emotion System (20-dim) | `EMOTION_SYSTEM_TODO.md` | ✅ Implemented | A-1 |
| Emotion memory / relationship layer | `HYBRID_EMOTION_SYSTEM.md` | ✅ Implemented | A-1 |
| Shadow Timeline (Phase 1 + Phase 2) | `SHADOW_TIMELINE.md` | ✅ Implemented | A-2 |
| Moral Reasoner (Shultz & Daley) | `MORAL_REASONING.md` | ✅ Implemented | A-3 |
| Emotion Projection (4D) | `HYBRID_EMOTION_SYSTEM.md` ✅ | ✅ Implemented | A-4 |
| Group Moral Dynamics | `GROUP_MORAL_DYNAMICS.md` | ✅ Implemented | A-5 |
| Emotion Contagion | `EMOTION_CONTAGION.md` | ✅ Implemented | A-6 |
| Big Five Phase 1: Neuroticism | `BIG_FIVE_PERSONALITY_SYSTEM.md` | ✅ Implemented | A-7 |
| Big Five Phase 2: Conscientiousness | `BIG_FIVE_PERSONALITY_SYSTEM.md` ✅ | ✅ Implemented | A-7 |
| Reputation System (players + mobs) | `REPUTATION_SYSTEM.md` | ✅ Implemented | A-8 |
| Evil Reputation System | `EVIL_REPUTATION_SYSTEM.md` | ✅ Implemented | A-9 |
| Escort Quests (`AQ_MOB_ESCORT`) | `ESCORT_QUESTS.md` | ✅ Implemented | A-10 |
| Magik Triggers (DG Script) | `MAGIK_TRIGGERS.md` | ✅ Implemented | A-11 |
| Voice Casting + Syllables | `VOICE_CASTING.md` | ✅ Implemented | A-12 |
| Sector System (all 17 types) | `SECTOR_SYSTEM.md` | ✅ Implemented | A-13 |
| Multiline Aliases | `MULTILINE_ALIASES.md` | ✅ Implemented | A-14 |
| Percentual Load (zone files) | `PERCENTUAL_LOAD.md` | ✅ Implemented | A-15 |
| Weather / Mana Density System | `WEATHER_SPELL_ENHANCEMENT.md` | ✅ Implemented | A-16 |
| Spell Variant Chains + `experiment` | `SPELL_VARIANT_CHAINS.md` | ✅ Implemented | A-17 |
| Emotion Configuration (cedit) | `CEDIT_EMOTION_CONFIGURATION_GUIDE.md` | ✅ Implemented | A-18 |
| QP ↔ Gold Exchange System | `QP_EXCHANGE_SYSTEM.md` ✅ | ✅ Implemented | A-19 |
| Danger Sense / Death-Trap protection | `DANGER_SENSE_FEATURE.md` ✅ | ✅ Implemented | A-20 |
| Rebegin / Remort | `REBEGIN_FEATURE.md` | ⚠️ Code complete, command disabled | B-1 |
| Big Five Phases 3 & 4 | `BIG_FIVE_PERSONALITY_SYSTEM.md` | ✅ Phase 3 implemented; Phase 4 active in Shadow Timeline | A-7b/c |
| Clan System | *(TODO only)* | ⚠️ Stub (10-line file) | B-3 |
| Hometown System | *(TODO only)* | ⚠️ Partial — zone 113 WIP | B-4 |
| Guild System (zone 134) | *(TODO only)* | ⚠️ Content incomplete | B-5 |
| Auction House | `AUCTION_HOUSE.md` ✅ | ⚠️ Experimental — restricted access | B-6 |
| Communication emotion triggers | `EMOTION_SYSTEM_TODO.md §2.1` | ❌ Not implemented | C-1 |
| Reputation → emotion seeding | `EMOTION_SYSTEM_TODO.md §3.2` | ❌ Not implemented | C-2 |
| Emotion visual cues in `look` | `EMOTION_SYSTEM_TODO.md §4.1` | ❌ Not implemented | C-3 |
| NPC emotion-driven dialogue | `EMOTION_SYSTEM_TODO.md §4.2` | ❌ Not implemented | C-4 |
| Emotion-based player skills | `EMOTION_SYSTEM_TODO.md §5.4` | ❌ Not implemented | C-5 |
| Emotion decay rate tuning config | `EMOTION_SYSTEM_TODO.md §6` | ❌ Not implemented | C-6 |
| Alignment ↔ emotion integration | `EMOTION_SYSTEM_TODO.md §8.1` | ❌ Config flag exists, not wired | C-7 |
| Faction / Deity emotion integration | `EMOTION_SYSTEM_TODO.md §8.2-8.3` | ❌ No faction system exists | C-8 |
| Emotional immunity (undead/constructs) | `EMOTION_SYSTEM_TODO.md §9.2` | ❌ Not implemented | C-9 |
| Player emotion visibility tools | `EMOTION_SYSTEM_TODO.md §7` | ❌ Not implemented | C-10 |
| Big Five Phase 3: Extraversion/Agreeableness | `BIG_FIVE_PERSONALITY_SYSTEM.md` | ✅ Implemented (March 2026) | A-7b |
| Big Five Phase 4: Openness | `BIG_FIVE_PERSONALITY_SYSTEM.md` | ✅ Active in Shadow Timeline; direct mobact.c wandering deferred | A-7c |
| Shadow Timeline Phase 3 features | `SHADOW_TIMELINE.md §Future Extensions` | ❌ Not implemented | C-12 |
| FANN neural network integration | `RFC-1001_NPC_PSYCHOLOGY_CHECKLIST.md` | ❌ Included, not integrated | C-13 |
| Monk class | *(TODO only)* | ❌ Not implemented | C-14 |
| Paladin class | *(TODO only)* | ❌ Not implemented | C-14 |
| Climb skill (warriors/thieves) | *(TODO only)* | ❌ Sector type exists, skill absent | C-15 |
| Object layering (PUT OVER / PUSH) | *(TODO only)* | ❌ Not implemented | C-16 |
| Vitalia Banking System | *(TODO only)* | ❌ Basic tbaMUD bank only | C-17 |
| Misc open bugs | *(TODO only)* | ❌ Open | C-18 |
| World encoding (mobs/areas/objs) | *(TODO only)* | 🔨 Builder content work | D |
| Zone shop completions | *(TODO only)* | 🔨 Builder content work | D |
| Skill `cooking` | `lib/misc/ideas` (Cansian) | ❌ Not implemented | E-2.1 |
| Single-word cast without quotes | `lib/misc/ideas` (Yazid) | ❌ Not implemented | E-2.3 |
| Comma-separated item keywords | `lib/misc/ideas` (Yazid) | ❌ Not implemented | E-2.2 |
| Message buffer in text-editor modes | `lib/misc/ideas` (Yazid) | ❌ Not implemented | E-2.3 |
| Direction aliases on exits | `lib/misc/ideas` (Yazid) | ❌ Not implemented | E-2.4 |
| Static item effects in `affects` | `lib/misc/ideas` (Roscoe) | ❌ Not implemented | E-2.5 |
| Emaranhar (Entangle) for Druids | `lib/misc/ideas` (Kasper) | ❌ Not implemented | E-2.6 |
| Auto-arrow equip in combat | `lib/misc/ideas` (Panoramix) | ❌ Not implemented | E-2.7 |
| Locate Object result improvement | `lib/misc/ideas` (Lupulis) | ❌ Not implemented | E-2.8 |
| Silence spell blocks cast + comms | `lib/misc/ideas` (Cansian) | ❌ Not implemented | E-2.9 |
| Item rebalance: argolas do ancião | `lib/misc/ideas` (Lupulis) | ❌ World content | E-2.10 |
| `rskills` remort command | `lib/misc/ideas` (Laguna) | ❌ Not implemented | E-2.11 |
| Ranger camping skill | `lib/misc/ideas` (Henzo) | ❌ Not implemented | E-2.12 |
| Card album / house stash | `lib/misc/ideas` (Durandal) | ❌ Not implemented | E-2.13 |
| MV increase based on DEX | `lib/misc/ideas` (Panoramix) | ❌ Not implemented | E-2.14 |
| Mana density in prompt line | `lib/misc/ideas` (Durandal) | ❌ Not implemented | E-2.15 |
| Druid Metamorphosis | `lib/misc/ideas` (Kasper) | ❌ Not implemented | E-2.16 |
| Equipment Set System | `lib/misc/ideas` (Kasper) | ❌ Not implemented | E-2.17 |
| Monk class (player design) | `lib/misc/ideas` (Kasper) | ❌ Not implemented (design exists) | E-2.18 |
| Stoneskin points in `affects` | `lib/misc/ideas` (Laguna) | ❌ Not implemented | E-2.19 |
| Flecha de Sagitário as worn item | `lib/misc/ideas` (Lupulis) | ❌ World content design | E-2.20 |
| Word-triggered doors broken (systemic) | `lib/misc/bugs` (Lupulis+) | ✅ **Fixed** — triggers attached; flags corrected; same-kw pairs merged | F-2.16/18/26/27 |
| `who` command showing 3× player count | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.33 |
| `toggle wimp` changes page size | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.34 |
| `polearms` skill unassigned to any class | `lib/misc/bugs` (Laguna) | 🔴 Open | F-2.21 |
| `harm` spell wrong damage (base/minus) | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.35 |
| `voice explosion` no output | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.38 |
| `disintegrate` doesn't destroy corpses | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.40 |
| `transport via plants` unpracticable | `lib/misc/bugs` (Yazid+) | 🔴 Open | F-2.15/41 |
| `O GRITO DA MORTE` song broken | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.37 |
| Wisdom not scaling practice gains | `lib/misc/bugs` (Roscoe) | 🔴 Open | F-2.24 |
| Hunger/thirst no advance warning | `lib/misc/bugs` (Lupulis) | 🔴 Open | F-2.47 |
| `AID PLUS` decreases max HP | `lib/misc/bugs` (Henzo) | 🔴 Open | F-2.22 |
| `windwall` aura not reflecting damage | `lib/misc/bugs` (Astus) | 🔴 Open | F-2.39 |
| Multiple colour-bleed & door bugs (F-1) | `lib/misc/bugs` | ✅ Likely fixed — verify | F-1 |
| 7 typo fixes (gender, name, translation) | `lib/misc/typos` | 🔨 Quick builder fix | G-1 to G-7 |

---

*End of TODO Audit — Vitalia Reborn*
