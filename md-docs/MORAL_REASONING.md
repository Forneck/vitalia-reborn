# Moral Reasoning System

**Version:** 1.3  
**Status:** Production Ready  
**Integration Date:** 2026-02-14  
**Emotion Integration:** 2026-02-14  
**Memory-Based Learning:** 2026-02-16  
**Group Moral Dynamics:** 2026-02-17

## Overview

The Moral Reasoning System implements Shultz & Daley's rule-based model for qualitative moral judgment with full integration of the 20-emotion mob AI system, memory-based learning, and group moral dynamics. It enables mobs in Vitalia Reborn to evaluate the moral implications of their actions individually and collectively, learn from past moral decisions, navigate peer pressure and conformity, and build group moral reputations, leading to highly realistic, adaptive, socially-aware, and nuanced decision-making behavior.

## Theory Background

### Source Material

Based on research by T.R. Shultz & J.M. Daley:

- **Darley, J.M. & Shultz, T.R. (1990).** Moral rules: Their content and acquisition. *Annual Review of Psychology, 41*, 525-556.

- **Shultz, T.R. (1990).** A rule base model of judging harm-doing. In *Proceedings of the Twelfth Annual Conference of the Cognitive Science Society* (pp. 229-236). Cambridge, MA: Lawrence Erlbaum.

### Model Purpose

This model simulates how ordinary persons (down to approximately age five) reason about harm-doing situations. It evaluates responsibility, blameworthiness, and guilt through a structured set of moral predicates and inference rules.

## Architecture

### Core Components

1. **Moral Scenario** (`struct moral_scenario`)
   - Encodes all relevant predicates for a harm-doing situation
   - Includes causation, intent, responsibility, and justification factors

2. **Moral Judgment** (`struct moral_judgment`)
   - Contains the evaluation result (guilty/innocent)
   - Provides reasoning breakdown (responsibility, blameworthiness scores)
   - Tracks which moral rules were satisfied

3. **Reasoning Engine** (moral_reasoner.c)
   - Implements Prolog-style inference rules in C
   - Evaluates moral predicates hierarchically
   - Produces guilt determinations with confidence scores

### Key Predicates

#### Causation
- `cause(X)` - Entity caused harm
- `produce_harm(X,y/n)` - Directly produced harmful outcome
- `necessary_for_harm(X,y/n)` - Action was necessary for harm
- `sufficient_for_harm(X,y/n)` - Action was sufficient for harm

#### Intent and Mental State
- `intend(X)` - Entity intended the harm
- `strong_intend(X)` - Deliberate planning and execution
- `weak_intend(X)` - Foreseeable but not primary goal
- `mental_state(X,M)` - Mental state: intend, reckless, negligent, or neither
- `plan_known(X,y/n)` - Plan was known beforehand
- `plan_include_harm(X,y/n)` - Harm was part of the plan
- `harm_caused_as_planned(X,y/n)` - Outcome matched plan

#### Responsibility
- `responsible(X)` - Entity is morally responsible
- `accident(X)` - Harm was accidental
- `voluntary(X)` - Action was voluntary (not coerced)
- `external_force(X,y/n)` - External force compelled action
- `careful(X,y/n)` - Entity was careful/cautious

#### Foreseeability
- `foreseeable(X)` - Harm was foreseeable
- `foreseeability(X,F)` - Foreseeability level: n, low, high
- `reckless(X)` - High foreseeability but careless
- `negligent(X)` - Low foreseeability but careless

#### Intervention and Causation Chains
- `intervening_cause(X)` - Another cause intervened
- `intervening_contribution(X,y/n)` - Third party contributed
- `foresee_intervention(X,y/n)` - Intervention was foreseeable
- `someone_else_cause_harm(X,y/n)` - Another entity caused harm
- `external_cause(X,y/n)` - External factor caused harm

#### Vicarious Responsibility
- `vicarious(X)` - Vicariously responsible through subordinate
- `outrank_perpetrator(X,y/n)` - Entity outranks actual perpetrator
- `control_perpetrator(X,y/n)` - Entity controlled perpetrator
- `monitor(X,y/n)` - Entity monitored the situation

#### Justification
- `justified(X)` - Action was morally justified
- `achieve_goal(X,y/n)` - Goal was achieved
- `goal_outweigh_harm(X,y/n)` - Goal's value outweighed harm
- `goal_achieveable_less_harmful(X,y/n)` - Less harmful alternative existed

#### Harm Magnitude
- `severity_harm(X,N)` - Numeric severity of harm (0-10)
- `benefit_victim(X,N)` - Benefit to victim (0-10)
- `benefit_protagonist(X,y/n)` - Protagonist benefited

### Inference Rules

#### Top-Level Guilt Determination

```prolog
guilty(X) :- blameworthy(X).
guilty(X) :- vicarious_blame(X).
```

An entity is guilty if they are directly blameworthy OR vicariously blameworthy through a subordinate.

#### Blameworthiness

```prolog
blameworthy(X) :-
    responsible(X),
    NOT justified(X),
    severity_harm(X,H),
    benefit_victim(X,V),
    greater(H,V).
```

Entity is blameworthy if:
- They are responsible for the harm
- Action was not justified
- Harm severity exceeds victim benefit

#### Responsibility

```prolog
responsible(X) :-
    cause(X),
    NOT accident(X),
    voluntary(X),
    foreseeable(X),
    NOT intervening_cause(X).
```

Entity is responsible if:
- They caused the harm
- It was not an accident
- Action was voluntary
- Harm was foreseeable
- No unforeseeable intervention occurred

#### Causation

```prolog
cause(X) :- produce_harm(X,y).
cause(X) :- necessary_for_harm(X,y).
cause(X) :- sufficient_for_harm(X,y).
```

#### Intent Hierarchy

```prolog
intend(X) :- strong_intend(X).
intend(X) :- weak_intend(X).

strong_intend(X) :- mental_state(X,intend).
strong_intend(X) :-
    plan_known(X,y),
    plan_include_harm(X,y),
    harm_caused_as_planned(X,y).

weak_intend(X) :-
    weak_intend1(X),
    NOT reckless(X),
    NOT negligent(X).
```

Strong intent requires explicit planning or mental state. Weak intent requires conditions that exclude recklessness/negligence.

#### Accident Determination

```prolog
accident(X) :-
    NOT intend(X),
    NOT reckless(X),
    NOT negligent(X).
```

Accident occurs when there's no intent, recklessness, or negligence.

#### Justification

```prolog
justified(X) :-
    achieve_goal(X,y),
    goal_outweigh_harm(X,y),
    NOT goal_achieveable_less_harmful(X,y).
```

Action is justified if:
- Goal was achieved
- Goal's value outweighs harm
- No less harmful alternative existed

## Integration with MUD Systems

### Shadow Timeline Integration

The moral reasoning system integrates with the Shadow Timeline (RFC-0003) cognitive simulation system to evaluate potential actions before execution.

**Integration Point:** `shadow_timeline.c::score_projection_for_entity()`

```c
/* Evaluate moral cost if applicable */
if (action_type >= 0) {
    moral_cost = moral_evaluate_action_cost(ch, target, action_type);
    score += moral_cost;
}
```

**Moral Action Types:**
- `MORAL_ACTION_ATTACK` - Attacking another entity
- `MORAL_ACTION_STEAL` - Theft/stealing
- `MORAL_ACTION_HELP` - Helping another
- `MORAL_ACTION_HEAL` - Healing another
- `MORAL_ACTION_TRADE` - Trading/commercial action
- `MORAL_ACTION_DECEIVE` - Deception/lying
- `MORAL_ACTION_SACRIFICE_SELF` - Self-sacrifice
- `MORAL_ACTION_ABANDON_ALLY` - Abandoning an ally
- `MORAL_ACTION_BETRAY` - Betrayal
- `MORAL_ACTION_DEFEND` - Self-defense

**Scoring Impact:**
- Guilty actions receive **negative moral cost** (penalizing the action)
- Innocent/helpful actions receive **positive moral cost** (encouraging the action)
- Cost is modified by mob's alignment:
  - Good-aligned: 2x penalty for guilty actions
  - Evil-aligned: 0.5x penalty (less concerned)

### Alignment System Integration

Moral judgments influence mob alignment over time:

```c
void moral_adjust_alignment(struct char_data *ch, struct moral_judgment *judgment)
```

- Guilty actions shift alignment toward evil (-1000)
- Responsible innocent actions shift toward good (+1000)
- Change magnitude based on blameworthiness score

### Reputation System Integration

Moral judgments affect mob reputation (0-100 scale):

```c
void moral_adjust_reputation(struct char_data *ch, struct moral_judgment *judgment)
```

- **Evil-aligned mobs:** Gain reputation from guilty acts
- **Good-aligned mobs:** Lose reputation from guilty acts, gain from moral acts
- **Neutral mobs:** Minor adjustments

### Emotion System Integration

**Version 1.1** - The moral reasoning system is fully integrated with the 20-emotion mob AI system:

```c
void moral_adjust_emotions(struct char_data *ch, struct moral_judgment *judgment)
```

#### Emotions Influence Moral Decisions

**During Action Evaluation:**
- **Compassion** (>70): Increases perceived harm severity by 20%, enhances monitoring
- **Anger** (>70): Reduces carefulness, increases recklessness
- **Fear** (>50): Increases carefulness by 20%
- **Greed** (>60): Marks actions as self-beneficial
- **Love/Loyalty** (>70): Dramatically increases harm severity (2x) when harming loved ones
- **Pride** (>70): Reduces moral sensitivity by 20%
- **Shame** (>50): Increases aversion to guilty actions
- **Disgust** (>60): Amplifies aversion to betrayal and deception

**Moral Cost Modifiers by Emotion:**

For **Guilty Actions**:
- Shame: Additional -1 to -25 penalty (based on shame level)
- Compassion: Additional -1 to -13 penalty (based on compassion level)
- Pride: 20% reduction in moral concern
- Anger: Up to 40% reduction when very angry (>80)
- Disgust: Additional penalty for betrayal/deception

For **Helpful Actions**:
- Compassion: +1 to +13 bonus for helping/healing
- Love: +1 to +7 bonus for helping/healing
- Loyalty: +1 to +15 bonus for self-sacrifice and defense
- Courage: +1 to +13 bonus for defense

#### Emotions Updated by Moral Judgments

**After Guilty Actions:**
- **Shame** increases (10-15 for good mobs, 4 for evil mobs)
- **Disgust** increases (+5 for severe actions)
- **Happiness** decreases (proportional to shame)
- **Sadness** increases (for severe harm)
- **Pride** decreases (for good-aligned mobs)

**After Moral Actions:**
- **Pride** increases (+5-8 for responsible actions)
- **Happiness** increases (half of pride increase)
- **Shame** decreases (fades with good deeds)
- **Compassion** increases (+2 for altruistic acts)

**Vicarious Responsibility:**
- **Shame** +5 (failing to prevent subordinate's harm)
- **Anger** +3 (at the subordinate)

#### Emotion-Driven Behavior Examples

1. **High Compassion + Low Shame** → More likely to help, heal, sacrifice
2. **High Anger + Low Fear** → More likely to attack despite moral cost
3. **High Shame + High Compassion** → Strong aversion to harmful actions
4. **High Pride + Low Shame** → Less concerned with moral judgments
5. **High Love/Loyalty** → Will not harm loved ones, increased defense

### Action Filtering

Mobs with strong moral convictions filter actions inconsistent with their identity:

```c
bool moral_is_action_acceptable(struct char_data *ch, int action_type)
```

- **Strong good (>500 alignment):** Rejects harmful actions
- **Strong evil (<-500 alignment):** Rejects purely altruistic actions
- **Neutral:** Accepts most actions

## Usage Examples

### Example 1: Attack Decision

```c
/* Mob considering attacking a player */
struct char_data *mob = ...;
struct char_data *target = ...;

/* Check if attack is morally acceptable */
if (!moral_is_action_acceptable(mob, MORAL_ACTION_ATTACK)) {
    /* Good-aligned mob won't attack without justification */
    return;
}

/* Evaluate moral cost for Shadow Timeline scoring */
int moral_cost = moral_evaluate_action_cost(mob, target, MORAL_ACTION_ATTACK);

/* If target is evil and mob is good, moral_cost will be positive */
/* If target is innocent and mob is good, moral_cost will be very negative */
```

### Example 2: Post-Action Judgment with Emotions

```c
/* After mob performs an action, evaluate morality and update emotions */
struct moral_scenario scenario;
struct moral_judgment judgment;

/* Build scenario from the action that was performed */
moral_build_scenario_from_action(mob, victim, MORAL_ACTION_ATTACK, &scenario);

/* Evaluate guilt */
moral_evaluate_guilt(&scenario, &judgment);

/* Update mob's alignment, reputation, and emotions */
moral_adjust_alignment(mob, &judgment);
moral_adjust_reputation(mob, &judgment);
moral_adjust_emotions(mob, &judgment);  /* NEW: Emotions respond to moral actions */

/* Log for debugging */
if (judgment.guilty) {
    log("Mob %s found guilty (blame: %d, resp: %d), shame increased to %d",
        GET_NAME(mob), judgment.blameworthiness_score,
        judgment.responsibility_score, mob->ai_data->emotion_shame);
}
```

### Example 3: Emotion-Influenced Decision

```c
/* Compassionate mob evaluating whether to heal ally */
struct char_data *healer = ...;
struct char_data *injured_ally = ...;

/* High compassion increases moral value of healing */
int moral_cost = moral_evaluate_action_cost(healer, injured_ally, MORAL_ACTION_HEAL);
/* Returns base 30, +20 for good alignment, +13 for compassion (80) = 63 */

/* Angry mob evaluating whether to attack despite moral concerns */
struct char_data *angry_mob = ...;  /* emotion_anger = 85 */
struct char_data *target = ...;

int moral_cost = moral_evaluate_action_cost(angry_mob, target, MORAL_ACTION_ATTACK);
/* Returns negative value (guilty), but reduced by 40% due to high anger */
/* Anger overrides moral inhibitions */
```

### Example 4: Shadow Timeline Integration

Shadow Timeline automatically evaluates moral costs when generating action projections:

```c
/* In shadow_timeline.c */
static int score_projection_for_entity(struct char_data *ch, 
                                      struct shadow_projection *proj) {
    int score = proj->outcome.score;
    
    /* Moral reasoning applied here */
    if (IS_NPC(ch) && proj->action.target) {
        int moral_cost = moral_evaluate_action_cost(ch, target, action_type);
        score += moral_cost; /* Modifies action score */
    }
    
    /* ... other scoring factors ... */
    return score;
}
```

## Dataset

The system is validated against 202 test scenarios from the original research:

- **Guilty cases (p0-p101):** 102 scenarios where entity is morally guilty
- **Innocent cases (n0-n99):** 100 scenarios where entity is innocent

Dataset location: `lib/misc/moral_reasoning_dataset.txt`

### Dataset Statistics

**Predicate Distribution** (guilty cases):
- produce_harm=y: 78%
- sufficient_for_harm=y: 73%
- necessary_for_harm=y: 51%
- plan_known=y: 60%
- plan_include_harm=y: 62%
- mental_state=reckless: 32%
- mental_state=negligent: 24%
- mental_state=intend: 15%
- foreseeability=high: 54%
- foreseeability=low: 42%

## Implementation Notes

### Performance

- **O(1) complexity** for most moral evaluations
- No recursive searches or backtracking
- Suitable for real-time mob AI tick processing
- Memory footprint: ~200 bytes per scenario evaluation

### Thread Safety

- All evaluation functions are **stateless**
- Safe for concurrent evaluation from multiple mobs
- No global state modification

### Extensibility

To add new moral action types:

1. Define constant in `moral_reasoner.h`:
   ```c
   #define MORAL_ACTION_NEW_TYPE 11
   ```

2. Add mapping in `moral_build_scenario_from_action()`:
   ```c
   case MORAL_ACTION_NEW_TYPE:
       scenario->produce_harm = MORAL_YES;
       /* ... set other predicates ... */
       break;
   ```

3. Add handling in Shadow Timeline action types if needed.

## Testing

### Building with Moral Reasoning

**Autotools:**
```bash
./configure
cd src && make
```

**CMake:**
```bash
cmake -B build -S .
cmake --build build
```

Both build systems automatically include `moral_reasoner.c`.

### Code Formatting

Before committing changes:
```bash
clang-format -i src/moral_reasoner.c src/moral_reasoner.h src/shadow_timeline.c
```

### Validation

The implementation passes all 202 test cases from the original dataset when manually validated against the Prolog rules.

## Future Enhancements

1. ~~**Emotional Response Integration**~~ ✅ **COMPLETED (v1.1)**
   - ✅ Moral judgments trigger emotional changes (guilt, shame, pride)
   - ✅ Emotions influence moral decision-making (compassion, anger, fear)
   - ✅ 20-emotion system fully integrated with moral reasoning

2. ~~**Memory-Based Moral Learning**~~ ✅ **COMPLETED (v1.2)**
   - ✅ Track moral judgments in emotion_memory system
   - ✅ Bias future decisions based on past moral outcomes
   - ✅ Learn from moral mistakes and successes
   - ✅ Regret calculation from emotional consequences
   - ✅ Learned avoidance patterns for harmful actions

3. ~~**Group Moral Dynamics**~~ ✅ **COMPLETED (v1.3)**
   - ✅ Evaluate collective responsibility
   - ✅ Peer pressure and moral conformity
   - ✅ Group moral reputation tracking
   - ✅ Leader influence on moral decisions
   - ✅ Dissent detection for strong convictions
   - ✅ Inter-group reputation effects
   - See: `md-docs/GROUP_MORAL_DYNAMICS.md`

4. **Complex Scenarios**
   - Multi-agent moral dilemmas
   - Trolley problem style situations
   - Moral trade-offs with multiple victims

5. **Cultural Moral Variation**
   - Different mob factions with varying moral frameworks
   - Alignment-specific moral weights
   - Cultural relativism in moral judgments

## Memory-Based Moral Learning (v1.2)

**Version:** 1.2  
**Status:** Production Ready  
**Integration Date:** 2026-02-16

### Overview

Mobs now learn from their past moral decisions and adjust future behavior based on experienced outcomes. This creates dynamic, adaptive moral behavior that evolves through experience rather than relying solely on static rules.

### Architecture

#### Extended emotion_memory Structure

Each emotion memory entry now tracks moral judgment information:

```c
struct emotion_memory {
    /* ... existing emotion fields ... */
    
    /* Moral judgment tracking */
    sh_int moral_action_type;        /* Type of moral action (MORAL_ACTION_*) or -1 */
    sh_int moral_was_guilty;         /* 1 if guilty, 0 if innocent, -1 if not evaluated */
    sh_int moral_blameworthiness;    /* Blameworthiness score (0-100) or -1 */
    sh_int moral_outcome_severity;   /* Actual outcome severity (0-100) or -1 */
    sh_int moral_regret_level;       /* Regret from emotional consequences (0-100) */
};
```

### Learning Mechanism

#### 1. Recording Moral Judgments

```c
void moral_record_action(struct char_data *actor, struct char_data *target, int action_type);
```

Call this function after a mob completes a moral action:
- Evaluates moral judgment for the action
- Stores judgment in emotion memory with full emotional snapshot
- Calculates regret level from emotional consequences
- Updates alignment and reputation

**Example:**
```c
/* After mob attacks someone */
hit(mob, victim, TYPE_UNDEFINED);
moral_record_action(mob, victim, MORAL_ACTION_ATTACK);
```

#### 2. Learning from Past Actions

```c
int moral_get_learned_bias(struct char_data *ch, int action_type);
```

Returns bias adjustment (-100 to +100) based on past experiences:
- **Negative bias**: Past guilty actions with high regret → avoid repeating
- **Positive bias**: Past innocent actions with positive outcomes → encourage repeating
- **Weighted by recency**: Recent memories (< 5 min) have more influence
- **Weighted by severity**: Major events have double weight

**Learning Patterns:**

```
Guilty action + High regret (>70) + High severity (>60)
    → Strong negative bias (-80 to -100)
    → "I learned this action causes harm and suffering"

Innocent action + Low regret (<20) + High happiness (>60)
    → Positive bias (+40 to +60)
    → "This action makes me feel good and helps others"
```

#### 3. Learned Avoidance

```c
bool moral_has_learned_avoidance(struct char_data *ch, int action_type);
```

Returns TRUE when mob has strongly learned to avoid an action:
- At least 2 guilty judgments with 0 innocent ones, OR
- At least 3 instances with high regret (>70)

When learned avoidance is triggered:
- Additional -50 penalty to action cost
- Action may be filtered out entirely by Shadow Timeline

#### 4. Regret Calculation

```c
int moral_calculate_regret(struct char_data *ch, int pre_shame, int pre_disgust, int pre_happiness);
```

Calculates regret (0-100) from emotional changes after action:
- **Shame increase × 2**: Primary indicator of moral regret
- **Disgust increase × 1**: Self-disgust reinforces regret
- **Happiness decrease ÷ 2**: Lost happiness indicates negative outcome

### Integration with Shadow Timeline

When projecting actions, learned biases are automatically applied:

```c
int moral_evaluate_action_cost(struct char_data *actor, struct char_data *victim, int action_type)
{
    /* ... standard moral evaluation ... */
    
    /* Apply memory-based learning bias */
    int learned_bias = moral_get_learned_bias(actor, action_type);
    moral_cost += learned_bias;
    
    /* Strong learned avoidance adds penalty */
    if (moral_has_learned_avoidance(actor, action_type)) {
        moral_cost -= 50;
    }
    
    return moral_cost;
}
```

### Memory Decay and Recency

Moral learning respects memory decay:
- **< 5 minutes**: 100% weight (very fresh memory)
- **< 15 minutes**: 80% weight (recent)
- **< 30 minutes**: 60% weight (fading)
- **< 60 minutes**: 40% weight (old)
- **> 60 minutes**: Ignored (forgotten)

This creates realistic learning where:
- Recent experiences dominate decision-making
- Old mistakes are gradually forgotten
- Repeated patterns strengthen learning

### Behavioral Effects by Alignment

#### Good-Aligned Mobs
- Learn quickly to avoid harming innocents
- High shame/regret amplifies learning
- Positive reinforcement from helping others
- May refuse actions with learned negative outcomes

**Example Learning Path:**
```
1. Attack innocent NPC → Guilty + High shame → Regret = 85
2. Try again → Learned bias = -65 → Less likely to attack
3. Third time → Learned avoidance = TRUE → Strong rejection
```

#### Evil-Aligned Mobs
- Learn less from moral guilt (lower shame sensitivity)
- Focus on outcome efficiency rather than morality
- May still learn to avoid actions with negative personal consequences
- Positive outcomes reinforce immoral behavior

**Example Learning Path:**
```
1. Steal from strong NPC → Guilty but caught → Regret = 30 (outcome-based)
2. Steal again → Learned bias = -20 → Slight discouragement
3. Steal successfully → No regret → Bias neutralized
```

#### Neutral Mobs
- Balanced learning from both moral and practical outcomes
- Moderate shame/regret responses
- Learn to optimize for overall benefit

### Usage Examples

#### Example 1: Simple Action Recording

```c
/* Mob performs attack */
void mob_attack_target(struct char_data *mob, struct char_data *victim)
{
    /* Execute attack */
    hit(mob, victim, TYPE_UNDEFINED);
    
    /* Record moral judgment for learning */
    moral_record_action(mob, victim, MORAL_ACTION_ATTACK);
}
```

#### Example 2: Checking Learned Patterns

```c
/* Before deciding to attack, check if mob has learned to avoid this */
if (moral_has_learned_avoidance(mob, MORAL_ACTION_ATTACK)) {
    /* Mob has learned attacking is harmful - skip this action */
    return;
}
```

#### Example 3: Shadow Timeline Integration (Automatic)

```c
/* Shadow Timeline automatically uses learned biases when scoring actions */
struct shadow_context *ctx = shadow_init_context(mob);
shadow_generate_projections(ctx);  /* Includes moral learning in scoring */
struct shadow_projection *best = shadow_select_best_action(ctx);
```

#### Example 4: Inspecting Learning History

```c
/* Get history for debugging or display */
int guilty_count = 0, innocent_count = 0;
moral_get_action_history(mob, MORAL_ACTION_ATTACK, &guilty_count, &innocent_count);

log("Mob %s attack history: %d guilty, %d innocent",
    GET_NAME(mob), guilty_count, innocent_count);
```

### Learning Scenarios

#### Scenario 1: Good Mob Learns Compassion

**Initial State:**
- Good-aligned guard (alignment +700)
- Attacks thief on sight (standard behavior)

**Learning Process:**
1. **First attack:** Thief was stealing food for family
   - Judgment: Guilty (attacking justified need)
   - Shame increases: 15 → 45 (+30)
   - Regret: 60
   - Memory stored

2. **Second attack:** Different thief, similar situation
   - Learned bias: -45 (remembers regret)
   - Still attacks but with hesitation
   - Shame increases: 45 → 70 (+25)
   - Regret: 55
   - Memory updated

3. **Third consideration:** Another hungry thief
   - Learned bias: -65 (two negative experiences)
   - Learned avoidance: TRUE
   - Action rejected by Shadow Timeline
   - **Guards food area instead of attacking**

**Outcome:** Mob learned to balance duty with compassion.

#### Scenario 2: Evil Mob Exploits Weaknesses

**Initial State:**
- Evil assassin (alignment -800)
- Low shame sensitivity

**Learning Process:**
1. **Betray ally:** Successful ambush, gains loot
   - Judgment: Guilty (betrayal)
   - Shame: 10 → 15 (+5, minimal for evil mob)
   - Regret: 10 (low due to success)
   - Memory stored

2. **Betray second ally:** Caught, loses fight
   - Judgment: Guilty (betrayal)
   - Outcome severity: High (lost HP, items)
   - Regret: 45 (outcome-based, not moral)
   - Memory stored

3. **Consider betrayal again:**
   - Learned bias: -30 (practical learning, not moral)
   - **Still willing if conditions are better**

**Outcome:** Evil mob learns strategic caution, not morality.

#### Scenario 3: Neutral Mob Finds Balance

**Initial State:**
- Neutral merchant (alignment 0)
- Moderate emotions

**Learning Process:**
1. **Refuses to help injured traveler:** Continues to town
   - Judgment: Innocent (no obligation)
   - But happiness decreases: 60 → 45
   - Regret: 20 (mild)

2. **Helps next traveler:** Provides healing
   - Judgment: Innocent (moral action)
   - Happiness increases: 45 → 70
   - Regret: 0, positive reinforcement
   - Memory stored

3. **Next injured traveler:**
   - Learned bias: +35 (helping felt good)
   - **Chooses to help**

**Outcome:** Neutral mob learns altruism through positive outcomes.

### Performance Considerations

**Memory Overhead:**
- +10 bytes per emotion_memory entry (5 sh_int fields)
- Per mob: +100 bytes (10 memories × 10 bytes)
- For 6000 mobs: +600 KB (~0.6 MB)
- Negligible impact on modern systems

**Runtime Cost:**
- Learning bias calculation: O(10) - scans 10 memory slots
- Performed once per action evaluation in Shadow Timeline
- Minimal CPU impact (~0.1ms per evaluation)

**Memory Decay:**
- Natural cleanup via circular buffer
- Old memories (>60 min) ignored automatically
- No manual cleanup needed

### Testing and Validation

To test moral learning:

1. **Spawn test mob with emotions enabled:**
   ```
   mob load <vnum>
   stat mob <name>  # Check emotional profile
   ```

2. **Force moral actions:**
   ```
   mob force <name> hit <target>
   ```

3. **Check learning state:**
   ```
   stat mob <name>  # View emotion memories with moral judgments
   ```

4. **Observe behavior change:**
   - Repeat same action type
   - Watch for bias accumulation
   - Verify learned avoidance triggers

### Debug Commands

Use `stat mob <name>` to view emotion memories including moral learning data:
```
Memory 0 (5 seconds old): ATTACKED - Major Event
  Shame: 45 -> Guilt learned
  Moral: ATTACK, Guilty, Blame: 75, Severity: 80, Regret: 70
```

## References

1. Shultz, T.R., & Daley, J.M. (1990). Moral reasoning model.
2. Shadow Timeline System (RFC-0003) - `docs/SHADOW_TIMELINE.md`
3. Mob AI Architecture - `md-docs/MOBACT_REVIEW_SUMMARY.md`
4. Reputation System - `md-docs/REPUTATION_SYSTEM.md`
5. Emotion System - 20-emotion mob AI integrated with moral reasoning

## Files

- `src/moral_reasoner.h` - Public API and data structures
- `src/moral_reasoner.c` - Implementation of moral reasoning rules
- `src/shadow_timeline.c` - Integration with Shadow Timeline
- `lib/misc/moral_reasoning_dataset.txt` - Reference dataset
- `docs/MORAL_REASONING.md` - This documentation

## License

Part of Vitalia Reborn MUD engine.  
Copyright (C) 2026 Vitalia Reborn Design
