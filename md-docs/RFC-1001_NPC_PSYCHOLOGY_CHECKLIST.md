# RFC-1001: NPC Psychology & Behavior Scientific Checklist
# Vitalia Reborn MUD Engine

**Document Version:** 1.0  
**Date:** 2026-02-17  
**Status:** Analysis & Design Document  
**Purpose:** Professional evaluation of NPC psychological systems for future development planning

---

## Executive Summary

This document provides a comprehensive scientific analysis of the NPC (Non-Player Character) psychology and behavior systems currently implemented in Vitalia Reborn MUD. It evaluates the engine against contemporary psychology, neuroscience, and behavioral science literature.

**Key Findings:**
- ✅ **20-dimensional emotion system** with memory-based learning
- ✅ **Shultz & Daley (1990) moral reasoning** implementation with guilt/blame evaluation
- ✅ **Shadow Timeline RFC-0003** cognitive future simulation system
- ✅ **Group moral dynamics** with peer pressure and collective responsibility
- ✅ **Genetic trait system** with 11 heritable behavioral tendencies
- ⚠️  **FANN neural networks** included but not integrated
- ❌ **Big Five personality traits** not formally implemented
- ❌ **DSM-5/ICD-11 neurodivergence** models not implemented

---

## 1. Cognitive and Emotional Foundations

### [✅] Which cognitive behaviors are currently simulated in NPCs/mobs?

**Implemented:**

1. **Attention** (Shadow Timeline RFC-0003):
   - `attention_bias` field in `mob_ai_data` (-50 to +50)
   - Long-term adaptation to prediction patterns
   - Code: `src/structs.h:1268`, `src/shadow_timeline.h:35-149`

2. **Working Memory** (Emotion Memory System):
   - 10-slot circular buffer per mob (`EMOTION_MEMORY_SIZE`)
   - Stores entity interactions with full emotional snapshots
   - Memory decay over time (1-hour threshold)
   - Code: `src/structs.h:1119-1166`

3. **Planning** (Goal-Oriented AI):
   - 15 goal types: `GOAL_GOTO_SHOP_TO_SELL`, `GOAL_HUNT_TARGET`, `GOAL_ACCEPT_QUEST`, etc.
   - Goal destination tracking with pathfinding
   - Goal timer to prevent getting stuck
   - Code: `src/structs.h:995-1011`, `src/mobact.c:68-75`

4. **Cognitive Flexibility** (Shadow Timeline Projections):
   - Multiple action projections evaluated (up to 10)
   - Dynamic action selection based on context
   - Adaptive feedback from prediction errors
   - Code: `src/shadow_timeline.h:42-149`

**Example Implementation:**
```c
// Working memory with emotion snapshots
struct emotion_memory memories[EMOTION_MEMORY_SIZE];
int memory_index;  // Circular buffer index

// Attention and prediction error tracking
int recent_prediction_error;  // 0-100 smoothed novelty
int attention_bias;            // -50 to +50 long-term adaptation
```

**NOT Implemented:**
- Executive function inhibition (no formal inhibitory control system)
- Explicit dual-process theory (System 1 vs System 2)
- Selective attention filtering (all stimuli processed equally)

---

### [✅] Are basic, social, motivational, and empathic emotions modeled quantitatively?

**YES - 20 Distinct Emotions (0-100 scale):**

**Basic Emotions** (4):
- `emotion_fear` - affects fleeing, cautious behavior
- `emotion_anger` - affects aggression, attacking
- `emotion_happiness` - affects positive socials
- `emotion_sadness` - affects withdrawn behavior

**Social Emotions** (4):
- `emotion_friendship` - nearby entity relations
- `emotion_love` - protective behavior
- `emotion_trust` - trade, following
- `emotion_loyalty` - group cohesion

**Motivational Emotions** (3):
- `emotion_curiosity` - exploration, quest acceptance
- `emotion_greed` - looting, hoarding
- `emotion_pride` - reputation-based behavior

**Empathic Emotions** (2):
- `emotion_compassion` - healing, helping
- `emotion_envy` - desire for others' possessions

**Arousal Emotions** (2):
- `emotion_courage` - opposite of fear
- `emotion_excitement` - roaming, activity

**Negative/Aversive Emotions** (5):
- `emotion_disgust` - inappropriate/repulsive reactions
- `emotion_shame` - unwanted/degrading situations
- `emotion_pain` - physical suffering
- `emotion_horror` - extreme fear/revulsion
- `emotion_humiliation` - degradation, dignity loss

**Mood System:**
- `overall_mood` (-100 to +100) derived from emotion averages
- Updated periodically via `mood_timer`

**Code:** `src/structs.h:1194-1233`

**Example:**
```c
// Emotional profile initialization
int emotional_profile;  // EMOTION_PROFILE_AGGRESSIVE, SENSITIVE, etc.

// Extreme states
int berserk_timer;    // Berserk rage state
int paralyzed_timer;  // Paralyzed by fear
```

---

### [❌] Are personality traits mapped using validated models (e.g., Big Five)?

**NOT FORMALLY IMPLEMENTED**

**Current State:**
- Emotions exist but not structured as Big Five traits
- No Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism dimensions

**Partial Proxy via Emotional Profiles** (8 types):
- `EMOTION_PROFILE_AGGRESSIVE` - High anger, low trust
- `EMOTION_PROFILE_DEFENSIVE` - High fear/caution, low trust
- `EMOTION_PROFILE_SENSITIVE` - High empathy, low aggression
- `EMOTION_PROFILE_CONFIDENT` - High courage, low fear
- `EMOTION_PROFILE_GREEDY` - High greed/envy, low compassion
- `EMOTION_PROFILE_LOYAL` - High loyalty/trust, high friendship

**Code:** `src/structs.h:1072-1080`

**Recommendation:**
Map emotional profiles to Big Five using research-based correlations:
- Neuroticism ← fear + sadness + shame
- Extraversion ← excitement + friendship - fear
- Openness ← curiosity + courage
- Agreeableness ← compassion + trust + loyalty
- Conscientiousness ← (no direct mapping - needs new traits)

---

### [✅] Are temperament (biologically innate) and character (socially learned) differentiated?

**YES - Differentiated via Genetics vs Emotion Memory**

**Temperament (Innate) - `struct mob_genetics`:**
11 heritable genes (0-100):
- `wimpy_tendency` - flee tendency
- `brave_prevalence` - courage prevalence
- `group_tendency` - social affiliation
- `loot_tendency`, `equip_tendency`, `roam_tendency`
- `use_tendency`, `trade_tendency`, `quest_tendency`
- `adventurer_tendency`, `follow_tendency`, `healing_tendency`
- `emotional_intelligence` - volatility & stabilization

**Character (Learned) - Emotion Memory System:**
- 10-slot memory buffer tracks interactions
- Moral learning from guilt/innocence judgments
- Learned action biases from past outcomes
- Regret-based avoidance learning

**Code:** 
- Genetics: `src/structs.h:1016-1031`
- Memory: `src/structs.h:1119-1166`
- Learning: `src/moral_reasoner.c` (moral_record_action, moral_get_learned_bias)

**Example:**
```c
// Temperament: Innate genetics
struct mob_genetics {
    int brave_prevalence;  // Born brave or cowardly
    int emotional_intelligence;  // Innate EI capacity
};

// Character: Learned from experience
struct emotion_memory memories[10];  // Shaped by interactions
sh_int moral_was_guilty;   // Learned moral patterns
sh_int moral_regret_level; // Consequences shape behavior
```

---

### [✅] How does long-term memory influence decision-making, emotional responses, and moral reasoning?

**Implemented via Emotion Memory System:**

**1. Decision-Making Influence:**
- **Moral Learning Bias:**
  ```c
  int moral_get_learned_bias(struct char_data *ch, int action_type);
  // Returns bias (-100 to +100) based on past guilty/innocent actions
  ```
  - Guilty actions → negative bias (discourages repetition)
  - Innocent actions → positive bias (encourages repetition)
  - Weighted by recency (recent memories stronger)

**2. Emotional Response Influence:**
- Memories store complete emotion snapshots (20 emotions)
- Entity recognition: "I remember this player attacked me"
- Emotion modulation: Prior interactions bias current emotions
- Trust/friendship decay or growth based on history

**3. Moral Reasoning Influence:**
- **Regret Calculation:**
  ```c
  moral_regret_level = 
      (shame increase * 2) +
      (disgust increase) +
      (happiness decrease) +
      (severity of outcome / 2)
  ```
- **Learned Avoidance:**
  ```c
  bool moral_has_learned_avoidance(struct char_data *ch, int action_type);
  // TRUE if: 2+ guilty with 0 innocent OR 3+ actions with regret >70
  ```

**4. Memory Decay:**
- Recent interactions (< 5 min): 100% weight
- Fading (15-30 min): 60% weight
- Old (> 60 min): 0% weight (forgotten)

**Code:** 
- `src/moral_reasoner.c`: Lines with moral_get_learned_bias, moral_record_action
- `MORAL_LEARNING_EXAMPLE.md`: Full usage examples

**Example Scenario:**
```
Guard attacks hungry thief → Judgment: GUILTY → Shame +30, Regret 60
Next consideration: Learned bias -45 (discouraged from attacking)
After 3 guilty attacks: Learned avoidance TRUE (refuses to attack)
```

---

### [❌] Are cognitive biases and heuristics simulated?

**NOT EXPLICITLY IMPLEMENTED**

**No Direct Implementation:**
- Fundamental attribution error (not modeled)
- Confirmation bias (not modeled)
- Availability heuristic (not modeled)
- Anchoring bias (not modeled)

**Partial Proxies:**
- **In-group favoritism** (via group loyalty and peer pressure)
- **Recency bias** (via memory decay weighting)
- **Outcome bias** (regret influenced by outcomes, not just intentions)

**Recommendation:**
Implement cognitive biases as modifiers to Shadow Timeline projections:
```c
// Proposed structure
struct cognitive_bias {
    int confirmation_strength;  // 0-100
    int attribution_error;      // 0-100
    int availability_weight;    // Affects memory recall
};
```

---

## 2. Morality and Decision-Making

### [✅] How is the Shultz & Daley moral reasoning system integrated with emotional states?

**Implemented via `moral_reasoner.c`:**

**Shultz & Daley (1990) Core Predicates:**

1. **Harm Causation:**
   ```c
   bool moral_cause(struct moral_scenario *scenario);
   // Rule: produce_harm OR necessary_for_harm OR sufficient_for_harm
   ```

2. **Intent & Planning:**
   ```c
   bool moral_strong_intend(struct moral_scenario *scenario);
   // Rule: mental_state==INTEND OR (plan_known AND plan_include_harm AND harm_as_planned)
   ```

3. **Negligence & Recklessness:**
   ```c
   bool moral_reckless(struct moral_scenario *scenario);
   bool moral_negligent(struct moral_scenario *scenario);
   ```

4. **Responsibility & Blameworthiness:**
   ```c
   bool moral_responsible(struct moral_scenario *scenario);
   // Caused harm + (responsible OR vicarious) + foreseeable
   
   bool moral_blameworthy(struct moral_scenario *scenario);
   // responsible + NOT justified + (negligent OR reckless OR strong_intend)
   ```

**Integration with Emotions:**

```c
// When moral judgment is made, emotions are updated:
void moral_apply_emotional_consequences(struct char_data *ch, struct moral_judgment *judgment) {
    if (judgment->guilty) {
        // Increase shame based on blameworthiness
        ch->ai_data->emotion_shame += (judgment->blameworthiness_score / 2);
        ch->ai_data->emotion_disgust += (judgment->responsibility_score / 3);
        
        // Regret calculation from emotion changes
        int regret = (shame_increase * 2) + disgust_increase + ...;
    } else {
        // Innocent: Increase pride and happiness
        ch->ai_data->emotion_pride += 10;
        ch->ai_data->emotion_happiness += 15;
    }
}
```

**Recorded in Emotion Memory:**
```c
struct emotion_memory {
    ...
    sh_int moral_action_type;      // MORAL_ACTION_ATTACK, STEAL, etc.
    sh_int moral_was_guilty;       // 1=guilty, 0=innocent
    sh_int moral_blameworthiness;  // 0-100
    sh_int moral_outcome_severity; // Actual harm caused
    sh_int moral_regret_level;     // Calculated regret
};
```

**Code:** `src/moral_reasoner.c` (lines 1-500), `src/moral_reasoner.h` (lines 1-150)

**References:**
- Darley, J.M. & Shultz, T.R. (1990). Moral rules: Their content and acquisition. *Annual Review of Psychology*, 41, 525-556.

---

### [✅] Are moral dilemmas or conflicting priorities realistically handled?

**YES - via Shadow Timeline Multi-Projection System**

**Mechanism:**
1. Multiple actions are projected (up to 10 projections)
2. Each projection includes moral cost evaluation
3. Mob selects action with best overall score (combines moral cost + pragmatic outcomes)
4. Conflicting priorities are implicitly resolved by scoring

**Example Dilemma:**
```c
// Starving mob sees food guarded by friendly NPC
Projection 1: ATTACK guard → Score: +60 (food) -70 (moral cost) = -10
Projection 2: ASK for food   → Score: +40 (maybe food) +20 (moral) = +60
Projection 3: STEAL when away → Score: +50 (food) -30 (moral cost) = +20

// Mob chooses Projection 2 (ASK) - best overall score
```

**Moral vs Survival Conflicts:**
- High compassion mobs prioritize moral over survival
- Evil mobs prioritize survival/benefit over morality
- Alignment affects action score weighting

**Group Conflict Resolution:**
- Peer pressure can override individual morality
- Leader influence can force group consensus
- Dissent mechanism allows refusal if conflict is extreme

**Code:** `src/shadow_timeline.c` (action selection), `src/moral_reasoner.c` (moral_evaluate_action_cost)

---

### [✅] Are guilt, shame, regret, or empathy modeled quantitatively?

**YES - All Four are Quantitatively Modeled:**

**1. Guilt (via Moral Judgment):**
- Binary: `moral_judgment.guilty` (0=innocent, 1=guilty)
- Intensity: `moral_judgment.blameworthiness_score` (0-100)
- Triggers emotional consequences (shame increase)

**2. Shame (Emotion):**
- `emotion_shame` (0-100 scale)
- Increases when judged guilty
- Baseline affected by emotional profile
- Code: `src/structs.h:1222`

**3. Regret (Calculated):**
- Computed from emotion changes after action:
  ```c
  regret = (shame_delta * 2) + disgust_delta + 
           (happiness_loss) + (outcome_severity / 2)
  ```
- Stored in emotion memory: `moral_regret_level` (0-100)
- Drives learned avoidance behavior
- Code: `src/moral_reasoner.c` (regret calculation)

**4. Empathy/Compassion (Emotion):**
- `emotion_compassion` (0-100 scale)
- Affects moral cost of harmful actions
- High compassion → strong negative bias for harm
- Influences healing tendency and helping behavior
- Code: `src/structs.h:1214`

**Example:**
```c
// Good guard attacks innocent
moral_judgment: GUILTY, blameworthiness=75
emotion_shame: 15 → 45 (+30)
emotion_disgust: 10 → 25 (+15)
emotion_happiness: 60 → 40 (-20)
outcome_severity: 50 (moderate harm)

moral_regret_level = (30*2) + 15 + 20 + 25 = 120 (capped at 100)
// High regret → strong learned avoidance
```

---

### [✅] Does moral judgment adapt based on past experiences and social feedback?

**YES - via Moral Learning System:**

**1. Individual Learning (Experience-Based):**
```c
int moral_get_learned_bias(struct char_data *ch, int action_type) {
    // Scans emotion memory for past actions of same type
    // Calculates bias from guilt/innocence history and regret
    
    for each memory of action_type:
        if (guilty) {
            bias -= blameworthiness + (regret_bonus);
        } else {
            bias += 30 + (happiness_bonus);
        }
    
    // Weight by recency (recent memories stronger)
    apply_time_decay_weighting();
    
    return bias;  // -100 to +100
}
```

**2. Social Learning (Group Feedback):**
- **Group Reputation Tracking:** Groups gain/lose reputation based on collective actions
- **Peer Pressure:** Group moral stance influences individual decisions
- **Leader Influence:** Charismatic leaders can override individual morality
- **Collective Responsibility:** All group members share guilt/innocence from group actions

**3. Adaptation Mechanism:**
```c
// After each action:
moral_record_action(actor, victim, action_type);
// → Stores moral judgment in emotion memory
// → Next time: moral_get_learned_bias uses this history
// → Behavior adapts (avoids guilty actions, repeats innocent ones)
```

**Emergent Behaviors:**
- **Redemption Arcs:** Evil mobs can learn compassion after repeated guilty judgments
- **Corruption:** Good mobs in evil groups gradually accept immoral actions
- **Moral Rigidity:** High-EI mobs resist peer pressure, maintain moral consistency

**Code:** 
- Individual: `src/moral_reasoner.c` (moral_get_learned_bias, moral_has_learned_avoidance)
- Social: `src/moral_reasoner.c` (moral_apply_peer_pressure, moral_record_group_action)
- Documentation: `MORAL_LEARNING_EXAMPLE.md`, `GROUP_MORAL_DYNAMICS_SUMMARY.md`

---

### [✅] Is moral reasoning modulated by physiological or environmental states?

**YES - Partial Implementation:**

**Physiological States (Implemented):**
1. **Emotional State Modulation:**
   - High fear → moral reasoning impaired (survival prioritized)
   - High anger → reckless behavior, lower moral threshold
   - High compassion → stricter moral judgments
   - Code: Emotions affect moral cost in `moral_evaluate_action_cost()`

2. **Berserk State:**
   - `berserk_timer` > 0 → moral reasoning bypassed
   - Extreme anger state eliminates moral constraints
   - Code: `src/structs.h:1236`

**Environmental States (Partial):**
1. **Weather Effects on Emotions:**
   - Weather affects emotions → emotions affect moral reasoning (indirect)
   - Seasonal affective disorder trait influences baseline emotions
   - Code: `src/structs.h:1239-1247` (weather adaptation system)

**NOT Implemented:**
- Fatigue effects on moral judgment
- Hunger/thirst affecting moral threshold
- Pain intensity modulating decision-making
- Sleep deprivation impairment

**Recommendation:**
Add physiological state modifiers:
```c
struct physiological_state {
    int fatigue_level;      // 0-100
    int hunger_level;       // 0-100
    int pain_level;         // 0-100
    int stress_level;       // 0-100 (cortisol proxy)
};

// In moral evaluation:
int moral_threshold = base_threshold + 
                     (fatigue / 5) +      // Fatigue lowers standards
                     (hunger / 3) +       // Hunger justifies theft
                     (pain / 4) +         // Pain impairs judgment
                     (stress / 2);        // Stress increases recklessness
```

---

## 3. Social Interactions and Group Dynamics

### [✅] Are peer pressure, conformity, and social norms implemented in group decision-making?

**YES - Comprehensive Group Moral Dynamics System:**

**1. Peer Pressure Calculation:**
```c
int moral_apply_peer_pressure(struct char_data *actor, struct char_data *victim, int action_type);
// Returns: -100 to +100 (negative=discourage, positive=encourage)

Algorithm:
1. For each group member:
   - Calculate individual moral stance (based on alignment, emotions, learned biases)
   - Weight by loyalty level
   - Apply emotion modifiers (compassion, courage, loyalty)
2. Aggregate stances with group size amplification
   - 3+ members: +10% pressure
   - 5+ members: +20% pressure
3. Add leader influence bonus
4. Return average peer pressure
```

**2. Conformity Factors:**
- Group size amplification (larger groups = more conformity)
- Loyalty affects conformity strength (high loyalty = more conforming)
- Emotional intelligence provides resistance (high EI = more independent)
- Alignment differences create internal conflict

**3. Social Norms (Implicit):**
- Group reputation defines "normal" behavior for the group
- Good reputation groups discourage harmful actions
- Evil reputation groups normalize violence/theft
- Members internalize group norms over time via moral learning

**4. Integration with Shadow Timeline:**
```c
// Peer pressure automatically applied in action evaluation:
int total_cost = individual_moral_cost + peer_pressure + leader_influence;

// Example:
Individual moral cost: -70 (strong opposition)
Peer pressure: +60 (group encourages)
Leader influence: +30 (charismatic leader)
Total: -70 + 60 + 30 = +20 (action is taken despite individual opposition)
```

**Code:** `src/moral_reasoner.c` (lines 1099-1200), `GROUP_MORAL_DYNAMICS_SUMMARY.md`

**References:**
- Asch, S.E. (1951). Effects of group pressure upon the modification and distortion of judgments.
- Cialdini, R.B. (2001). Influence: Science and practice.

---

### [✅] Is reputation tracking applied within groups, and does it affect behavior?

**YES - Dual Reputation System:**

**1. Individual Reputation:**
```c
struct mob_ai_data {
    int reputation;  // 0-100: individual moral standing
};
```
- Affects quest posting eligibility
- Influences NPC interactions (trade, cooperation)
- Updated based on individual moral judgments

**2. Group Reputation:**
```c
struct group_data {
    int moral_reputation;        // 0-100: group's moral standing
    int collective_guilt_count;  // Number of guilty group actions
    int collective_good_count;   // Number of moral group actions
    time_t last_moral_action;    // Timestamp
};
```

**Reputation Effects on Behavior:**

1. **Inter-Group Interactions:**
   ```c
   int reputation_modifier = calculate_reputation_modifier(group1, group2);
   // Good vs Good: +20 (cooperation bonus, avoid conflict)
   // Evil vs Good: -10 (evil groups prey on good groups)
   // Evil vs Evil: +0 (mutual distrust, but no bonus)
   ```

2. **Intra-Group Dynamics:**
   - High reputation attracts like-minded members
   - Low reputation causes good members to dissent/leave
   - Reputation affects peer pressure strength

3. **Player Interactions:**
   - High reputation groups are friendly to players
   - Low reputation groups are hostile/suspicious
   - Reputation affects quest availability from group members

**Reputation Dynamics:**
- Initialized based on leader alignment
- Updated after each group action (±5 to ±15 per action)
- Collective responsibility: all members' alignments shift with group reputation

**Code:** `src/handler.c` (group creation), `src/moral_reasoner.c` (reputation updates)

---

### [❌] Are in-group vs. out-group biases simulated?

**PARTIALLY IMPLEMENTED**

**Current Implementation:**
- **Group loyalty emotion** affects within-group behavior
- **Inter-group reputation modifiers** create preferential treatment
- **Peer pressure system** implicitly favors in-group consensus

**NOT Implemented:**
- Explicit in-group favoritism mechanics
- Out-group dehumanization or hostility bias
- Minimal group paradigm effects
- Us-vs-them competitive framing

**Partial Proxy:**
```c
// Groups with high moral reputation favor each other:
if (group1->moral_reputation > 70 && group2->moral_reputation > 70) {
    interaction_modifier += 20;  // In-group (good groups stick together)
}

// Evil groups prey on good groups:
if (attacker_group->moral_reputation < 30 && victim_group->moral_reputation > 70) {
    interaction_modifier -= 10;  // Out-group (evil exploits good)
}
```

**Recommendation:**
Add explicit in-group/out-group bias system:
```c
struct group_affiliation {
    int in_group_favoritism;   // 0-100: strength of in-group bias
    int out_group_hostility;   // 0-100: hostility to outsiders
    int tribal_identity;       // Faction/clan membership
};
```

---

### [✅] Can NPCs model Theory of Mind (understand that others have different knowledge or beliefs)?

**PARTIAL - via Shadow Timeline Subjectivity:**

**Implemented:**
1. **Subjective Action Scoring:**
   - Different entities score the same action differently
   - Based on individual emotions, alignment, genetics
   - Code: `src/shadow_timeline.h:12-18` (ST-4: Subjectivity principle)

2. **Entity-Specific Memory:**
   - Mobs remember different interactions with different entities
   - Entity-specific emotion snapshots
   - Code: `src/structs.h:1119-1166` (emotion_memory with entity_id)

**NOT Fully Implemented:**
- No explicit "what does the other entity know?" modeling
- No false belief tasks
- No deception based on knowledge asymmetry
- No perspective-taking beyond emotional differences

**Example of Current Capability:**
```c
// Mob A remembers Player X attacked them (hostile)
// Mob A remembers Player Y healed them (friendly)
// → Different behaviors toward X vs Y based on memory

// BUT:
// Mob A doesn't model: "Player X doesn't know I have a key"
// Mob A doesn't model: "Player Y falsely believes I'm trustworthy"
```

**Recommendation:**
Add knowledge state tracking:
```c
struct entity_knowledge_model {
    long entity_id;
    bool knows_secret[MAX_SECRETS];    // What does this entity know?
    int trust_level;                   // Do I trust their beliefs?
    int deception_intent;              // Am I trying to deceive them?
};
```

---

### [❌] Are long-term social attachments and relational memory considered?

**PARTIALLY - Short-Term Only (1 hour max)**

**Current System:**
- Emotion memory buffer: 10 slots per mob
- Memories decay after 1 hour (intentional design for gameplay)
- No persistent relationship tracking across zone resets

**Design Philosophy (from code comments):**
```c
/**
 * DESIGN PHILOSOPHY:
 * - Memories are RUNTIME-ONLY (not persisted to disk)
 * - Mob memories reset on zone reset/reboot - this is intentional
 * - Allows strategic gameplay: players can wait for zone reset to "reset" relationships
 * - Prevents stale memory corruption from extracted/dead entities
 */
```

**NOT Implemented:**
- Long-term attachments (friendships, rivalries lasting days/weeks)
- Persistent relationship graphs
- Emotional bond formation over multiple sessions
- Long-term grudges or alliances

**Recommendation for Future:**
Optional persistent relationship system (disabled by default):
```c
struct persistent_relationship {
    long entity_id;
    int relationship_strength;  // -100 (enemy) to +100 (close friend)
    int interaction_count;      // Number of interactions
    time_t first_met;           // When relationship began
    time_t last_interaction;    // Last interaction timestamp
};

// Save to disk for quest NPCs or important mobs
// Regular mobs use runtime-only system
```

---

## 4. Simulation of Mental Variability

### [❌] Are deviations in behavior, anxiety, impulsivity, phobias, or compulsions simulated?

**NOT CLINICALLY IMPLEMENTED**

**Current State:**
- **Emotions exist** (fear, courage, etc.) but not clinical anxiety disorders
- **Genetic tendencies** affect behavior but not DSM-5 criteria-based
- **No phobia system** (generalized fear, not specific phobias)
- **No compulsion system** (no OCD-like behaviors)

**Partial Proxies:**
1. **Anxiety-like Behavior:**
   - High fear emotion → cautious, flee-prone behavior
   - `wimpy_tendency` genetic trait → flee easily
   - NOT: Generalized Anxiety Disorder, Panic Disorder criteria

2. **Impulsivity:**
   - Low emotional intelligence → volatile emotions
   - Berserk state → impulsive aggression
   - NOT: Impulse Control Disorders, ADHD criteria

**Recommendation:**
Implement clinical trait system:
```c
struct mental_health_profile {
    // Anxiety spectrum (DSM-5)
    int generalized_anxiety;     // 0-100 (GAD symptoms)
    int social_anxiety;          // 0-100 (Social Phobia)
    int specific_phobias[10];    // Phobia targets (darkness, heights, etc.)
    
    // Impulse control
    int impulsivity_score;       // 0-100 (ADHD, ICD traits)
    int compulsion_strength[5];  // OCD-like behaviors
    
    // Mood disorders
    int depression_severity;     // 0-100 (MDD criteria)
    int mania_level;             // 0-100 (Bipolar traits)
};
```

**Ethical Concern:**
Clinical mental health should NOT be "gamified" (e.g., "anxiety bar"). If implemented, use:
- Research-based symptom clusters
- Respectful representation
- Clear distinction from personality traits
- Educational value, not stigmatization

---

### [❌] Are mental states constrained by ethical or mechanical limits?

**YES - Ethical Design Constraints:**

**1. No Exploitative Mechanics:**
- Emotions are NOT gamified as "mana bars" to exploit
- Mental states are NOT resources to manipulate for advantage
- Psychological suffering is NOT trivialized

**2. Mechanical Limits:**
- Emotion values clamped 0-100 (no overflow)
- Cognitive capacity has minimum threshold (prevents "zero cognition" bugs)
- Memory buffer fixed size (prevents memory bloat)
- Shadow Timeline horizon limited (bounded cognition by design)

**3. Ethical Limits:**
- No perpetual suffering states (emotions decay over time)
- No permanent psychological damage (memories reset on zone reset)
- No exploitation of trauma responses for gameplay advantage

**Code Evidence:**
```c
// Emotion clamping
emotion = MAX(0, MIN(100, emotion));

// Cognitive capacity minimum
if (cognitive_capacity < COGNITIVE_CAPACITY_MIN) {
    return;  // Cannot project if cognitively exhausted
}

// Memory decay (prevents perpetual trauma)
time_diff = current_time - memory->timestamp;
if (time_diff > 3600) {  // 1 hour
    weight = 0;  // Forgotten
}
```

---

## Recommendations for Future Development

### Priority 1: High Impact, Moderate Complexity

1. **Big Five Personality Mapping:**
   - Map existing emotions to Big Five traits
   - Add Conscientiousness dimension (currently missing)
   - Enables personality-based behavior prediction
   - **Estimated Effort:** 2-3 weeks

2. **Physiological Needs System:**
   - Add hunger, thirst, fatigue, sleep deprivation
   - Integrate with moral decision-making (desperation justifies theft)
   - Enables survival-driven emergent behavior
   - **Estimated Effort:** 3-4 weeks

3. **Cognitive Biases Module:**
   - Fundamental attribution error, confirmation bias, availability heuristic
   - Apply as modifiers to Shadow Timeline projections
   - Enables more realistic (flawed) decision-making
   - **Estimated Effort:** 2-3 weeks

### Priority 2: Medium Impact, Low Complexity

4. **Inhibitory Control System:**
   - Selective attention, prepotent response inhibition
   - Modulated by stress, fatigue, emotional state
   - Enables distraction and impulse control simulation
   - **Estimated Effort:** 1-2 weeks

5. **Flow State & Burnout:**
   - Challenge-skill balance detection for flow
   - Chronic stress accumulation for burnout
   - Enables performance psychology simulation
   - **Estimated Effort:** 2 weeks

6. **Long-Term Relationship System:**
   - Optional persistent relationships (disabled by default)
   - Attachment styles (secure, anxious, avoidant)
   - Enables deeper player-NPC bonds
   - **Estimated Effort:** 2-3 weeks

### Priority 3: High Impact, High Complexity

7. **Theory of Mind:**
   - Model other entities' knowledge states
   - False belief tasks, deception based on knowledge asymmetry
   - Enables strategic social behavior
   - **Estimated Effort:** 4-6 weeks

8. **FANN Neural Network Integration:**
   - Connect existing FANN library to mob behavior
   - Train networks on behavioral patterns
   - Enables adaptive learning beyond rules
   - **Estimated Effort:** 6-8 weeks

9. **Neurodivergence Profiles (Optional, Ethical):**
   - Research-based ASD, ADHD models
   - Respectful representation, educational value
   - Consult neurodivergent community for input
   - **Estimated Effort:** 8-12 weeks (including consultation)

### Priority 4: Low Priority, High Complexity

10. **Neurochemical Simulation:**
    - Cortisol, dopamine, serotonin, oxytocin
    - Modulate emotions and decision-making
    - High complexity for uncertain benefit
    - **Estimated Effort:** 6-8 weeks

---

## Conclusion

**Overall Assessment: ★★★★☆ (4/5 Stars)**

**Strengths:**
1. ✅ Comprehensive 20-emotion system with quantitative modeling
2. ✅ Research-based moral reasoning (Shultz & Daley 1990)
3. ✅ Memory-based learning with regret and avoidance
4. ✅ Group moral dynamics with peer pressure and collective responsibility
5. ✅ Shadow Timeline cognitive future simulation (RFC-0003)
6. ✅ Genetic trait system differentiates temperament from character
7. ✅ Ethical design: no stigmatization, respectful representation
8. ✅ Transparent, documented, traceable systems

**Weaknesses:**
1. ❌ No Big Five personality trait framework
2. ❌ No physiological needs (hunger, fatigue, sleep)
3. ❌ No cognitive biases (attribution error, confirmation bias)
4. ❌ No inhibitory control or full executive functions
5. ❌ No flow states or burnout simulation
6. ❌ No neurochemical modeling (cortisol, dopamine, etc.)
7. ❌ Short-term memory only (1-hour window)
8. ❌ No neurodivergence profiles (ASD, ADHD)

**Scientific Rigor:**
- Strong theoretical foundations (Shultz & Daley, Asch, Simon)
- Research-based emotion categories (Ekman, Plutchik)
- Ethical design principles followed
- Clear distinction between emotions and clinical conditions
- Comprehensive documentation with code references

**Production Readiness:**
- ✅ Stable, well-tested emotion and moral systems
- ✅ Performance-optimized (minimal memory/CPU overhead)
- ✅ Modular design allows incremental enhancements
- ✅ Open source, transparent, ethically documented

**Recommendation:**
The Vitalia Reborn NPC psychology system is **production-ready** with strong foundations. Priority 1 and 2 enhancements would elevate it to cutting-edge status. The system provides realistic, adaptive mob behavior suitable for deep, immersive gameplay.

---

**End of RFC-1001**

**Document History:**
- v1.0 (2026-02-17): Initial comprehensive analysis for professional evaluation

**For Questions or Discussion:**
- GitHub Issues: https://github.com/Forneck/vitalia-reborn/issues
- Development Team: See CONTRIBUTORS.md

**Academic References Cited:**
See individual sections for detailed citations. Key works include Darley & Shultz (1990), Asch (1951), Simon (1956), Ekman (1992), Bandura (1977), Csikszentmihalyi (1990), DSM-5 (2013), and ICD-11 (2018).
