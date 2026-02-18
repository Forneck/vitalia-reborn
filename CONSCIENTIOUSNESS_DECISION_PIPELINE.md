# Conscientiousness Decision Pipeline Integration

## Overview

This document describes the integration of Conscientiousness (C) into the mob decision-making pipeline. The integration follows a **phased architectural approach** to avoid complexity explosion while delivering observable behavioral differentiation.

## Architectural Principle

**Conscientiousness operates at the point where intention becomes action, not during passive or mechanical processes.**

### What C Modulates
✅ **Deliberative decisions** - choices between alternatives
✅ **Impulse vs deliberation conflicts** - attack or intimidate?
✅ **Execution decisions** - act now or hesitate?

### What C Does NOT Modulate
❌ **Reflexive reactions** - memory-based retaliation, charmed rebellion
❌ **Mechanical processes** - damage calculation, emotion decay
❌ **Emergency responses** - heal dying master, automatic flee
❌ **Passive events** - special procedures running automatically

## Integration Phases

### Phase 1: Direct Decision Modulation ✅ COMPLETE

**Status**: Implemented in commit 2393590

**Integration Points**:
1. **Aggressive Mob Targeting** (`mobact.c:1905-1980`)
   - **Decision**: Attack victim or intimidate?
   - **C Effect**: High C reduces impulsive attacks
   - **Base impulse**: `1.0 - (victim_charisma / 20.0)`
   - **Modulation**: `impulse_prob = base * (1 - γC)`
   - **Observable**: Low C mobs attack more readily

2. **Shadow Timeline Attack Execution** (`mobact.c:686-724`)
   - **Decision**: Execute projected attack action?
   - **C Effect**: High C adds hesitation under arousal, can suppress impulse
   - **Base impulse**: `0.8` (projections are somewhat impulsive)
   - **Modulation**: Impulse reduction + reaction delay
   - **Observable**: High C mobs hesitate before executing Shadow Timeline attacks

**Helper Function**:
```c
static float apply_executive_control(
    struct char_data *ch,
    float base_impulse_prob,
    bool needs_delay,
    float *delay_ptr
)
```

Centralizes:
- Emotional arousal calculation
- Impulse modulation
- Reaction delay application
- Once-per-cycle guarantee

### Phase 2: Shadow Timeline Simulation Modulation (FUTURE)

**Status**: Not yet implemented - awaiting Phase 1 stability validation

**Planned Integration Points**:
1. **Simulation Depth Modulation**
   - C < 0.3 → 1-step projection
   - 0.3 ≤ C < 0.7 → 2-step projection
   - C ≥ 0.7 → 3-step projection
   - **Rationale**: High C agents plan further ahead

2. **Scenario Filtering**
   ```c
   if (risk_score > threshold * (1 - C))
       discard_scenario;
   ```
   - **Effect**: High C agents discard risky scenarios
   - **Observable**: Prudent vs impulsive planning

3. **Deliberative Delay in Simulation**
   - Apply delay ONCE per decision cycle, not per scenario
   - Prevents exponential delay compounding

**Why Phase 2 is Deferred**:
- Requires stable Phase 1 foundation
- Increases system complexity significantly
- Needs extensive testing to avoid emergent instability
- Shadow Timeline internals are complex; integration must be surgical

### Phase 3: Extended Cognitive Integration (FUTURE)

**Status**: Not yet implemented

**Potential Integration Points**:
1. **Group Formation Decisions** (with moral weight)
2. **Quest Acceptance Deliberation**
3. **Extended Emotional Social Response Modulation**
4. **Goal Abandonment Timing**

## Implementation Details

### Executive Control Pipeline

```
mobile_activity(mob) {
    1. Emotion updates (not C-modulated)
    
    2. DELIBERATIVE DECISION POINT:
       - Calculate arousal = calculate_emotional_arousal(mob)
       - Calculate impulse = apply_executive_control(mob, base_impulse, ...)
       
    3. DECISION EXECUTION:
       - if (rand() <= impulse_threshold)
           execute_action()
       - else
           suppress_action()
           
    4. Reaction delay (if high C + high arousal):
       - Probabilistic hesitation
       - Skip action this tick
}
```

### Integration Criterion

For each potential integration point, ask:

> **Is the mob reacting automatically, or is it choosing?**

- **Reacting automatically** → Do NOT apply C
- **Choosing between alternatives** → Apply C

### Once-Per-Cycle Guarantee

C modulation is applied **once** when making a decision, not at every micro-action. Multiple applications within the same tick would cause:
- Excessive delay
- Over-dampening
- Behavioral loops

## Behavioral Validation

### Expected Observable Differences

Create two test mobs:
- **Mob A**: C = 10 (low conscientiousness)
- **Mob B**: C = 90 (high conscientiousness)

Expose both to:
1. **High emotional arousal** (combat, threat)
2. **Aggressive targeting scenarios**
3. **Shadow Timeline decision points**

**Expected Results**:

| Scenario | Low C (10) | High C (90) |
|----------|------------|-------------|
| Aggressive targeting | Attacks impulsively | Shows restraint, intimidates more |
| High arousal | Immediate reactions | Hesitation, delayed responses |
| Shadow attack | Executes projection readily | May suppress or delay execution |
| Neutral state | Standard behavior | Standard behavior (no artificial slowness) |

### Validation Test Script

```
1. Create identical mobs except C value
2. Place in combat scenario
3. Measure:
   - Attack frequency
   - Hesitation occurrences
   - Impulse suppression rate
4. Expected: Measurable behavioral differentiation
```

If both mobs behave identically, C is not correctly integrated.

## Configuration

All C parameters are configurable via CEDIT:
- **Impulse Control Strength (γ)**: 0-200 (default: 100 = 1.0)
- **Reaction Delay Sensitivity (β)**: 0-200 (default: 100 = 1.0)
- **Moral Weight Amplification**: 0-200 (default: 100 = 1.0)
- **Debug Logging**: ON/OFF

Access: `cedit → Emotion Config → [I] Conscientiousness`

## Debug Logging

When enabled (`conscientiousness_debug = 1`), logs executive calculations:
```
CONSCIENTIOUSNESS: Mob guard (#1001) impulse modulation: 
  base=0.80, C=0.75, γ=1.00 -> result=0.20
```

Helps validate:
- Function is being called
- Values are correct
- Modulation is occurring

## Performance Considerations

**CPU Cost per Decision**:
- `calculate_emotional_arousal()`: Sum 7 emotions, normalize → O(1)
- `apply_conscientiousness_impulse_modulation()`: 3 float ops → O(1)
- `apply_conscientiousness_reaction_delay()`: 4 float ops → O(1)

**Total**: <10 float operations per deliberative decision
**Impact**: Negligible (<0.1% CPU overhead)

**Memory**: No dynamic allocation, uses existing personality struct

## Future Enhancements

### Phase 2 Priorities
1. **Validate Phase 1 stability** with production testing
2. **Measure behavioral differentiation** via metrics
3. **Gather builder feedback** on C effectiveness
4. **Then** proceed to Shadow Timeline simulation depth

### Phase 3 Considerations
- Moral weight integration needs active moral evaluation system
- Group dynamics need stable group formation mechanics
- Quest deliberation needs mature quest system

## Technical Debt & Known Limitations

1. **Delay Implementation**: Currently probabilistic hesitation, not true temporal delay
   - Future: Could use turn-based delay queue
   - Current approach avoids blocking game loop

2. **Shadow Timeline Integration**: Only at execution point, not simulation internals
   - By design for Phase 1
   - Phase 2 will extend into simulation depth

3. **Moral Weight**: Function exists but not yet integrated
   - Awaits active moral evaluation decision points
   - Placeholder for Phase 3

## References

- **Implementation**: `src/mobact.c` (Phase 1 integration)
- **Functions**: `src/utils.c` (executive control functions)
- **Configuration**: `src/cedit.c` (CEDIT interface)
- **Structures**: `src/structs.h` (personality and config)
- **Architecture**: This document (phased approach rationale)

## Conclusion

Phase 1 establishes the foundation for Conscientiousness as a **structural personality trait** that produces **observable behavioral differentiation** through **executive control modulation** of **deliberative decisions**.

The phased approach ensures:
- ✅ Architectural cleanliness
- ✅ Testable increments
- ✅ Controlled complexity growth
- ✅ Separation of concerns preserved

Next steps focus on validation and stability before extending to deeper cognitive layers.
