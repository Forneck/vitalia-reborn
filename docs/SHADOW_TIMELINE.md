# Shadow Timeline: Cognitive Future Simulation Layer

**RFC-0001 Implementation**  
**Status:** Implemented  
**Version:** 1.0  
**Date:** 2026-02-07

---

## Overview

The Shadow Timeline is a non-authoritative, non-causal, isolated simulation layer that allows autonomous entities (players and mobs) to internally explore possible future outcomes of actions without modifying the real world state.

This system formalizes what currently exists implicitly (player foresight and heuristic AI decisions) into a system-level construct that improves realism, plausibility, and decision quality.

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

---

## Cognitive Capacity Model

Each cognitive entity has:
- **K_e(t)**: Available cognitive capacity (0-1000)
- **cost(π)**: Cost to simulate a projection

### Constraints
- Sum of projection costs ≤ cognitive capacity
- Capacity regenerates over time (50 per tick)
- Entities differ in capacity based on emotional intelligence

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

The Shadow Timeline is integrated into the mob activity loop:

```c
void mobile_activity(void) {
    for (ch = character_list; ch; ch = next_ch) {
        // ... existing checks ...
        
        // Regenerate cognitive capacity each tick
        if (ch->ai_data) {
            shadow_regenerate_capacity(ch);
        }
        
        // Mob can use Shadow Timeline for decision-making
        // (integration examples to be added)
    }
}
```

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

## Future Extensions

The following are planned but not yet implemented:

### Phase 6 Features
- [ ] Causal Ledger integration (record actual past)
- [ ] Temporal Authority Layer hooks (decide what becomes real)
- [ ] Player-accessible foresight mechanics
- [ ] Multi-step planning with dependency chains
- [ ] Collaborative projections (group decision-making)
- [ ] Learning from projection accuracy

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

| File | Description |
|------|-------------|
| `src/shadow_timeline.h` | Header with types and API |
| `src/shadow_timeline.c` | Core implementation |
| `src/structs.h` | Added `cognitive_capacity` to `mob_ai_data` |
| `src/quest.c` | Initialize cognitive capacity in `init_mob_ai_data()` |
| `src/mobact.c` | Regenerate capacity in `mobile_activity()` |

---

## References

- RFC-0001: Shadow Timeline Design Document
- Emotion System (`mob_ai_data` emotions)
- Genetic Traits System (`mob_genetics`)
- Mob AI Goal System (`current_goal`)

---

## Authors

- Vitalia Reborn Design Team
- Implemented: 2026-02-07

---

## License

Part of Vitalia Reborn MUD engine.  
See main LICENSE.md for details.
