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

**Example:**
```c
// Mob enters a dangerous area - update mood
if (ROOM_FLAGGED(room, ROOM_DEATH_TRAP)) {
    adjust_emotion(mob, &mob->ai_data->emotion_fear, 15);
}
```

### Use RELATIONSHIP (hybrid function) when:

- ✅ Making decisions about specific players/mobs
- ✅ Personalized behavior (pricing, quest rewards)
- ✅ Combat decisions toward specific opponents
- ✅ Display emotions to specific viewers
- ✅ Any interaction where target matters

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

## Conclusion

The hybrid emotion system provides realistic, long-term relationship building while maintaining simple, performant code. Always use `get_effective_emotion_toward()` for per-entity decisions, and let the system handle the complexity of combining mood and relationship layers.

For more details, see:
- `EMOTION_SYSTEM_TODO.md` - Feature roadmap and architecture
- `src/utils.c` - Implementation of hybrid functions
- `src/structs.h` - Data structures and constants
