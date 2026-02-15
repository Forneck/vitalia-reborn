# RFC-0002: Shadow Timeline — Open Questions & Domain Gaps

**Date:** 2026-02-08  
**Status:** 📝 CLASSIFIED  
**Target Audience:** Engine Developers, Systems Designers  
**Related RFCs:**
- RFC-0001 (Implementation Summary)
- docs/INVARIANTS_ANALYSIS.md
- docs/SHADOW_TIMELINE.md

**Scope:** Conceptual / Architectural  
**Non-Goal:** This RFC does not propose solutions or implementations.

---

## 1. Purpose

The purpose of this RFC is to enumerate the open questions required to formalize the Shadow Timeline as a first-class system inside the Vitalia Reborn MUD engine.

These questions aim to:

- **Identify which aspects are already implicitly answered** by the current codebase
- **Identify missing concepts** that cannot be inferred from existing mechanics
- **Prevent premature architectural commitment**
- **Establish a shared vocabulary** for future RFCs

The Shadow Timeline is treated here as a **cognitive observation domain**, not as a time-travel mechanic.

---

## 2. Foundational Assumptions (Current Consensus)

The following assumptions are considered accepted for the purpose of this RFC:

The Shadow Timeline:
- Observes **possible futures**, not past states
- **Cannot violate global invariants**
- **Does not modify world state**
- **Is not bound** to rooms, zones, objects, or resets

**Objects and rooms have no agency** and therefore cannot consult the Shadow Timeline

**Entities with decision-making capacity** (players, mobs) may consult it

The Shadow Timeline **may be imperfect, biased, or incomplete**

---

## 3. Ontology of the Shadow Timeline

These questions define **what** the Shadow Timeline **is**.

### Question 1
**Is a Shadow Timeline instantiated:**
- per entity?
- per decision?
- per action attempt?

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Per decision (per tick).** Each call to `mob_shadow_choose_action()` in `mobile_activity()` (mobact.c:628) creates a new `shadow_context` via `shadow_init_context()`, generates projections, selects best action, then frees the context (shadow_timeline.c:940-979). The context is ephemeral and lasts only for one decision cycle. Mobs with `MOB_SHADOWTIMELINE` flag invoke this once per game tick.

**Impact if unresolved:** Memory model, concurrency, AI realism.

---

### Question 2
**Does a Shadow Timeline have an identity (ID), or is it always ephemeral?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Always ephemeral.** The `shadow_context` structure has no ID field and exists only for the duration of projection generation. Created in `shadow_init_context()` and freed in `shadow_free_context()` with no persistent identity.

**Impact if unresolved:** Memory model, debugging, observability.

---

### Question 3
**Can multiple Shadow Timelines coexist for the same entity at the same moment?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **No.** The implementation follows a synchronous pattern: init → generate → select → free. Only one context exists per decision call. No concurrent context management is implemented.

**Impact if unresolved:** Concurrency, resource management, AI complexity.

---

### Question 4
**Can two entities share a Shadow Timeline (fully or partially)?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **No.** Each `shadow_context` is explicitly bound to a single entity via `ctx->entity` (line 55 in shadow_context structure). No sharing mechanism exists.

**Impact if unresolved:** Social intelligence, cooperative planning, performance.

---

### Question 5
**Can a Shadow Timeline observe another Shadow Timeline (meta-simulation)?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** Shadow projections simulate action outcomes, but there's no mechanism for a projection to recursively invoke shadow simulation or observe other entities' projections.

**Impact if unresolved:** Cognitive depth, computational complexity, recursive limits.

---

### Question 6
**Is a Shadow Timeline strictly single-branch during observation, or can it hold multiple branches concurrently?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Multiple branches concurrently.** The context holds an array of projections (`ctx->projections`, max 10) representing different possible futures evaluated in parallel. Each projection is an independent branch.

**Impact if unresolved:** Memory model, branch management, decision quality.

---

### Question 7
**Is there a theoretical maximum branching depth, independent of computational limits?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Yes.** `SHADOW_MAX_HORIZON` is set to 5 steps (line 26 in shadow_timeline.h), defining the maximum projection depth. This is a design constant, not just a computational limit.

**Impact if unresolved:** Resource planning, AI behavior bounds, game balance.

---

## 4. Epistemology: Knowledge and Awareness

These questions define what an entity **knows** about the Shadow Timeline.

### Question 8
**Does an entity know it is simulating futures, or does it treat results as intuition?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** The implementation provides no mechanism for entities to be aware of or reflect upon their simulation process. Mobs simply execute the selected action with no meta-cognition.

**Impact if unresolved:** Narrative consistency, player experience, AI transparency.

---

### Question 9
**Are Shadow Timeline results explicit knowledge or implicit bias?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Implicit bias.** The shadow system influences action selection through scoring (`shadow_score_projections()`) but doesn't create explicit knowledge or memory. The entity acts on the result without "knowing" it predicted.

**Impact if unresolved:** Learning systems, memory integration, roleplay realism.

---

### Question 10
**Can an entity mistrust or discount its own predictions?**

**Classification:**
- [ ] Answered implicitly by current code
- [x] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Partially constrained.** While there's no explicit mistrust mechanism, `shadow_select_best_action()` (line 659) rejects actions with scores below -50, effectively discounting bad predictions. However, there's no learning or calibration based on prediction accuracy.

**Impact if unresolved:** Emotional feedback loops, personality systems, AI believability.

---

### Question 11
**Can predictions contradict each other, and if so, how is conflict resolved?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Yes, resolved by highest score.** Multiple projections generate different (potentially contradictory) outcomes, and `shadow_select_best_action()` selects the one with the highest subjective score.

**Impact if unresolved:** Decision quality, cognitive realism, debugging complexity.

---

### Question 12
**Does emotional state affect:**
- only the weighting of futures?
- or also which futures are considered at all?

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Only weighting.** Emotional state affects scoring in `score_projection_for_entity()` (lines 622-654) and subjectivity bias in `shadow_apply_subjectivity()` (lines 808-838), but not which actions are considered. All available actions are projected regardless of emotion.

**Impact if unresolved:** Emotion system integration, personality variance, AI diversity.

---

### Question 13
**Can an entity be permanently unable to imagine certain futures?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** All cognitive entities can project all action types. There's no mechanism for permanent cognitive disabilities or learned limitations.

**Impact if unresolved:** Learning systems, cognitive disabilities, character depth.

---

## 5. Validity and Discard Rules

These questions define which futures are **allowed to exist** in simulation.

### Question 14
**What formally distinguishes:**
- impossible futures
- improbable futures

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Impossible = invariant violation, improbable = low score.** `shadow_validate_action()` (lines 366-449) distinguishes impossible actions (return ACTION_IMPOSSIBLE, ACTION_VIOLATES_INVARIANT) from feasible ones. Improbable actions are feasible but score poorly.

**Impact if unresolved:** Performance, realism, AI trustworthiness.

---

### Question 15
**Are futures that violate global invariants:**
- silently discarded?
- explicitly marked as impossible?

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Silently discarded.** Invalid actions are filtered during projection generation (lines 168, 218, 291, 341) and never added to the projection array. No explicit marking occurs.

**Impact if unresolved:** Debugging, cognitive realism, invariant enforcement.

---

### Question 16
**Can an impossible future still generate emotional responses (fear, relief)?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** Impossible actions are discarded before outcome prediction, so they never generate emotional predictions. The system has no mechanism for "impossible but considered" scenarios.

**Impact if unresolved:** Emotion system integration, phobia modeling, gameplay depth.

---

### Question 17
**Is there a cognitive cost to simulating an impossible future?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **No cost.** Invalid actions are discarded before `shadow_calculate_cost()` is called, and capacity is only consumed after successful projection (line 192). Validation is "free."

**Impact if unresolved:** Resource management, AI realism, performance optimization.

---

### Question 18
**At what stage are invalid futures discarded:**
- before simulation
- after partial simulation?

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Before simulation.** `shadow_validate_action()` is called before `shadow_execute_projection()` in all projection generators (lines 168, 218, 291, 341).

**Impact if unresolved:** Performance, early-exit strategies, cognitive realism.

---

## 6. Temporal Resolution and Randomness

These questions define how **time** behaves inside the Shadow Timeline.

### Question 19
**What is the minimum temporal granularity of a Shadow Timeline simulation?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **One game tick.** The Shadow Timeline is invoked once per tick in `mobile_activity()` (mobact.c:623-628), and capacity regenerates per tick (mobact.c:617-619). The "horizon" parameter represents number of action attempts, but the base temporal unit is the game's tick cycle. Each projection step corresponds to one potential action execution in the game loop.

**Impact if unresolved:** Sync with main game loop, determinism, precision.

---

### Question 20
**Is simulated time discrete or continuous?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Discrete.** Time is measured in integer horizon steps (`int horizon`), not continuous time. Each projection represents a discrete sequence of actions.

**Impact if unresolved:** Time representation, physics consistency, implementation complexity.

---

### Question 21
**How is randomness handled:**
- frozen at observation start?
- re-sampled per branch?

**Classification:**
- [ ] Answered implicitly by current code
- [x] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Partially constrained.** The implementation uses heuristic predictions without explicit randomness modeling. Outcome predictions (e.g., combat results in lines 514-543) are deterministic given inputs, but ST-5 principle states repeated evaluations *should* yield different results. No RNG seeding or randomness control is implemented.

**Impact if unresolved:** Determinism, branch diversity, predictability.

---

### Question 22
**Can a Shadow Timeline simulate hesitation, delay, or execution error?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** Projections assume perfect execution. There's no modeling of timing variation, hesitation, or execution failure beyond success/failure of the action itself.

**Impact if unresolved:** Realism, timing-dependent actions, combat accuracy.

---

### Question 23
**Is there a distinction between:**
- chosen action
- executed action inside the simulation?

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Yes, there is a distinction.** The chosen action (from shadow projection) is passed to `mobact.c:628-806` where it undergoes real-world validation before execution. For example, `SHADOW_ACTION_ATTACK` verifies the target still exists and is in the same room (mobact.c:640-650) before executing `hit()`. The simulation assumes perfect execution, but real execution includes runtime validation and can fail due to changed world state.

**Impact if unresolved:** Action resolution, failure modeling, skill system integration.

---

## 7. Interaction with Memory and History

These questions bridge Shadow Timeline with future systems (e.g. Causal Ledger).

### Question 24
**When does a prediction become memory?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** Projections are ephemeral and not stored after action selection. There's no mechanism for converting predictions into memories.

**Impact if unresolved:** Learning, narrative coherence, memory system design.

---

### Question 25
**Do incorrect predictions leave persistent emotional or cognitive traces?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** The system has no feedback loop to compare predictions against actual outcomes or store traces of prediction errors.

**Impact if unresolved:** Learning systems, emotional dynamics, character development.

---

### Question 26
**Is prediction accuracy tracked per entity?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** No tracking of prediction vs. reality exists. The system has no metrics for accuracy or calibration.

**Impact if unresolved:** AI self-improvement, debugging, player-facing stats.

---

### Question 27
**Does the Causal Ledger observe:**
- only actual events?
- or also predicted ones?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** The Causal Ledger system doesn't exist yet. This question is forward-looking for RFC-005.

**Impact if unresolved:** Future Causal Ledger design, audit trails, causality tracking.

---

### Question 28
**Can an entity recall past predictions?**

**Classification:**
- [x] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **No.** Projections are freed immediately after action selection (line 976). No storage mechanism exists for past predictions.

**Impact if unresolved:** Memory system integration, learning, narrative continuity.

---

## 8. Authority and Observability

These questions define who can **see** or **interfere** with the Shadow Timeline.

### Question 29
**Can administrators inspect Shadow Timelines?**

**Classification:**
- [ ] Answered implicitly by current code
- [x] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Partially constrained.** `shadow_dump_context()` exists (line 917) but is unimplemented (no-op). The infrastructure for admin inspection is stubbed but not functional.

**Impact if unresolved:** Debugging, admin tooling, transparency.

---

### Question 30
**Can Shadow Timelines be logged or audited?**

**Classification:**
- [ ] Answered implicitly by current code
- [x] Partially constrained by current mechanics
- [ ] Not represented at all

**Answer:** **Partially constrained.** `shadow_log()` exists (line 900) but is a no-op to avoid log spam. Logging infrastructure exists but is disabled.

**Impact if unresolved:** Debugging, performance profiling, behavior analysis.

---

### Question 31
**Is there a notion of "invalid reasoning" for AI debugging?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** The system validates actions but doesn't track or log reasoning quality or identify flawed decision patterns.

**Impact if unresolved:** AI quality assurance, debugging, trust.

---

### Question 32
**Can scripts access Shadow Timeline results?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** No scripting API exists for Shadow Timeline. The system is internal to C code only.

**Impact if unresolved:** Scripting power, quest design, narrative control.

---

### Question 33
**Is player observation ever authoritative over prediction systems?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [x] Not represented at all

**Answer:** **Not represented.** Players have no interface to or authority over Shadow Timeline. The system is AI-only with no player interaction.

**Impact if unresolved:** Player agency, admin overrides, game fairness.

---

## 9. Relation to Existing Codebase

For each question above, reviewers should classify:
- **Answered implicitly by current code** - The implementation already constrains or answers this
- **Partially constrained by current mechanics** - Some aspects are defined, others remain open
- **Not represented at all** - No current code addresses this question

**Classification is now complete as of 2026-02-08.**

### Current Implementation Status

Based on RFC-0001 implementation (`src/shadow_timeline.c`, `src/shadow_timeline.h`, `docs/SHADOW_TIMELINE.md`):

The current implementation provides:
- ✅ Shadow action structures and types
- ✅ Shadow outcome predictions
- ✅ Cognitive capacity system
- ✅ Projection generation and scoring
- ✅ Subjectivity through entity-specific evaluation
- ✅ Bounded horizon (1-5 steps, default 3)
- ✅ Non-determinism through random variation
- ✅ Validation against basic invariants

### Classification Summary

**Questions Answered (19 total):**
- Q1: Per-decision instantiation (once per tick for flagged mobs)
- Q2: Ephemeral, no ID
- Q3: No concurrent timelines
- Q4: No sharing between entities
- Q6: Multiple branches concurrent
- Q7: Max horizon = 5
- Q9: Implicit bias, not explicit knowledge
- Q11: Contradictions resolved by score
- Q12: Emotions affect only weighting
- Q14: Impossible vs improbable distinction
- Q15: Invalid futures silently discarded
- Q17: No cost for impossible futures
- Q18: Discard before simulation
- Q19: One tick is minimum granularity
- Q20: Discrete time (steps)
- Q23: Yes, chosen differs from executed (validation in mobact.c)
- Q28: No recall of past predictions

**Questions Partially Constrained (4 total):**
- Q10: Limited discounting via score threshold
- Q21: Deterministic outcomes, but ST-5 claims non-determinism
- Q29: Dump function exists but unimplemented
- Q30: Log function exists but disabled

**Questions Not Represented (10 total):**
- Q5: No meta-simulation
- Q8: No entity awareness of simulation
- Q13: No permanent cognitive limitations
- Q16: No emotions from impossible futures
- Q22: No hesitation/delay/error modeling
- Q24: No prediction-to-memory conversion
- Q25: No prediction error traces
- Q26: No accuracy tracking
- Q27: No Causal Ledger integration
- Q31: No invalid reasoning detection
- Q32: No script access
- Q33: No player authority

---

## 10. Next Steps (Not Part of This RFC)

Future RFCs that will address subsets of these questions:

- **RFC-003:** Shadow Timeline — Formal Model
- **RFC-004:** Temporal Authority Layer
- **RFC-005:** Causal Ledger

These subsequent RFCs should reference specific question numbers from this document.

---

## 11. Final Note

> **The Shadow Timeline is not a feature.**  
> **It is a domain of meaning.**

This RFC exists to ensure that **meaning is defined before code gives it unintended shape**.

The questions enumerated here have been classified against the current implementation. Their purpose is to:
1. **Document uncertainty** without prematurely constraining solutions
2. **Establish vocabulary** for future architectural discussions
3. **Prevent implicit assumptions** from becoming permanent constraints
4. **Enable informed debate** about design tradeoffs

Answers are now provided based on code analysis. The classification framework ensures that:
- **Explicit** rather than implicit
- **Intentional** rather than accidental
- **Documented** rather than tribal knowledge

---

## 12. Review and Classification Process

This RFC has been completed by examining:

1. **Implementation Code:** `src/shadow_timeline.c` and `src/shadow_timeline.h`
2. **Documentation:** `docs/SHADOW_TIMELINE.md`
3. **Integration Points:** `src/mobact.c` (mobile_activity function, lines 616-806)
4. **Invariants:** `docs/INVARIANTS_ANALYSIS.md`

For each question, one classification was marked:
- ✅ **Answered implicitly** (19) - Code constrains or defines the answer
- 🔶 **Partially constrained** (4) - Some aspects defined, others open
- ❌ **Not represented** (10) - No current code addresses this

This classification has been completed by the development team on 2026-02-08.

---

**RFC Author:** Development Team  
**Date Opened:** 2026-02-08  
**Date Classified:** 2026-02-08  
**Status:** Classification Complete  
**Version:** 1.0 (Classified)
