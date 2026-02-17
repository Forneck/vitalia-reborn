# Memory-Based Moral Learning - Usage Example

## Quick Start Guide

This guide demonstrates how mobs learn from moral decisions in Vitalia Reborn.

## Example 1: Good Guard Learns Compassion

### Setup
```
# Spawn a good-aligned guard
mob load 3096  # Maria, the newbie instructor (alignment +166)
stat maria     # Check initial emotions
```

### Scenario: Guard Attacks Hungry Thief

**Step 1:** Guard attacks thief stealing food
```c
// In code, when guard attacks:
moral_record_action(guard, thief, MORAL_ACTION_ATTACK);
```

**Result:**
- Moral judgment: GUILTY (attacking someone with justified need)
- Shame increases: 15 → 45 (+30)
- Regret: 60
- Memory stored with moral data

**Memory Entry:**
```
[1 min ago] Player#12345 Attacked           [MAJOR]
  Emotions: Fear:20 Anger:30 Happy:40 Sad:20 Shame:45 ...
  Moral: Attack, Guilty, Blame:75, Severity:50, Regret:60
```

### Step 2: Guard Considers Second Attack

```c
// Shadow Timeline evaluates action cost
int cost = moral_evaluate_action_cost(guard, thief2, MORAL_ACTION_ATTACK);
// Returns: -75 (base) + -45 (learned bias) = -120
```

**What Happened:**
- `moral_get_learned_bias()` found the previous guilty attack
- Applied negative bias of -45 based on regret
- Guard is now discouraged from attacking again

### Step 3: Third Consideration - Learned Avoidance

After 2 more guilty attacks with high regret:

```c
if (moral_has_learned_avoidance(guard, MORAL_ACTION_ATTACK)) {
    // TRUE - guard has learned to avoid this action
    return;  // Skip the attack
}
```

**Final Result:** Guard learned compassion through experience!

## Example 2: Evil Assassin Learns Caution

### Setup
```
# Spawn evil assassin (alignment -800)
```

### Scenario: Assassin Betrays Ally

**Step 1:** Successful betrayal
- Judgment: GUILTY (betrayal always guilty)
- Shame: 10 → 15 (+5, minimal for evil mob)
- Regret: 10 (low due to success and low shame)

**Step 2:** Betrayal goes wrong (loses fight)
- Judgment: GUILTY
- Outcome severity: 80 (high HP loss)
- Regret: 45 (outcome-based, not moral)

**Step 3:** Next betrayal consideration
- Learned bias: -30 (practical caution, not moral)
- **Still willing to betray if conditions are better**

**Key Difference:** Evil mob learned strategic caution, not morality.

## Example 3: Debugging Moral Learning

### View Mob's Moral Memory

```
stat maria
```

Output shows:
```
Emotion Memory: (5/10 slots used)
  [2 min ago] Player#12345 Attacked           [MAJOR]
      Emotions: Fear:20 Anger:30 Happy:40 Sad:20 Shame:45
      Moral: Attack, Guilty, Blame:75, Severity:50, Regret:60
  
  [5 min ago] Player#12346 Attacked           [MAJOR]
      Emotions: Fear:25 Anger:35 Happy:35 Sad:25 Shame:60
      Moral: Attack, Guilty, Blame:80, Severity:60, Regret:75
```

### Check Learning Pattern

```c
int guilty = 0, innocent = 0;
moral_get_action_history(maria, MORAL_ACTION_ATTACK, &guilty, &innocent);
// guilty = 2, innocent = 0

int bias = moral_get_learned_bias(maria, MORAL_ACTION_ATTACK);
// bias = -65 (strong negative)

bool avoid = moral_has_learned_avoidance(maria, MORAL_ACTION_ATTACK);
// avoid = TRUE (2 guilty, 0 innocent)
```

## Example 4: Integration with Gameplay

### When to Call moral_record_action()

```c
// After combat attack
void perform_violence(struct char_data *ch, struct char_data *vict) {
    hit(ch, vict, TYPE_UNDEFINED);
    
    // Record moral judgment for learning
    if (IS_NPC(ch) && ch->ai_data) {
        moral_record_action(ch, vict, MORAL_ACTION_ATTACK);
    }
}

// After stealing
void perform_steal(struct char_data *ch, struct char_data *vict) {
    // ... steal logic ...
    
    if (IS_NPC(ch) && ch->ai_data) {
        moral_record_action(ch, vict, MORAL_ACTION_STEAL);
    }
}

// After healing
void perform_heal(struct char_data *ch, struct char_data *vict) {
    // ... healing logic ...
    
    if (IS_NPC(ch) && ch->ai_data) {
        moral_record_action(ch, vict, MORAL_ACTION_HEAL);
    }
}
```

### Shadow Timeline Automatically Uses Learning

```c
// When mob considers actions, learning is applied automatically
struct shadow_context *ctx = shadow_init_context(mob);
shadow_generate_projections(ctx);  
// ^ Calls moral_evaluate_action_cost which includes learned biases
struct shadow_projection *best = shadow_select_best_action(ctx);
```

## Memory Decay Timeline

| Time        | Weight | Effect                                    |
|-------------|--------|-------------------------------------------|
| < 5 min     | 100%   | Fresh memory, full influence             |
| 5-15 min    | 80%    | Recent, strong influence                 |
| 15-30 min   | 60%    | Fading, moderate influence               |
| 30-60 min   | 40%    | Old, weak influence                      |
| > 60 min    | 0%     | Forgotten, no influence                  |

## Learning Thresholds

### Learned Avoidance Triggers When:
- At least 2 guilty judgments with 0 innocent ones, OR
- At least 3 instances with regret > 70

### Bias Calculation:
- Guilty action: Bias = -blameworthiness - (regret bonus) - (severity bonus)
- Innocent action: Bias = +30 + (happiness bonus) - (regret penalty)

## Testing Moral Learning

### Test Script Example

```c
// Spawn good guard
struct char_data *guard = load_mobile(3096);

// Force multiple attacks to test learning
for (int i = 0; i < 5; i++) {
    struct char_data *victim = load_mobile(3001);
    
    // Record action
    moral_record_action(guard, victim, MORAL_ACTION_ATTACK);
    
    // Check learning progress
    int bias = moral_get_learned_bias(guard, MORAL_ACTION_ATTACK);
    log("Attack %d: Learned bias = %d", i+1, bias);
    
    // Extract victim
    extract_char(victim);
    
    // Wait to simulate time passing
    sleep(60);
}

// Final check
if (moral_has_learned_avoidance(guard, MORAL_ACTION_ATTACK)) {
    log("Guard learned to avoid attacking!");
}
```

## Expected Behaviors

### Good Guard After 3 Guilty Attacks:
- Learned bias: -65 to -85
- Learned avoidance: TRUE
- Behavior: Refuses to attack unless strongly justified
- Alternative: Defends area, doesn't pursue

### Evil Assassin After Mixed Outcomes:
- Learned bias: -20 to -40 (practical only)
- Learned avoidance: FALSE (still willing)
- Behavior: More cautious, picks better targets
- Alternative: Waits for advantage

### Neutral Merchant After Helping:
- Learned bias: +35 to +50
- Behavior: More likely to help again
- Mechanism: Positive reinforcement from happiness

## Implementation Notes

### Performance
- Learning calculation: O(10) - scans 10 memory slots
- Called once per Shadow Timeline projection
- Negligible impact: ~0.1ms per call

### Memory Usage
- +10 bytes per emotion_memory entry
- +100 bytes per mob (10 memories)
- +600 KB for 6000 mobs total

### Thread Safety
- All learning functions are stateless
- Safe for concurrent mob AI ticks
- No global state modification

## Conclusion

Memory-based moral learning creates adaptive, realistic mob behavior:
- **Dynamic**: Mobs change based on experience
- **Alignment-aware**: Different learning for good/evil/neutral
- **Efficient**: Minimal performance impact
- **Debuggable**: Full visibility in stat command

Mobs now feel like living creatures that learn and grow from their experiences!
