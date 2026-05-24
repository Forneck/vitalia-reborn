# Hybrid Emotion System - Developer Guide

## Overview

The Vitalia Reborn emotion system uses a **hybrid model** that combines two layers of emotional state to create realistic mob behavior and long-term relationship building.

## Architecture

### Layer 1: MOOD (Global State)

**What it is:** The mob's general emotional baseline

**Stored in:** `mob->ai_data->emotion_*` fields (20 emotions)
```c
mob->ai_data->emotion_fear      // General fear level
mob->ai_data->emotion_anger     // General anger level
mob->ai_data->emotion_trust     // General trust level
// ... etc for all 20 emotions
```

**Influenced by:**
- Environmental factors (dangerous areas, safe zones)
- Time-based effects (weather, time of day)
- General experiences (not tied to specific entities)
- Passive decay toward genetic baseline
- Emotional profile (personality traits)

**Use cases:**
- Ambient behavior (social initiation frequency)
- Environmental decisions (entering dangerous areas)
- General demeanor and mood swings
- Baseline for all interactions

### Layer 2: RELATIONSHIP (Per-Entity State)

**What it is:** Specific feelings toward individual players/mobs

**Stored in:** `mob->ai_data->memories[]` circular buffer (10 entries)
```c
struct emotion_memory {
    long entity_id;           // Player/mob identifier
    int entity_type;          // ENTITY_TYPE_PLAYER or ENTITY_TYPE_MOB
    int interaction_type;     // INTERACT_ATTACKED, INTERACT_HEALED, etc.
    time_t timestamp;         // When the interaction occurred
    int major_event;          // 1 for important events (2x weight)
    
    // Complete emotion snapshot at time of interaction
    sh_int fear_level;        // How afraid mob was
    sh_int anger_level;       // How angry mob was
    sh_int trust_level;       // How trusting mob was
    // ... all 20 emotions
};
```

**Influenced by:**
- Direct interactions with specific players/mobs
- Weighted by recency (recent = more influence)
- Weighted by importance (major events = 2x weight)
- Decays over time (old memories fade)

**Use cases:**
- Personalized behavior toward specific players
- Long-term relationship building
- Memory of past interactions
- Grudges, friendships, trust relationships

## Using the Hybrid System

### Core Function: get_effective_emotion_toward()

**Signature:**
```c
int get_effective_emotion_toward(struct char_data *mob, 
                                 struct char_data *target, 
                                 int emotion_type)
```

**What it does:**
- Retrieves the mob's effective emotion toward a specific target
- Combines MOOD (global) + RELATIONSHIP (per-target)
- Returns value 0-100

**Formula:**
```
effective_emotion = mood_emotion + (relationship_emotion - 50) * 0.6
```
- Mood provides the base
- Relationship acts as modifier (-30 to +30)
- Result is clamped to 0-100

**Parameters:**
- `mob` - The NPC whose emotion to query
- `target` - The player/mob to check feelings toward (can be NULL for mood-only)
- `emotion_type` - One of the EMOTION_TYPE_* constants

**Returns:**
- 0-100 emotion value
- Returns mood only if target is NULL
- Returns 0 if mob has no AI data or system is disabled

**Example Usage:**
```c
// Check how afraid this mob is of the attacker
int fear_of_attacker = get_effective_emotion_toward(mob, attacker, EMOTION_TYPE_FEAR);

// Check how much this shopkeeper trusts the buyer
int trust_of_buyer = get_effective_emotion_toward(keeper, buyer, EMOTION_TYPE_TRUST);

// Check general courage (no specific target)
int general_courage = get_effective_emotion_toward(mob, NULL, EMOTION_TYPE_COURAGE);
```

### Emotion Type Constants

Use these constants when calling `get_effective_emotion_toward()`:

```c
EMOTION_TYPE_FEAR          // Fear level
EMOTION_TYPE_ANGER         // Anger level
EMOTION_TYPE_HAPPINESS     // Happiness level
EMOTION_TYPE_SADNESS       // Sadness level
EMOTION_TYPE_FRIENDSHIP    // Friendship level
EMOTION_TYPE_LOVE          // Love/affection level
EMOTION_TYPE_TRUST         // Trust level
EMOTION_TYPE_LOYALTY       // Loyalty level
EMOTION_TYPE_CURIOSITY     // Curiosity level
EMOTION_TYPE_GREED         // Greed level
EMOTION_TYPE_PRIDE         // Pride level
EMOTION_TYPE_COMPASSION    // Compassion level
EMOTION_TYPE_ENVY          // Envy level
EMOTION_TYPE_COURAGE       // Courage level
EMOTION_TYPE_EXCITEMENT    // Excitement level
EMOTION_TYPE_DISGUST       // Disgust level
EMOTION_TYPE_SHAME         // Shame level
EMOTION_TYPE_PAIN          // Pain level
EMOTION_TYPE_HORROR        // Horror level
EMOTION_TYPE_HUMILIATION   // Humiliation level
```

## Implementation Examples

### Example 1: Shop Pricing

**Before (global emotions only):**
```c
// Shopkeeper gives discount based on general trust
if (keeper->ai_data->emotion_trust >= 60) {
    price *= 0.90;  // 10% discount
}
```

**After (hybrid system):**
```c
// Shopkeeper gives discount based on trust toward THIS buyer
int trust_toward_buyer = get_effective_emotion_toward(keeper, buyer, EMOTION_TYPE_TRUST);
if (trust_toward_buyer >= 60) {
    price *= 0.90;  // 10% discount for trusted customer
}
```

**Why it's better:** Shopkeeper can trust one player while being suspicious of another.

### Example 2: Combat Fleeing

**Before (global emotions only):**
```c
// Check general fear level
if (mob->ai_data->emotion_fear > 70) {
    attempt_flee(mob);
}
```

**After (hybrid system):**
```c
// Check fear specifically toward this attacker
int fear_of_attacker = get_effective_emotion_toward(mob, attacker, EMOTION_TYPE_FEAR);
if (fear_of_attacker > 70) {
    attempt_flee(mob);
}
```

**Why it's better:** Mob might flee from scary Player A but stand and fight against weaker Player B.

### Example 3: Emotion Display

**Before (global emotions only):**
```c
// Show mob's general anger
if (mob->ai_data->emotion_anger >= 70) {
    send_to_char(viewer, "The mob looks angry.\r\n");
}
```

**After (hybrid system):**
```c
// Show mob's anger specifically toward this viewer
int anger_toward_viewer = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_ANGER);
if (anger_toward_viewer >= 70) {
    send_to_char(viewer, "The mob glares at you with anger.\r\n");
}
```

**Why it's better:** Different players see different emotions based on their relationship.

### Example 4: Love-Based Following

**Before (global emotions only):**
```c
// Follow any player if love is high
if (mob->ai_data->emotion_love > 80) {
    add_follower(mob, some_player);
}
```

**After (hybrid system):**
```c
// Find the player the mob loves most
struct char_data *best_loved = NULL;
int highest_love = 0;

for (each player in room) {
    int love_for_player = get_effective_emotion_toward(mob, player, EMOTION_TYPE_LOVE);
    if (love_for_player > highest_love && love_for_player > 80) {
        highest_love = love_for_player;
        best_loved = player;
    }
}

if (best_loved) {
    add_follower(mob, best_loved);
}
```

**Why it's better:** Mob specifically follows the player it loves, not just any player.

## When to Use Mood vs. Relationship

### Use MOOD (direct emotion fields) when:

- ✅ Making ambient decisions (social initiation frequency)
- ✅ Environmental triggers (entering dangerous areas)
- ✅ General demeanor changes
- ✅ No specific target is involved
- ✅ Updating emotions from non-entity events
- ✅ **Social selection** (which social to perform - based on general mood)

**Example:**
```c
// Mob enters a dangerous area - update mood
if (ROOM_FLAGGED(room, ROOM_DEATH_TRAP)) {
    adjust_emotion(mob, &mob->ai_data->emotion_fear, 15);
}

// Social initiation uses mood
if (ch->ai_data->emotion_happiness >= 70) {
    social_chance += 15;  // More likely to initiate socials when generally happy
}

// Social selection uses mood
int mob_anger = ch->ai_data->emotion_anger;  // Check general anger
if (mob_anger >= 80) {
    perform_negative_social();  // Mob is generally angry
}
```

### Use RELATIONSHIP (hybrid function) when:

- ✅ Making decisions about specific players/mobs
- ✅ Personalized behavior (pricing, quest rewards)
- ✅ Combat decisions toward specific opponents
- ✅ Display emotions to specific viewers
- ✅ Any interaction where target matters
- ✅ **NOT social selection** (socials use mood, not relationship - see below)

**Example:**
```c
// Shopkeeper decides whether to serve this customer
int trust = get_effective_emotion_toward(keeper, customer, EMOTION_TYPE_TRUST);
if (trust < 30) {
    refuse_service(keeper, customer);
}
```

## Memory System Details

### Memory Creation

Memories are automatically created when emotion update functions are called with a target:

```c
// This creates a memory entry
update_mob_emotion_attacked(mob, attacker);
update_mob_emotion_healed(mob, healer);
update_mob_emotion_from_social(mob, actor, "hug");
// etc.
```

The `add_emotion_memory()` function is called internally and stores:
- Complete emotion snapshot (all 20 emotions)
- Interaction type (attacked, healed, social, etc.)
- Timestamp (for age weighting)
- Major event flag (for importance weighting)
- Entity ID and type

### Memory Weighting

**By Age:**
- Recent (< 5 min): Full weight
- Fresh (5-10 min): 80% weight
- Moderate (10-30 min): 60% weight
- Old (30-60 min): 40% weight
- Ancient (> 1 hour): 20% weight

**By Importance:**
- Major events (rescue, theft, betrayal, ally death): 2x weight
- Normal interactions: 1x weight

**Weighted Average:**
```c
effective_emotion = Σ(emotion_value * weight) / Σ(weight)
```

### Memory Limits

- **Size:** 10 entries per mob (circular buffer)
- **Scope:** Only tracks recent interactions
- **Cleanup:** Old entries overwritten by new ones
- **Reset:** Memories cleared on zone reset (by design)

## Advanced Topics

### Relationship Modifier Calculation

The relationship emotion is used as a modifier to mood:

```c
// Get relationship emotion (0-100 from memories)
int relationship = get_relationship_emotion(mob, target, emotion_type);

// Calculate modifier (deviation from neutral = 50)
int modifier = relationship - 50;  // Range: -50 to +50

// Apply modifier at 60% strength to avoid extremes
int adjusted_modifier = modifier * 60 / 100;  // Range: -30 to +30

// Combine with mood
int effective = mood + adjusted_modifier;

// Clamp to valid range
effective = CLAMP(effective, 0, 100);
```

**Why 60%?**
- Prevents relationship from completely overriding mood
- Allows mood to still influence behavior
- Creates balanced, realistic emotions

### Personality Profiles

Mobs have emotional profiles that affect mood baselines:

```c
EMOTION_PROFILE_NEUTRAL      // Balanced emotions (default)
EMOTION_PROFILE_AGGRESSIVE   // High anger, low trust/friendship
EMOTION_PROFILE_DEFENSIVE    // High fear/caution, low trust
EMOTION_PROFILE_BALANCED     // Moderate all emotions
EMOTION_PROFILE_SENSITIVE    // High empathy, low aggression
EMOTION_PROFILE_CONFIDENT    // High courage, low fear
EMOTION_PROFILE_GREEDY       // High greed/envy, low compassion
EMOTION_PROFILE_LOYAL        // High loyalty/trust, high friendship
```

These influence:
- Initial emotion values
- Decay rates (emotions return to profile baseline)
- Reaction intensity to events

### Configuration

The system respects the config flag:
```c
CONFIG_MOB_CONTEXTUAL_SOCIALS
```

When disabled:
- Emotions still tracked
- `get_effective_emotion_toward()` returns 0
- Falls back to traditional behavior

### Combat Emotion Effects (NEW)

The hybrid system now affects combat effectiveness through anger and pain emotions:

#### Anger Increases Attack Power

**Attack Frequency:**
- When anger toward opponent >= threshold (default: 70)
- Chance for extra attack per round (default: 25% chance)
- Uses hybrid system: `get_effective_emotion_toward(mob, target, EMOTION_TYPE_ANGER)`
- Example: Mob furious at Player A gets extra attacks against them specifically

**Damage Bonus:**
- High anger adds damage bonus (default: +15%)
- Applied after all other damage calculations
- Personalized: anger toward THIS opponent matters
- Example: 100 base damage + 15% anger = 115 damage

Configuration:
```c
CONFIG_EMOTION_COMBAT_ANGER_HIGH_THRESHOLD  // Default: 70
CONFIG_EMOTION_COMBAT_ANGER_DAMAGE_BONUS    // Default: 15 (%)
CONFIG_EMOTION_COMBAT_ANGER_ATTACK_BONUS    // Default: 25 (%)
```

#### Pain Reduces Combat Effectiveness

Pain is **mood-based** (not relational) - physical suffering affects all combat:

**Accuracy Penalty (THAC0):**
- Low pain (>= 30): +1 THAC0 (worse accuracy)
- Moderate pain (>= 50): +2 THAC0
- High pain (>= 70): +4 THAC0 (significantly worse)
- Higher THAC0 = harder to hit

**Damage Reduction:**
- Low pain: -5% damage output
- Moderate pain: -10% damage output
- High pain: -20% damage output
- Applied after bonuses, before minimum damage

Configuration:
```c
CONFIG_EMOTION_COMBAT_PAIN_LOW_THRESHOLD        // Default: 30
CONFIG_EMOTION_COMBAT_PAIN_MODERATE_THRESHOLD   // Default: 50
CONFIG_EMOTION_COMBAT_PAIN_HIGH_THRESHOLD       // Default: 70
CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_LOW // Default: 1
CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_MOD // Default: 2
CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_HIGH // Default: 4
CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_LOW   // Default: 5 (%)
CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_MOD   // Default: 10 (%)
CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_HIGH  // Default: 20 (%)
```

**Example Combat Scenario:**
```
Mob fighting Player A (whom it's very angry at):
- Effective anger toward Player A: 85 (>= 70 threshold)
- Pain level (mood): 45 (moderate pain zone)

Effects:
- 25% chance for extra attack this round (anger)
- +15% damage on successful hits (anger)
- +2 THAC0 penalty to hit (pain, accuracy)
- -10% final damage output (pain, effectiveness)

Net result: More aggressive but less effective due to injuries
```

## Best Practices

### DO:
✅ Always use `get_effective_emotion_toward()` for per-entity decisions  
✅ Use emotion type constants (EMOTION_TYPE_*)  
✅ Check for NULL targets when appropriate  
✅ Update memories when emotions change due to interactions  
✅ Consider both mood and relationship in complex decisions  

### DON'T:
❌ Don't access emotion fields directly for per-player decisions  
❌ Don't forget to pass target when it matters  
❌ Don't assume relationship emotions exist (they may not have memories yet)  
❌ Don't create complex emotion logic without mood + relationship  

## Special Case: Social System Design

### Why Socials Use Mood (Not Relationship)

The social system (`mob_contextual_social` and social initiation) intentionally uses **mood only**, not the hybrid system. This is a deliberate design choice:

**Social Initiation (Frequency):**
```c
// Uses mood to decide HOW OFTEN to perform socials
if (ch->ai_data->emotion_happiness >= 70) {
    social_chance += 15;  // Happy mobs socialize more
}
if (ch->ai_data->emotion_anger >= 70) {
    social_chance += 10;  // Angry mobs also socialize (negatively)
}
```

**Social Selection (Which Social):**
```c
// Uses mood to decide WHICH TYPE of social
int mob_anger = ch->ai_data->emotion_anger;  // General anger
int mob_fear = ch->ai_data->emotion_fear;    // General fear

if (mob_anger >= 80) {
    perform_negative_social(target);  // Glare, frown, etc.
} else if (mob_fear >= 70) {
    perform_fearful_social(target);   // Cower, cringe, etc.
}
```

**Why Mood, Not Relationship?**

1. **Ambiance:** Socials create atmospheric behavior. A generally angry mob should look angry to everyone in the room, not just to specific players.

2. **Observability:** Other players in the room see the social. If Mob glares at Player A because of relationship history, Player B watching doesn't understand why. Using mood makes behavior predictable.

3. **Performance:** Checking relationship for every potential target in a room during social selection would be expensive.

4. **Emotion Updates:** When the target receives the social, `update_mob_emotion_from_social()` creates a memory, building relationship over time.

**The Flow:**
```
1. Mood determines: Should I do a social? (initiation frequency)
2. Mood determines: What type? (angry/happy/fearful social)
3. Target is selected from room
4. Social is performed
5. Relationship memory is created with target
6. Future interactions (combat, trading, quests) use relationship
```

**Example Scenario:**
```
Mob has:
- Mood: anger = 85 (generally angry from being attacked)
- Relationship: anger toward Player A = 90 (attacked it)
- Relationship: anger toward Player B = 20 (helped it)

Social Behavior:
- Mob's mood (85) makes it likely to perform negative socials
- Mob might glare at anyone in room (mood-driven selection)
- But when trading: refuses Player A, gives discount to Player B (relationship)
- And in combat: extra attacks against Player A only (relationship)
```

### Systems Using Mood vs Relationship

**MOOD ONLY (Global State):**
- ✅ Social initiation frequency
- ✅ Social type selection
- ✅ Pain effects on combat
- ✅ Environmental fear/happiness
- ✅ Passive emotion decay

**RELATIONSHIP (Hybrid Per-Target):**
- ✅ Shop pricing/service
- ✅ Quest acceptance/rewards
- ✅ Combat anger bonuses
- ✅ Combat flee decisions
- ✅ Emotion display to viewer
- ✅ Love-based following

**BOTH (Context-Dependent):**
- ✅ Social reactions: Mood picks social, relationship memory created
- ✅ Combat: Pain uses mood, anger uses relationship

## Testing Your Implementation

### Test Scenarios:

1. **Relationship Building:**
   - Have Player A heal a mob 5 times
   - Check if trust/friendship toward Player A increases
   - Have Player B attack the same mob
   - Verify mob treats them differently

2. **Memory Persistence:**
   - Interact with mob, then leave for 5 minutes
   - Return and check if mob remembers you
   - Verify behavior differs from first-time visitor

3. **Mood vs Relationship:**
   - Set mob's general fear high (dangerous area)
   - Have one player help mob, another threaten it
   - Verify flee threshold differs per player

4. **Memory Limits:**
   - Create 15 interactions (more than buffer size of 10)
   - Verify oldest memories are replaced
   - Check weighting still works correctly

## Migration Guide

If you're updating old emotion code:

**Old Pattern:**
```c
if (mob->ai_data->emotion_trust > 60) {
    // Do something
}
```

**New Pattern (with target):**
```c
int trust = get_effective_emotion_toward(mob, target, EMOTION_TYPE_TRUST);
if (trust > 60) {
    // Do something personalized
}
```

**New Pattern (no target):**
```c
int trust = get_effective_emotion_toward(mob, NULL, EMOTION_TYPE_TRUST);
if (trust > 60) {
    // Do something based on mood
}
```

## Performance Considerations

- **Memory overhead:** 10 emotion_memory structs per mob (~1KB)
- **CPU overhead:** Minimal - only on decision points, not every tick
- **Lookup time:** O(10) linear search through memories (constant time)
- **Memory creation:** O(1) circular buffer insertion

The hybrid system is designed to be lightweight and efficient.

## 4D Emotion Projection Component

### Overview

The **4D Emotion Projection** layer (`emotion_projection.c` / `emotion_projection.h`) converts a mob's 20-component emotion vector into a compact **4-dimensional decision space** used by the mob AI pipeline when choosing actions.

The four axes are:

| Axis | Range | Meaning |
|------|-------|---------|
| **Valence** | −100 to +100 | Positive vs. negative evaluation of the current interaction |
| **Arousal** | 0 to 100 | Calm (0) to highly activated (100) |
| **Dominance** | −100 to +100 | Perceived control / assertiveness bias |
| **Affiliation** | −100 to +100 | Avoidance (−100) to approach (+100) toward target |

### Core Formula

```
P_base      = M_profile × E
P_raw       = (M_profile + ΔM_personal) × E
P_effective = ContextualMod(P_raw, mob, target, environment, memory, shadow)
```

Where:
- `M_profile` — Fixed 4×20 projection matrix for the mob's emotional profile (NEUTRAL, AGGRESSIVE, etc.)
- `ΔM_personal` — Personal drift matrix, bounded within ±`PERSONAL_DRIFT_MAX_PCT`%
- `E` — Current 20-component emotion vector (each value 0–100)
- `P_base` — Profile-only projection, no drift, no context
- `P_raw` — Drift-adjusted projection before contextual modulation
- `P_effective` — Final 4D state after contextual modulation

### Contextual Modulation Layer

After computing `P_raw`, four adjustments are applied:

```
Dominance   = clamp(raw_D  + (coping_potential − 50) × 0.4)
Arousal     = clamp(raw_A  × (1 + env_intensity × 0.5))
Affiliation = clamp(raw_Af + relationship_bias)
Valence     = clamp(raw_V  + shadow_forecast_bias)   // only if Shadow Timeline active
```

**Coping Potential** (`emotion_compute_coping_potential()`) is an objective measure of the mob's situational capacity (HP ratio, mana ratio, status effects, group numbers), distinct from the subjective Dominance axis it modulates.

### Relationship to Mood and Relationship Layers

| Component | Source | Influence on Projection |
|-----------|--------|------------------------|
| Mood layer | `mob->ai_data->emotion_*` | Primary input emotion vector `E` |
| Relationship layer | `mob->ai_data->memories[]` | Contributes `relationship_bias` to Affiliation axis |
| Shadow Timeline | `mob->ai_data->attention_bias` | Contributes `shadow_forecast_bias` to Valence axis |
| Emotional Profile | `mob->ai_data->emotional_profile` | Selects the fixed projection matrix `M_profile` |

The projection is **read-only** with respect to the emotion state: it maps emotions to a decision coordinate but never writes back to the emotion fields directly. This preserves the separation between the reactive emotion pipeline and the strategic decision layer.

### API

```c
// Get the fixed profile projection matrix for a given emotional profile
const float (*emotion_get_profile_matrix(int profile))[20];

// Compute P_raw for a mob
void emotion_compute_raw_projection(struct char_data *mob, float raw_out[DECISION_SPACE_DIMS]);

// Compute objective coping potential (0–100)
float emotion_compute_coping_potential(struct char_data *mob);

// Apply contextual modulation to produce P_effective
void emotion_apply_contextual_modulation(struct char_data *mob, struct char_data *target,
                                         const float raw[DECISION_SPACE_DIMS], float coping_pot,
                                         float effective_out[DECISION_SPACE_DIMS]);
```

### Files

- `src/emotion_projection.h` — API documentation and type definitions
- `src/emotion_projection.c` — Matrix definitions and modulation logic
- `src/mobact.c` — Integration point (reads `P_effective` for mob action scoring)
- `src/act.wizard.c` — `stat mob` display of projected OCEAN values

## Conclusion

The hybrid emotion system provides realistic, long-term relationship building while maintaining simple, performant code. Always use `get_effective_emotion_toward()` for per-entity decisions, and let the system handle the complexity of combining mood and relationship layers.

For more details, see:
- `EMOTION_SYSTEM_TODO.md` - Feature roadmap and architecture
- `src/utils.c` - Implementation of hybrid functions
- `src/structs.h` - Data structures and constants
