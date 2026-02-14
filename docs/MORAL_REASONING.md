# Moral Reasoning System

**Version:** 1.0  
**Status:** Production Ready  
**Integration Date:** 2026-02-14  

## Overview

The Moral Reasoning System implements Shultz & Daley's rule-based model for qualitative moral judgment. It enables mobs in Vitalia Reborn to evaluate the moral implications of their actions, leading to more realistic and nuanced decision-making behavior.

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

### Example 2: Post-Action Judgment

```c
/* After mob performs an action, evaluate morality */
struct moral_scenario scenario;
struct moral_judgment judgment;

/* Build scenario from the action that was performed */
moral_build_scenario_from_action(mob, victim, MORAL_ACTION_ATTACK, &scenario);

/* Evaluate guilt */
moral_evaluate_guilt(&scenario, &judgment);

/* Update mob's alignment and reputation */
moral_adjust_alignment(mob, &judgment);
moral_adjust_reputation(mob, &judgment);

/* Log for debugging */
if (judgment.guilty) {
    log("Mob %s found guilty (blame: %d, resp: %d)",
        GET_NAME(mob), judgment.blameworthiness_score,
        judgment.responsibility_score);
}
```

### Example 3: Shadow Timeline Integration

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

1. **Emotional Response Integration**
   - Link moral judgments to emotional changes (guilt, shame, pride)
   - Mobs with high compassion could feel worse about guilty actions

2. **Memory-Based Moral Learning**
   - Track moral judgments in emotion_memory system
   - Bias future decisions based on past moral outcomes

3. **Group Moral Dynamics**
   - Evaluate collective responsibility
   - Peer pressure and moral conformity

4. **Complex Scenarios**
   - Multi-agent moral dilemmas
   - Trolley problem style situations

5. **Cultural Moral Variation**
   - Different mob factions with varying moral frameworks
   - Alignment-specific moral weights

## References

1. Shultz, T.R., & Daley, J.M. (1990). Moral reasoning model.
2. Shadow Timeline System (RFC-0003) - `docs/SHADOW_TIMELINE.md`
3. Mob AI Architecture - `md-docs/MOBACT_REVIEW_SUMMARY.md`
4. Reputation System - `md-docs/REPUTATION_SYSTEM.md`

## Files

- `src/moral_reasoner.h` - Public API and data structures
- `src/moral_reasoner.c` - Implementation of moral reasoning rules
- `src/shadow_timeline.c` - Integration with Shadow Timeline
- `lib/misc/moral_reasoning_dataset.txt` - Reference dataset
- `docs/MORAL_REASONING.md` - This documentation

## License

Part of Vitalia Reborn MUD engine.  
Copyright (C) 2026 Vitalia Reborn Design
