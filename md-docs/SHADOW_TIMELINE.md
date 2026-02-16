# Shadow Timeline: Cognitive Future Simulation Layer

**RFC-0003 COMPLIANT**  
**Implementation:** RFC-0001 (completed 2026-02-07)  
**Enhancement:** Adaptive Feedback (completed 2026-02-16)  
**Architecture:** RFC-0003 (normative 2026-02-09)  
**Status:** Production Ready  
**Version:** 1.1  

---

## Latest Enhancement: Adaptive Feedback System ✨

**NEW (2026-02-16):** The Shadow Timeline now includes a complete prediction-error feedback loop with valence-specific learning. This enhancement enables mobs to:
- Learn from prediction accuracy
- Adapt cognitive resource allocation based on environmental predictability
- Model biological loss aversion (threats weighted +30%, rewards -10%)
- Exhibit emergent vigilance adaptation

**📖 See [SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md](SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md) for complete details.**

---

## Compliance Statement

This implementation is **RFC-0003 COMPLIANT**, conforming to the normative specification defined in RFC-0003 "Shadow Timeline — Definition, Scope, and Authority".

See **RFC_0003_DEFINITION.md** for the authoritative architectural specification.

---

## Overview

The Shadow Timeline is a non-authoritative, observational domain that allows autonomous entities (players and mobs) to internally explore possible future outcomes of actions without modifying the real world state.

This system formalizes what currently exists implicitly (player foresight and heuristic AI decisions) into a system-level construct that improves realism, plausibility, and decision quality.

With the **Adaptive Feedback enhancement**, the system now closes the cognitive loop: **Project → Act → Observe → Compare → Adapt**

**Key RFC-0003 Principles:**
- **Non-authoritative** (§5.1) - Proposes possibilities, never asserts facts
- **Observational only** (§4.2) - Never mutates live world state
- **Invariant-safe** (§5.3) - Discards actions violating global invariants
- **Cognitively bounded** (§7.2) - Limited by entity capacity and entropy
- **Non-persistent** (§10.1) - No recording of predictions
- **Reset-bounded** (§8.2) - Respects temporal horizons

---

## Core Principles (Axioms)

### ST-1: Observational Only
The Shadow Timeline **must never mutate real world state**. All simulations are isolated:
- No real handlers execute
- No real triggers fire
- No global list modifications
- No persistence to disk

### ST-2: Invariant Preservation
No simulated action or projection may violate global invariants:
- No existence without prototype
- No simultaneous multi-location
- No impossible actions
- No violation of ontological constraints

### ST-3: Bounded Cognition
Shadow Timeline simulation is intentionally incomplete:
- No exhaustive search
- No full world enumeration
- No global foresight
- All projections are partial, heuristic, and truncated

### ST-4: Subjectivity
Different entities generate different Shadow Timelines under the same world state based on:
- Emotional state
- Genetic traits
- Memory
- Experience
- Cognitive fatigue

### ST-5: Non-determinism
Repeated evaluations may yield different projections. This is intentional and increases realism.

---

## Architecture

### Core Structures

#### `shadow_action`
Represents a candidate action to simulate:
```c
struct shadow_action {
    enum shadow_action_type type;  // Type of action
    void *target;                  // Target entity/object
    int direction;                 // For movement
    int cost;                      // Cognitive cost
};
```

#### `shadow_outcome`
Predicted outcome of an action:
```c
struct shadow_outcome {
    int score;                     // Quality score (-100 to +100)
    int hp_delta;                  // Estimated HP change
    int danger_level;              // Danger (0-100)
    int reward_level;              // Reward (0-100)
    
    // Emotional impact
    sh_int fear_change;
    sh_int anger_change;
    sh_int happiness_change;
    
    // State flags
    bool leads_to_combat;
    bool leads_to_death;
    bool achieves_goal;
    bool obvious;
};
```

#### `shadow_projection`
Complete projection: action + predicted trajectory:
```c
struct shadow_projection {
    struct shadow_action action;
    struct shadow_outcome outcome;
    int horizon;                   // Projection depth
    int total_cost;                // Cognitive cost
    time_t timestamp;
};
```

#### `shadow_context`
Shadow Timeline context for a cognitive entity:
```c
struct shadow_context {
    struct char_data *entity;
    struct shadow_projection *projections;
    int num_projections;
    int cognitive_budget;
    int horizon;
    bool active;
};
```

#### Adaptive Feedback Fields (mob_ai_data)
**NEW (2026-02-16):** Added to `struct mob_ai_data`:
```c
/* Shadow Timeline - Adaptive Feedback System */
int last_predicted_score;    /* Score predicted for chosen action (-100 to 100) */
int last_hp_snapshot;        /* HP before action execution */
int last_real_score;         /* Last evaluated real outcome (-100 to 100) */
bool last_outcome_obvious;   /* Whether the last outcome was obvious/predictable */
int recent_prediction_error; /* 0-100 smoothed novelty (exponentially smoothed) */
int attention_bias;          /* -50 to +50 long-term adaptation */
```

These fields enable:
- **Prediction-error learning**: Compare predicted vs. actual outcomes
- **Valence-specific adaptation**: Amplify threats, dampen rewards
- **Precision weighting**: Reduce learning from obvious outcomes
- **Vigilance modulation**: Adapt activation frequency to environment

---

## Cognitive Capacity Model

Each cognitive entity has:
- **K_e(t)**: Available cognitive capacity (0-1000)
- **cost(π)**: Cost to simulate a projection
- **recent_prediction_error**: Smoothed novelty signal (0-100) ✨ NEW
- **attention_bias**: Long-term vigilance adaptation (-50 to +50) ✨ NEW

### Constraints
- Sum of projection costs ≤ cognitive capacity
- Capacity regenerates over time (50 per tick)
- Entities differ in capacity based on emotional intelligence
- **Activation frequency adapts** to prediction accuracy ✨ NEW

### Capacity Formula
```
Initial capacity = 700 + (emotional_intelligence × 3)
Initial range: 500-1000 (clamped after calculation)
Runtime range: 0-1000 (capacity is spent and regenerated)
```

---

## API Usage

### Basic Usage

```c
// Initialize shadow context for an entity
struct shadow_context *ctx = shadow_init_context(mob);

if (ctx) {
    // Generate projections for available actions
    int num_projections = shadow_generate_projections(ctx);
    
    if (num_projections > 0) {
        // Select best action
        struct shadow_projection *best = shadow_select_best_action(ctx);
        
        if (best) {
            // Execute the real action based on projection
            switch (best->action.type) {
                case SHADOW_ACTION_MOVE:
                    // Perform actual movement
                    do_move(mob, best->action.direction, 0);
                    break;
                    
                case SHADOW_ACTION_ATTACK:
                    // Perform actual attack
                    hit(mob, best->action.target, TYPE_UNDEFINED);
                    break;
                    
                // ... handle other actions
            }
        }
    }
    
    // Free context when done
    shadow_free_context(ctx);
}
```

### Integration with Mob AI

The Shadow Timeline is integrated into the mob activity loop and controlled by the `MOB_SHADOWTIMELINE` flag:

```c
void mobile_activity(void) {
    for (ch = character_list; ch; ch = next_ch) {
        // ... existing checks ...
        
        // Regenerate cognitive capacity each tick
        if (ch->ai_data) {
            shadow_regenerate_capacity(ch);
        }
        
        // Shadow Timeline decision-making (only for flagged mobs)
        if (MOB_FLAGGED(ch, MOB_SHADOWTIMELINE) && ch->ai_data &&
            ch->ai_data->cognitive_capacity >= COGNITIVE_CAPACITY_MIN) {
            struct shadow_action action;
            
            if (mob_shadow_choose_action(ch, &action)) {
                // Execute chosen action (move, attack, flee, social, wait)
                // ... implementation details ...
            }
        }
    }
}
```

### Enabling Shadow Timeline for Mobs

To enable Shadow Timeline for a specific mob, add the `SHADOWTIMELINE` flag to the mob's action flags in the world files:

```
#12345
mob_name~
short description~
long description~
detailed description
~
ISNPC SHADOWTIMELINE    <-- Add SHADOWTIMELINE flag here
...
```

**Important**: Start with testing on a few select mobs before enabling widely. The Shadow Timeline is a cognitive system that:
- Uses computational resources (bounded by cognitive capacity)
- Makes mobs more intelligent but less predictable
- Should be tested for gameplay balance

### Sentinel Compatibility

The Shadow Timeline is fully compatible with `MOB_SENTINEL` flag. Sentinel mobs with Shadow Timeline will:

1. **Prioritize guard duty**: When at their post, they generate a `SHADOW_ACTION_GUARD` projection that encourages staying put
2. **Avoid abandoning posts**: Movement projections that take sentinels away from their posts receive heavy score penalties (-60)
3. **Return to post**: When away from their post, movement projections toward the post receive bonuses:
   - Moving directly to post: +70 score bonus
   - Moving closer to post: +40 score bonus
   - Moving away from post: -50 score penalty

This ensures that sentinels with Shadow Timeline maintain their posts while still being able to use cognitive simulation for combat decisions, social interactions, and other duties.

**Example**: A guard at a castle gate with both `MOB_SENTINEL` and `MOB_SHADOWTIMELINE` will:
- Stand guard when no threats are present (selecting `SHADOW_ACTION_GUARD`)
- Intelligently evaluate combat options if attacked (using Shadow Timeline combat projections)
- Return to post after handling threats (due to movement scoring bonuses)

### Simple Integration Example

Here's how a mob could use Shadow Timeline to make intelligent decisions:

```c
// In mob AI decision-making code
if (ch->ai_data && ch->ai_data->cognitive_capacity > COGNITIVE_CAPACITY_MIN) {
    // Use convenience function to get best action
    struct shadow_action action;
    
    if (mob_shadow_choose_action(ch, &action)) {
        // Execute the chosen action
        switch (action.type) {
            case SHADOW_ACTION_MOVE:
                perform_move(ch, action.direction, 1);
                break;
                
            case SHADOW_ACTION_ATTACK:
                if (action.target && !FIGHTING(ch)) {
                    hit(ch, (struct char_data *)action.target, TYPE_UNDEFINED);
                }
                break;
                
            case SHADOW_ACTION_FLEE:
                do_flee(ch, "", 0, 0);
                break;
                
            case SHADOW_ACTION_USE_ITEM:
                // Use item logic
                break;
                
            case SHADOW_ACTION_SOCIAL:
                // Perform social action
                break;
                
            case SHADOW_ACTION_WAIT:
                // Intentionally wait
                break;
                
            default:
                // Unknown action type
                break;
        }
    }
}
```

---

## Action Types

The Shadow Timeline supports projecting these action types:

| Action Type | Description | Typical Cost |
|-------------|-------------|--------------|
| `SHADOW_ACTION_MOVE` | Movement to another room | 10 |
| `SHADOW_ACTION_ATTACK` | Attack another entity | 10 |
| `SHADOW_ACTION_FLEE` | Flee from combat | 5 |
| `SHADOW_ACTION_USE_ITEM` | Use item/equipment | 10 |
| `SHADOW_ACTION_CAST_SPELL` | Cast a spell | 20 |
| `SHADOW_ACTION_SOCIAL` | Social interaction | 3 |
| `SHADOW_ACTION_TRADE` | Trading/shopping | 10 |
| `SHADOW_ACTION_QUEST` | Quest-related action | 10 |
| `SHADOW_ACTION_WAIT` | Wait/do nothing | 3 |
| `SHADOW_ACTION_FOLLOW` | Follow entity | 5 |
| `SHADOW_ACTION_GROUP` | Group formation | 5 |
| `SHADOW_ACTION_GUARD` | Stand guard at post (sentinels) | 5 |

### Implemented Actions in mobile_activity()

All action types are now implemented in the mob AI loop for mobs with the `MOB_SHADOWTIMELINE` flag:

- **SHADOW_ACTION_MOVE**: Executes `perform_move()` in the projected direction
- **SHADOW_ACTION_ATTACK**: Attacks the projected target if still valid and in same room
- **SHADOW_ACTION_FLEE**: Executes `do_flee()` if currently in combat
- **SHADOW_ACTION_SOCIAL**: Performs a simple friendly social interaction
- **SHADOW_ACTION_USE_ITEM**: Uses items from inventory (potions, food, etc.)
- **SHADOW_ACTION_CAST_SPELL**: Casts spells on allies (healing) or enemies (damage)
- **SHADOW_ACTION_TRADE**: Examines shopkeeper's wares (placeholder for full trading)
- **SHADOW_ACTION_QUEST**: Completes active quests when objectives are met
- **SHADOW_ACTION_FOLLOW**: Starts following another character
- **SHADOW_ACTION_GROUP**: Strengthens bonds with master/leader
- **SHADOW_ACTION_WAIT**: Intentionally skips action this tick (strategic waiting)
- **SHADOW_ACTION_GUARD**: Sentinel stands guard at their post, maintaining vigilance

All actions include proper validation and safety checks to prevent crashes or invalid state.

---

## Projection Generation

The system generates projections based on entity's context:

### In Combat
- Focus on combat projections (attack, flee)
- Short horizon (1 step)
- Higher priority for survival

### With Active Goal
- Focus on goal-relevant actions (movement, items)
- Medium horizon (3 steps)
- Bonus for goal achievement

### Exploring
- Diverse projections (movement, social, items)
- Default horizon (3 steps)
- Balanced scoring

---

## Subjectivity Implementation

Different entities perceive the same action differently:

### Fear Influence
- High fear amplifies perceived danger by 30%
- Reduces risk-taking behavior
- Penalties to aggressive actions

### Courage Influence
- High courage reduces perceived danger by 30%
- Increases risk-taking behavior
- Bonuses to aggressive actions

### Anger Influence
- High anger gives +20 bonus to attack actions
- Reduces social action scores

### Goal Influence
- Actions achieving current goal get +50 bonus
- Strong preference for goal-directed behavior

---

## Invariant Validation

Before simulating any action, the system validates:

1. **Existence Invariant**: Entity has valid prototype
2. **Location Invariant**: Destination room is valid
3. **State Invariant**: Entity state allows action
4. **Action Invariant**: Action is ontologically possible

Invalid actions are rejected with specific error codes:
- `ACTION_INVALID_TARGET`: Target doesn't exist
- `ACTION_INVALID_STATE`: State doesn't allow action
- `ACTION_VIOLATES_INVARIANT`: Would violate invariants
- `ACTION_IMPOSSIBLE`: Ontologically impossible

---

## Performance Considerations

### Memory Usage
- Each context: ~200 bytes
- Each projection: ~80 bytes
- Max 10 projections per context
- Total per entity: ~1KB

### CPU Usage
- Projections are lightweight heuristics
- No full state copies
- No recursive searches
- Bounded by cognitive capacity

### Optimization
- Obvious outcomes are cheaper to compute
- Simple actions cost less
- High emotional intelligence = 20% cost reduction

---

## Implemented Features

### ✅ Phase 1: Core Shadow Timeline (2026-02-07)
- [x] Projection generation and scoring
- [x] Cognitive capacity system
- [x] Invariant validation
- [x] Subjectivity modeling
- [x] Action execution integration

### ✅ Phase 2: Adaptive Feedback (2026-02-16)
- [x] **Learning from projection accuracy** ✨
- [x] Prediction-error feedback loop
- [x] Valence-specific adaptation (loss aversion)
- [x] Precision weighting (obvious outcomes)
- [x] Vigilance modulation (attention bias)
- [x] Cognitive resource conservation

**See [SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md](SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md) for details.**

---

## Future Extensions

The following are planned but not yet implemented:

### Phase 3 Features (Future)
- [ ] Causal Ledger integration (record actual past)
- [ ] Temporal Authority Layer hooks (decide what becomes real)
- [ ] Player-accessible foresight mechanics
- [ ] Multi-step planning with dependency chains
- [ ] Collaborative projections (group decision-making)
- [ ] Confidence-based precision weighting (continuous confidence scores)

---

## Debug and Testing

### Debug Logging
Shadow-only logging is available but disabled by default to avoid spam:
```c
shadow_log(ctx, "Generated 5 projections");
```

### Debug Dump
Dump context state for analysis:
```c
shadow_dump_context(ctx);
```

### Testing Recommendations
1. Test with mobs of varying emotional intelligence
2. Verify cognitive capacity regeneration
3. Check invariant validation catches invalid actions
4. Confirm no real state mutations occur
5. Profile performance with many active mobs

---

## Implementation Files

| File | Description | Version |
|------|-------------|---------|
| `src/shadow_timeline.h` | Header with types and API | 1.1 (adds feedback functions) |
| `src/shadow_timeline.c` | Core implementation | 1.1 (adds feedback logic) |
| `src/structs.h` | Cognitive fields in `mob_ai_data` | 1.1 (adds feedback fields) |
| `src/quest.c` | Initialize cognitive capacity in `init_mob_ai_data()` | 1.0 |
| `src/mobact.c` | Capacity regen and feedback integration | 1.1 (adds feedback evaluation) |

### New Functions (v1.1)
- `shadow_evaluate_real_outcome()` - Compute real outcome score
- `shadow_update_feedback()` - Update prediction error and attention bias

---

## Documentation Files

| File | Description |
|------|-------------|
| `md-docs/SHADOW_TIMELINE.md` | Main Shadow Timeline documentation (this file) |
| `md-docs/SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md` | Adaptive feedback system details ✨ NEW |
| `md-docs/RFC_0001_IMPLEMENTATION_SUMMARY.md` | Implementation summary |
| `md-docs/RFC_0003_DEFINITION.md` | Normative specification |

---

## References

- **RFC-0003:** Shadow Timeline — Definition, Scope, and Authority (normative specification)
- **RFC-0001:** Shadow Timeline Implementation Summary (design document)
- **RFC-0002:** Shadow Timeline Questions (architectural analysis)
- **SHADOW_TIMELINE_ADAPTIVE_FEEDBACK.md:** Feedback system documentation ✨ NEW
- **docs/INVARIANTS_ANALYSIS.md:** Invariant system documentation
- Emotion System (`mob_ai_data` emotions)
- Genetic Traits System (`mob_genetics`)
- Mob AI Goal System (`current_goal`)

---

## Compliance and Verification

### RFC-0003 Compliance Verification

This implementation has been verified as **RFC-0003 COMPLIANT**:

✅ Domain separation (§4.1) - Separate module, no embedding  
✅ Zero temporal authority (§5.1) - Proposes possibilities only  
✅ Invariant enforcement (§5.3) - Validates before projection  
✅ Autonomous entity restriction (§6.1) - Cognitive entities only  
✅ Cognitive cost modeling (§7.2) - Capacity system implemented  
✅ Bounded horizon (§8.1) - Limited by capacity and entropy  
✅ Reset boundaries (§8.2) - Temporal horizons respected  
✅ Non-persistent (§10.1) - Ephemeral projections only  
✅ Past non-influence (§11) - Forward-only observation  

For detailed compliance assessment, see **RFC_0003_DEFINITION.md §18**.

### Compliance Markers

All Shadow Timeline code includes compliance markers:
- `/* RFC-0003 COMPLIANT */` - Fully conformant
- References to specific RFC-0003 sections in comments

---

## Authors

- Vitalia Reborn Design Team
- Implemented: 2026-02-07

---

## License

Part of Vitalia Reborn MUD engine.  
See main LICENSE.md for details.
