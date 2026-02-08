# RFC-0002: Shadow Timeline — Open Questions & Domain Gaps

**Date:** 2026-02-08  
**Status:** 📝 DRAFT  
**Target Audience:** Engine Developers, Systems Designers  
**Related RFCs:**
- RFC-0001 (Implementation Summary)
- INVARIANTS_ANALYSIS.md
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
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Memory model, concurrency, AI realism.

---

### Question 2
**Does a Shadow Timeline have an identity (ID), or is it always ephemeral?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Memory model, debugging, observability.

---

### Question 3
**Can multiple Shadow Timelines coexist for the same entity at the same moment?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Concurrency, resource management, AI complexity.

---

### Question 4
**Can two entities share a Shadow Timeline (fully or partially)?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Social intelligence, cooperative planning, performance.

---

### Question 5
**Can a Shadow Timeline observe another Shadow Timeline (meta-simulation)?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Cognitive depth, computational complexity, recursive limits.

---

### Question 6
**Is a Shadow Timeline strictly single-branch during observation, or can it hold multiple branches concurrently?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Memory model, branch management, decision quality.

---

### Question 7
**Is there a theoretical maximum branching depth, independent of computational limits?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Resource planning, AI behavior bounds, game balance.

---

## 4. Epistemology: Knowledge and Awareness

These questions define what an entity **knows** about the Shadow Timeline.

### Question 8
**Does an entity know it is simulating futures, or does it treat results as intuition?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Narrative consistency, player experience, AI transparency.

---

### Question 9
**Are Shadow Timeline results explicit knowledge or implicit bias?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Learning systems, memory integration, roleplay realism.

---

### Question 10
**Can an entity mistrust or discount its own predictions?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Emotional feedback loops, personality systems, AI believability.

---

### Question 11
**Can predictions contradict each other, and if so, how is conflict resolved?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Decision quality, cognitive realism, debugging complexity.

---

### Question 12
**Does emotional state affect:**
- only the weighting of futures?
- or also which futures are considered at all?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Emotion system integration, personality variance, AI diversity.

---

### Question 13
**Can an entity be permanently unable to imagine certain futures?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Learning systems, cognitive disabilities, character depth.

---

## 5. Validity and Discard Rules

These questions define which futures are **allowed to exist** in simulation.

### Question 14
**What formally distinguishes:**
- impossible futures
- improbable futures

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Performance, realism, AI trustworthiness.

---

### Question 15
**Are futures that violate global invariants:**
- silently discarded?
- explicitly marked as impossible?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Debugging, cognitive realism, invariant enforcement.

---

### Question 16
**Can an impossible future still generate emotional responses (fear, relief)?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Emotion system integration, phobia modeling, gameplay depth.

---

### Question 17
**Is there a cognitive cost to simulating an impossible future?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Resource management, AI realism, performance optimization.

---

### Question 18
**At what stage are invalid futures discarded:**
- before simulation
- after partial simulation?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Performance, early-exit strategies, cognitive realism.

---

## 6. Temporal Resolution and Randomness

These questions define how **time** behaves inside the Shadow Timeline.

### Question 19
**What is the minimum temporal granularity of a Shadow Timeline simulation?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Sync with main game loop, determinism, precision.

---

### Question 20
**Is simulated time discrete or continuous?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Time representation, physics consistency, implementation complexity.

---

### Question 21
**How is randomness handled:**
- frozen at observation start?
- re-sampled per branch?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Determinism, branch diversity, predictability.

---

### Question 22
**Can a Shadow Timeline simulate hesitation, delay, or execution error?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Realism, timing-dependent actions, combat accuracy.

---

### Question 23
**Is there a distinction between:**
- chosen action
- executed action inside the simulation?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Action resolution, failure modeling, skill system integration.

---

## 7. Interaction with Memory and History

These questions bridge Shadow Timeline with future systems (e.g. Causal Ledger).

### Question 24
**When does a prediction become memory?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Learning, narrative coherence, memory system design.

---

### Question 25
**Do incorrect predictions leave persistent emotional or cognitive traces?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Learning systems, emotional dynamics, character development.

---

### Question 26
**Is prediction accuracy tracked per entity?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** AI self-improvement, debugging, player-facing stats.

---

### Question 27
**Does the Causal Ledger observe:**
- only actual events?
- or also predicted ones?

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Future Causal Ledger design, audit trails, causality tracking.

---

### Question 28
**Can an entity recall past predictions?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Memory system integration, learning, narrative continuity.

---

## 8. Authority and Observability

These questions define who can **see** or **interfere** with the Shadow Timeline.

### Question 29
**Can administrators inspect Shadow Timelines?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Debugging, admin tooling, transparency.

---

### Question 30
**Can Shadow Timelines be logged or audited?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Debugging, performance profiling, behavior analysis.

---

### Question 31
**Is there a notion of "invalid reasoning" for AI debugging?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** AI quality assurance, debugging, trust.

---

### Question 32
**Can scripts access Shadow Timeline results?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Scripting power, quest design, narrative control.

---

### Question 33
**Is player observation ever authoritative over prediction systems?**

**Classification:**
- [ ] Answered implicitly by current code
- [ ] Partially constrained by current mechanics
- [ ] Not represented at all

**Impact if unresolved:** Player agency, admin overrides, game fairness.

---

## 9. Relation to Existing Codebase

For each question above, reviewers should classify:
- **Answered implicitly by current code** - The implementation already constrains or answers this
- **Partially constrained by current mechanics** - Some aspects are defined, others remain open
- **Not represented at all** - No current code addresses this question

**No implementation should begin until this classification is complete.**

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

**Questions requiring further classification:**
- Questions 1-33 above need to be mapped against the implementation
- Some answers may be implicit in design choices
- Some questions may reveal gaps requiring RFC-003 or later

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

The questions enumerated here are intentionally left open. Their purpose is to:
1. **Document uncertainty** without prematurely constraining solutions
2. **Establish vocabulary** for future architectural discussions
3. **Prevent implicit assumptions** from becoming permanent constraints
4. **Enable informed debate** about design tradeoffs

No answers are provided in this RFC. The classification framework ensures that when answers emerge, they are:
- **Explicit** rather than implicit
- **Intentional** rather than accidental
- **Documented** rather than tribal knowledge

---

## 12. Review and Classification Process

To complete this RFC, each question should be reviewed by examining:

1. **Implementation Code:** `src/shadow_timeline.c` and `src/shadow_timeline.h`
2. **Documentation:** `docs/SHADOW_TIMELINE.md`
3. **Integration Points:** `src/mobact.c`, `src/quest.c`
4. **Invariants:** `docs/INVARIANTS_ANALYSIS.md`

For each question, mark one classification:
- ✅ **Answered implicitly** - Code constrains or defines the answer
- 🔶 **Partially constrained** - Some aspects defined, others open
- ❌ **Not represented** - No current code addresses this

This classification should be completed by the development team in a subsequent review session.

---

**RFC Author:** Development Team  
**Date Opened:** 2026-02-08  
**Status:** Open for Classification  
**Version:** 1.0 (Draft)
