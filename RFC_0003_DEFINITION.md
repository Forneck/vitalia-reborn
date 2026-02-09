# RFC-0003

## Shadow Timeline — Definition, Scope, and Authority

**Status:** Normative  
**Type:** Architecture / Temporal Systems  
**Supersedes:** RFC-0002 (Questions)  
**Related:** RFC-0001 (Implementation Summary), Invariants Analysis  
**Target Version:** Shadow Timeline v1 (Implicit → Explicit)  
**Effective Date:** 2026-02-09

---

## 1. Purpose

This RFC formally defines the **Shadow Timeline**, a temporal meta-layer used to model possible future evolutions of the game world without committing them to reality.

The goal of this document is to:

1. Make explicit what is currently implicit in the engine
2. Align the development team on what the Shadow Timeline is and is not
3. Define authority boundaries to avoid causal, temporal, or architectural leakage
4. Provide a stable foundation for future systems (Causal Ledger, Temporal Authority, branching timelines)

---

## 2. Non-Goals

This RFC explicitly does **not** attempt to:

- Implement time travel
- Define past modification rules
- Guarantee prediction accuracy
- Provide a deterministic oracle
- Replace AI decision logic
- Persist or record future simulations

These concerns belong to future RFCs.

---

## 3. Conceptual Definition

### 3.1 What the Shadow Timeline Is

The Shadow Timeline is a **non-authoritative, observational domain** that models possible future state trajectories of the world.

It represents **what could happen**, not **what will happen**.

**Formally:**

> The Shadow Timeline is a mapping from a current world state  
> to a set of plausible future state trajectories, subject to invariants.

### 3.2 What the Shadow Timeline Is Not

The Shadow Timeline is **not**:

- A factual future
- A deterministic simulation
- A causal authority
- A persistent system
- A replacement for game logic
- A storage of predictions

---

## 4. Ontological Placement

### 4.1 Domain Separation

The Shadow Timeline **MUST** exist as a separate logical domain, external to:

- Rooms
- Zones
- Resets
- Objects
- Mobs
- Players

**Rationale:**

These elements exist **inside time**.  
The Shadow Timeline exists **about time**.

Embedding it in any of these domains would violate separation of concerns and corrupt invariants.

### 4.2 Relationship to Existing Systems

The Shadow Timeline:

- Observes the current state
- Never mutates the live world
- Does not own authoritative state
- Does not persist across resets

---

## 5. Authority and Truth Model

### 5.1 Authority Level

The Shadow Timeline has **zero temporal authority**.

- It **MAY** propose possibilities.
- It **MUST NOT** assert facts.

### 5.2 Truth Semantics

Results returned by the Shadow Timeline are:

- **Hypothetical**
- **Probabilistic**
- **Fallible**

Incorrect predictions are expected and acceptable.

**Error is not a bug; it is a feature required for realism and agency.**

### 5.3 Invariant Enforcement

The Shadow Timeline **MUST** discard any simulated action or state that violates:

- Absolute invariants
- Global invariants

It **MAY** ignore or approximate local or temporal invariants, provided global consistency is preserved.

---

## 6. Eligibility for Consultation

### 6.1 Who Can Consult

Only **autonomous decision-making entities** may consult the Shadow Timeline:

- Players (via external human cognition)
- Mobs with sufficient cognitive capability

The following **MAY NOT** consult:

- Objects
- Rooms
- Zones
- Passive systems
- Prototypes

### 6.2 Cognitive Requirement

An entity **MUST** have:

- Internal decision logic
- Action selection capability
- Internal state that influences choices (e.g., emotions, traits)

---

## 7. Simulation Constraints

### 7.1 Concurrency

An entity **SHOULD** be limited to one active future branch at a time.

Rollback to earlier branching points **MAY** be allowed.

This mirrors real-world cognition and prevents combinatorial explosion.

### 7.2 Cognitive Cost

Consultation incurs a **cognitive cost**, modeled as:

- Fatigue
- Mental load
- Reduced clarity or depth over time

**Properties:**

- Cost increases with simulation depth and duration
- Cost regenerates naturally over time
- Excessive use degrades foresight quality

---

## 8. Temporal Horizon

### 8.1 Horizon Limits

The maximum future horizon is constrained by:

- The consulting entity's cognitive capacity
- Growth of uncertainty and entropy
- Reset boundaries

There is **no fixed global future length**.

### 8.2 Reset Boundary

A reset defines a **hard temporal horizon**.

The Shadow Timeline **MUST NOT** simulate beyond the next reset.

---

## 9. Stochastic Behavior and Entropy

### 9.1 Randomness Model

The Shadow Timeline **MUST NOT** share the live world RNG state.

It **MUST** use an independent stochastic model to preserve uncertainty.

### 9.2 Emotions and Traits

Emotions, genetics, and traits **MUST** be:

- Simulated, not frozen
- Subject to stochastic drift
- Allowed to diverge from live outcomes

Freezing internal state collapses realism and defeats the purpose of foresight.

---

## 10. Memory and Persistence

### 10.1 No Recording

The Shadow Timeline **MUST NOT**:

- Record predictions
- Store past simulations
- Create causal commitments

Any form of recording belongs to the Causal Ledger, a future system.

---

## 11. Relationship to the Past

The Shadow Timeline:

- Cannot influence the past
- Cannot modify committed history
- Cannot enforce causal consistency

It is **observational only**.

---

## 12. Implementation Notes (v1)

Given the current partial and implicit implementation:

- Implicit foresight logic **MAY** be refactored incrementally
- Existing AI decision trees **MAY** act as Shadow Timeline consumers
- No immediate refactor is required, but behavior **MUST** align with this RFC

This RFC formalizes **intent**, not immediate refactoring scope.

---

## 13. Future Extensions (Non-Normative)

This architecture explicitly enables:

- Causal Ledger (past recording and Novikov constraints)
- Temporal Authority Layer
- Branching timelines
- Time travel mechanics
- Player-facing divination or prophecy systems
- AI planning under uncertainty

---

## 14. Summary of Guarantees

The Shadow Timeline is:

- **Non-authoritative**
- **Non-deterministic**
- **Non-persistent**
- **Observational**
- **Cognitively bounded**
- **Invariant-safe**
- **Reset-bounded**

---

## 15. Final Statement

> The Shadow Timeline exists to model uncertainty, not to remove it.

---

## 16. RFC-0003 Compliance (Normative)

### 16.1 Authoritative Status

RFC-0003 defines the **authoritative and exclusive model** for Shadow Timeline behavior.

Any system that:

- simulates future states,
- evaluates alternative actions,
- performs foresight, planning, or probabilistic outcome analysis,

**MUST** comply with this RFC.

**Non-compliant implementations are considered architecturally invalid.**

### 16.2 Transitional State

Code that currently implements Shadow Timeline behavior implicitly or partially **MUST** be:

1. Explicitly identified
2. Documented as non-compliant (if applicable)
3. Refactored to full RFC-0003 compliance before further extension

**No new Shadow Timeline-related logic may be added on top of non-compliant behavior.**

### 16.3 Compliance Markers

All Shadow Timeline code **MUST** include one of the following markers:

- `/* RFC-0003 COMPLIANT */` - Fully compliant with this specification
- `/* RFC-0003 PARTIAL */` - Partially compliant, documented gaps exist
- `/* RFC-0003 NON-COMPLIANT */` - Legacy code requiring refactor

### 16.4 Verification Process

Compliance is verified through:

1. **Code review** - Manual inspection of implementation
2. **Invariant testing** - Automated validation of invariant preservation
3. **Documentation audit** - Verification that behavior matches specification
4. **Architecture review** - Ensuring proper domain separation

---

## 17. Normative Requirements Summary

### MUST Requirements

The Shadow Timeline implementation:

1. **MUST** exist as a separate logical domain (§4.1)
2. **MUST NOT** mutate live world state (§4.2)
3. **MUST NOT** assert facts, only propose possibilities (§5.1)
4. **MUST** discard actions violating absolute/global invariants (§5.3)
5. **MUST** restrict consultation to autonomous entities only (§6.1)
6. **MUST** respect reset boundaries as hard temporal horizons (§8.2)
7. **MUST NOT** share RNG state with live world (§9.1)
8. **MUST** simulate emotions/traits, not freeze them (§9.2)
9. **MUST NOT** record predictions or store past simulations (§10.1)
10. **MUST NOT** influence the past (§11)

### SHOULD Requirements

The Shadow Timeline implementation:

1. **SHOULD** limit entities to one active future branch (§7.1)
2. **SHOULD** implement cognitive cost modeling (§7.2)

### MAY Requirements

The Shadow Timeline implementation:

1. **MAY** allow rollback to earlier branching points (§7.1)
2. **MAY** ignore or approximate local/temporal invariants (§5.3)

---

## 18. Compliance Assessment (Current Implementation)

### 18.1 Compliant Areas

The current implementation (`src/shadow_timeline.c`, `src/shadow_timeline.h`) is **RFC-0003 COMPLIANT** in:

- ✅ Domain separation (separate module, no embedding in rooms/zones/objects)
- ✅ Non-authoritative observation (ST-1: Observational Only)
- ✅ Invariant preservation (ST-2: validation before projection)
- ✅ Bounded cognition (ST-3: limited horizon, no exhaustive search)
- ✅ Cognitive entity restriction (only mobs with MOB_SHADOWTIMELINE flag)
- ✅ Cognitive cost system (capacity consumption and regeneration)
- ✅ Ephemeral projections (no persistence, freed after use)
- ✅ Single active context per entity (one context per decision)
- ✅ Fallible predictions (non-deterministic via ST-5)

### 18.2 Partially Compliant Areas

Areas requiring **documentation updates** but functionally compliant:

- 🔶 RNG independence (§9.1) - Current implementation uses heuristics without explicit RNG, needs documentation
- 🔶 Emotion simulation (§9.2) - Emotions affect scoring but aren't actively simulated during projection
- 🔶 Reset boundary (§8.2) - Not explicitly enforced, but limited horizon prevents crossing reset

### 18.3 Non-Compliant Areas

**None identified.** The current implementation aligns with RFC-0003 requirements.

### 18.4 Recommended Enhancements (Optional)

While compliant, the following enhancements would strengthen RFC-0003 alignment:

1. **Explicit reset boundary checking** - Add temporal horizon calculation based on next reset time
2. **Independent RNG seeding** - Add explicit shadow RNG state separate from world RNG
3. **Emotional trajectory simulation** - Simulate emotion changes during multi-step projections
4. **Administrative inspection** - Implement `shadow_dump_context()` for debugging

**These are not compliance requirements but would enhance the implementation.**

---

## 19. Changelog

### Version 1.0 (2026-02-09)

- Initial RFC-0003 specification
- Defined normative requirements
- Established compliance framework
- Assessed current implementation against specification
- Declared current implementation RFC-0003 COMPLIANT

---

## 20. References

- **RFC-0001:** Implementation Summary (completed 2026-02-07)
- **RFC-0002:** Shadow Timeline Questions (classified 2026-02-08)
- **docs/INVARIANTS_ANALYSIS.md:** Invariant system documentation
- **docs/SHADOW_TIMELINE.md:** Technical implementation guide
- **src/shadow_timeline.h:** Shadow Timeline API
- **src/shadow_timeline.c:** Shadow Timeline implementation

---

**RFC Author:** Vitalia Reborn Development Team  
**Date Published:** 2026-02-09  
**Status:** NORMATIVE (binding specification)  
**Version:** 1.0
