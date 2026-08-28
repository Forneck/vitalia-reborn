# NPC "Acting" Systems — Code Review Answers (Issue #1008)

This document answers the issue sections with concrete code references.

## 1. Goal / Objective System (Shadow Timeline)

- **`current_goal` definition/population**
  - Defined in `struct mob_ai_data` (`src/structs.h`, field `current_goal`).
  - Populated by:
    - Goal editor (`src/gedit.c`, `gedit_parse()` / `GEDIT_GOAL_CHOICE`).
    - Quest assignment (`src/quest.c`, `set_mob_quest()` sets `GOAL_COMPLETE_QUEST`).
    - Runtime tactical AI (`src/mobact.c`, e.g. `mob_process_wishlist_goals()`, `mob_try_to_accept_quest()`).
- **Utility-only vs relational**
  - Not utility-only: goals include target-aware/socially-adjacent flows (`GOAL_HUNT_TARGET`, `GOAL_FOLLOW`) and `goal_target_mob_rnum` (`src/structs.h`).
  - But there is **no explicit “relational objective” schema** (e.g., “raise trust with X by +20”).
- **`achieves_goal` extensibility**
  - Per-action projection logic already sets `outcome.achieves_goal` with target/context checks (`src/shadow_timeline.c`, `shadow_execute_projection()` and projection generators).
  - Final scorer gives a generic bonus (`score_projection_for_entity()`: `if (proj->outcome.achieves_goal) score += 50;`).
  - Conclusion: target-specific objective checks are already possible in projection generation, but reward application is currently generic.
- **Per-NPC scene objective structure**
  - Closest reusable state is `mob_ai_data` goal fields (`current_goal`, `goal_destination`, `goal_target_mob_rnum`, `goal_timer` in `src/structs.h`).
  - For `(target + desired relational delta + expiry)` you likely need new fields (or a small new struct) because desired emotional delta is not represented today.
- **Who assigns goals today**
  - Builder-assigned via OLC (`src/gedit.c`).
  - Runtime-emergent via genetics/emotion/quest-wishlist logic (`src/mobact.c`).
  - Personality/memory (Big Five/MALP/MPLP) currently influence projection scoring/selection (`src/shadow_timeline.c`), not a first-class relational goal object.

## 2. Obstacle Modeling

- **Existing resistance from relationship data**
  - There is a resistance concept in Shadow biasing (`src/shadow_timeline.c`, `apply_anchoring_bias()`), where resistance uses MPLP suspicion + inverse forgiveness.
  - There is contextual relationship influence in 4D modulation (`src/emotion_projection.c`, `emotion_apply_contextual_modulation()` affiliation bias from trust/friendship).
  - But there is no clean explicit “target resistance blocks/delays action” primitive in MobAct.
- **Best injection point**
  - Prefer **candidate generation/scoring time** (`generate_*_projections()` / `shadow_score_projections()`), not `shadow_evaluate_real_outcome()`.
  - `shadow_evaluate_real_outcome()` currently evaluates post-action HP/combat state only (`src/shadow_timeline.c`), so it is too late for obstacle gating.
- **Fear/Courage score vs latency**
  - Score: yes (`src/shadow_timeline.c`, `score_projection_for_entity()` adjusts for fear/courage).
  - Latency/deliberation: indirectly yes via arousal path (`src/mobact.c`, `apply_executive_control()`; `src/utils.c`, `calculate_emotional_arousal()` and `apply_conscientiousness_reaction_delay()`).

## 3. Subtext / Trait-vs-Behavior Divergence

- **Explicit stored gap today?**
  - Not stored as a dedicated value.
  - Effective emotion is computed on read (`src/utils.c`, `get_effective_emotion_toward()`).
  - Trait baselines (Big Five) are separate (`src/structs.h` personality fields; trait readers used in scoring).
- **Can `mask_delta` be bolt-on read-only?**
  - Yes. A derived read-only value can be computed where needed from existing trait getters + `get_effective_emotion_toward()` without changing the hybrid formula.
- **Social text generation fixed vs live hook**
  - Social text payload is fixed per social entry loaded from socials files (`src/db.c`, `boot_social_messages()`).
  - Central social render path is `src/act.social.c`, `ACMD(do_action)` using `act(...)`.
  - Emotion state is already read for behavior updates around that path (`update_mob_emotion_*` calls), but not as dynamic text rewriting of the social strings.

## 4. Emotional Disclosure Tiers (`look` / `examine` / `stat`)

- **What normal players see now**
  - Dynamic emotional cue text in `look <mob>` via `look_at_char()` -> `show_emotion_cue_to_char()` (`src/act.informative.c`).
  - Optional brief indicators in room listings when `PRF_DISPEMOTE` is enabled (`list_one_char()` path using `get_highest_emotion_display()`).
  - So it is not “nothing” and not purely static description.
- **Skill/attribute gate available now?**
  - No existing Wisdom/Empathy/Perception gate found in these disclosure paths.
  - Full effective-emotion block is GOD-gated (`src/act.wizard.c`, `do_stat` -> `do_stat_mob_emotions()`, level check `LVL_GOD`).
- **Rough effort for tiered close-up read**
  - Low-to-medium: add a skill/attribute threshold check and a compact 1–2 emotion view in `look_at_char()` / `show_emotion_cue_to_char()` path, leaving `stat ... emotions` unchanged.

## 5. Voice / Delivery Tags (paralinguistic text)

- **How `say`/`tell` output is built**
  - `say`: `src/act.comm.c`, `ACMD(do_say)` builds template then sends through `act(...)`.
  - `tell/reply`: `perform_tell()`, `ACMD(do_tell)`, `ACMD(do_reply)` in `src/act.comm.c`.
- **Single chokepoint for all NPC speech?**
  - Not globally. Some NPC “speech-like” lines in AI/special procs are direct `act("$n ...")` strings (`src/mobact.c`, `src/spec_procs.c`) and bypass `do_say`.
- **Risk/cost of delivery tags**
  - Cheap/safe for command-path speech (`do_say`/`perform_tell`), but partial coverage unless AI-specific direct `act` lines are also refactored to use a shared formatter.

## 6. Silence / Response Latency

- **Fixed pulse vs variable delay in triggers**
  - DG scripts support variable delay via `wait` (`src/dg_scripts.c`, `process_wait()`, `trig_wait_event()`, restart with `TRIG_RESTART`; event queue in `src/dg_event.c`).
  - So there is already a native variable-delay mechanism.
- **Risk of fear/trust-conditioned latency**
  - Medium/high compatibility risk if applied globally to trigger firing semantics: many scripts assume immediate flow unless they explicitly use `wait`.
  - Safer pattern: opt-in delays at selected NPC response sites rather than changing DG core timing.

## 7. Rehearsal / Habituation & Variation

- **Is `Rehearsal` used outside consolidation formula?**
  - Yes:
    - Passive decay/thresholding (`src/malp.c`, consolidator paths).
    - Dominance weighting in emotional effects (`apply_malp_emotion_effects()`).
    - Reconsolidation update strength (`retrieve_and_reconsolidate()`).
    - Social gossip transfer/update (`try_social_gossip()`).
- **Where habituation/variation should live**
  - Emotional habituation belongs near MALP/emotion effect application.
  - Text variation belongs in social/speech emission code (`do_action`, `do_say`, AI social emitters), not inside MALP persistence internals.

## 8. Blocking / Spatial Behavior

- **Follow-distance static vs dynamic**
  - No explicit distance scalar beyond room-level follow/pathing checks.
  - Decision to follow is heavily tied to `follow_tendency` and local heuristics (`src/mobact.c`, `mob_try_stealth_follow()`), with path movement in `mob_follow_leader()`.
  - Trust/Fear are not currently used as first-class per-target spacing controls.
- **Where to implement reactive spacing**
  - Requires touching movement/AI decision points (`mob_follow_leader()`, `mob_try_stealth_follow()`, optionally Shadow follow projection scoring), not just static data lookup.

## 9. Priority / Scope / Risk (planning)

- **Hottest-path items**
  - Anything added to per-tick loops is sensitive:
    - `mobile_activity()` / `mob_emotion_activity()` (`src/mobact.c`).
    - `shadow_score_projections()` (`src/shadow_timeline.c`).
- **Existing tests for Shadow/MALP**
  - No dedicated unit/regression suite found for Shadow Timeline scoring or MALP consolidation in current repo test scripts.
- **Fragile areas to harden first**
  - MobAct + script-trigger interaction is historically fragile (many extraction guards around `act()`, movement, combat calls in `src/mobact.c`).
  - Any latency/response or speech-path changes touching these areas should be incremental and guarded.
- **Quick wins (low risk / high legibility)**
  - Delivery tags on `say`/`tell` command path.
  - Tiered player-facing emotional readout in `look`.
  - Social text variation hooks in social execution layer (without touching MALP core structure).
- **Bigger lifts (architectural)**
  - Relational objective schema (`target + desired delta + expiry`) with planner integration.
  - Obstacle/resistance model that truly gates/suppresses target actions end-to-end.
  - Global latency conditioning across trigger-driven interactions.
