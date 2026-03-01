# Emotion System Enhancement TODOs

This document outlines potential enhancements to the mob emotion system in Vitalia Reborn.

## Current Status - HYBRID EMOTION SYSTEM (v2.0)

The emotion system now implements a **hybrid model** with two layers:

### Layer 1: MOOD (Global State)
- 20 emotion fields in mob_ai_data (emotion_fear, emotion_anger, etc.)
- Represents the mob's general emotional state
- Influenced by:
  - Environmental factors (dangerous areas, safe zones)
  - Time-based effects (weather, time of day - future)
  - Recent general experiences
  - Passive regulation (emotion decay toward baseline)
  - Genetic personality traits (emotional_profile)

### Layer 2: RELATIONSHIP (Per-Player/Entity)
- Circular buffer of 10 emotion_memory entries per mob
- Each memory stores a complete emotion snapshot after interactions
- Memories are weighted by:
  - Age (recent = more weight)
  - Major events (2x weight for rescue, theft, betrayal, etc.)
  - Interaction type (attacked, healed, social, quest, etc.)
- Function: `get_effective_emotion_toward(mob, target, emotion_type)` combines:
  - **Mood** (base emotion) + **Relationship** (modifier from memories)
  - Returns personalized emotion value for decisions

### Implementation Details
- **COMPLETED**: Core hybrid system functions:
  - `get_effective_emotion_toward()` - combines mood + relationship
  - `get_relationship_emotion()` - extracts per-player emotion from memories
  - 20 EMOTION_TYPE_* constants for identifying emotions
- **COMPLETED**: Integration into decision points:
  - Shop behavior (trust/greed/friendship per player)
  - Combat flee (fear/courage toward specific attacker)
  - Quest system (trust per player)
  - Emotion display (shows feelings toward viewer)
  - Love-based following (targets specific beloved players)

## Enhancement Categories

### 1. Emotion-Driven Mob Behavior

#### 1.1 Combat Behavior
- **DONE**: High fear (>70) increases flee probability during combat (uses hybrid system)
- **DONE**: High courage (>70) reduces flee probability (uses hybrid system)
- **DONE**: Horror level (>80) triggers panic flee (uses hybrid system)
- **DONE**: High anger (>70) increases attack frequency and damage (uses hybrid system - per target)
- **DONE**: Pain level affects combat effectiveness (accuracy/damage penalties, mood-based)

#### 1.2 Trading Behavior
- **DONE**: High trust (>60) gives better shop prices (uses hybrid system - per player)
- **DONE**: Low trust (<30) makes shopkeepers refuse service (uses hybrid system - per player)
- **DONE**: High greed increases shop prices (uses hybrid system - per player)
- **DONE**: High friendship gives discounts (uses hybrid system - per player)

#### 1.3 Quest Behavior
- **DONE**: Low trust makes mobs refuse to give quests (uses hybrid system - per player)
- **DONE**: High trust gives better quest rewards (uses hybrid system - per player)
- **TODO**: High curiosity should make mobs more likely to offer quests
- **TODO**: High loyalty should make mobs remember past quest helpers

#### 1.4 Social Initiation
- **DONE**: Mobs with high happiness initiate positive socials more often (mood-based)
- **DONE**: Mobs with high anger initiate negative socials (mood-based)
- **DONE**: Mobs with high sadness withdraw and social less (mood-based)
- **DONE**: Mobs with high love (>80) follow players they love (uses hybrid system - per player)

#### 1.5 Group Behavior
- **DONE**: High loyalty should make mobs stay in group even when hurt (fight.c - configurable via CEDIT)
- **DONE**: Low loyalty should make mobs abandon group when scared (fight.c - configurable via CEDIT)
- **DONE**: High friendship should make mobs more likely to join groups (mobact.c - configurable via CEDIT)
- **DONE**: High envy should make mobs refuse to group with better-equipped players (mobact.c - configurable via CEDIT)

### 2. Additional Emotion Triggers (MEDIUM PRIORITY)

Actions that should trigger emotion updates but currently don't:

#### 2.1 Communication Actions
- **TODO**: Being insulted/cursed at (chat/say) → anger, shame
- **TODO**: Being praised/thanked (chat/say) → happiness, pride
- **TODO**: Being ignored (player walks away mid-conversation) → sadness, anger
- **TODO**: Receiving tells/whispers from strangers → curiosity or suspicion

#### 2.2 Environmental Triggers
- **DONE**: Witnessing player death → fear, sadness (uses existing system)
- **DONE**: Seeing powerful equipment → envy (uses existing system)
- **DONE**: Entering dangerous areas → fear increase (uses existing system)
- **DONE**: Returning to safe areas → fear decrease, happiness increase (uses existing system)

#### 2.3 Magic/Spell Effects
- **DONE**: Being cursed/harmed by spells → anger, fear, pain (uses existing system)
- **DONE**: Being blessed/buffed → happiness, trust, gratitude (uses existing system)
- **DONE**: Witnessing offensive magic → fear, horror (uses existing system)

#### 2.4 Quest-Related Triggers
- **DONE**: Quest completion → happiness, trust, friendship increase (uses existing system)
- **DONE**: Quest failure → anger, trust decrease (uses existing system)
- **DONE**: Quest betrayal (killing quest giver) → horror, anger (uses existing system)

#### 2.5 Economic Actions
- **DONE**: Being robbed (shopping) → anger, distrust (steal skill triggers emotions)
- **DONE**: Receiving fair trade → trust, happiness (uses existing system)
- **DONE**: Selling valuable items to mob → greed response (uses existing system)

### 3. Emotion Persistence & Memory - HYBRID SYSTEM IMPLEMENTED

#### 3.1 Emotion History
- **DONE**: Track emotion history per player (10 interactions in circular buffer)
- **DONE**: Long-term relationships build over multiple encounters (weighted by age)
- **DONE**: Major events have lasting impact (2x weight: rescue, theft, ally death, etc.)
- **IMPROVEMENT**: Consider longer decay times for negative events vs positive

#### 3.2 Reputation Integration
- **TODO**: Player reputation should initialize mob emotions at first meeting
- **TODO**: High reputation players start with higher trust/friendship
- **TODO**: Low reputation players start with higher suspicion/fear
- **TODO**: Emotion levels should influence reputation gains/losses

### 4. Emotion Display & Communication (LOW PRIORITY)

#### 4.1 Visual Cues
- **TODO**: Add emotion indicators to mob descriptions (angry mob, frightened mob, etc.)
- **TODO**: High emotion levels should show in `look` command
- **TODO**: Add color coding for emotional states in stat display
- **TODO**: Mobs should occasionally emote their emotional state

#### 4.2 NPC Dialogue
- **TODO**: Mob speech patterns should reflect emotions (angry = aggressive, sad = withdrawn)
- **TODO**: High emotion levels should trigger spontaneous statements
- **TODO**: Quest dialogue should adapt based on emotional state
- **TODO**: Shop dialogue should reflect trader's emotions

### 5. Advanced Emotion Mechanics (LOW PRIORITY)

#### 5.1 Emotion Contagion
- **DONE**: Nearby mobs should be influenced by each other's emotions (5-15% transfer)
- **DONE**: Fear should spread among grouped mobs (12-20% transfer in groups)
- **DONE**: Excitement/happiness should be contagious in crowds (8-15% transfer)
- **DONE**: Leader's emotions should influence followers more (2x influence, 15-25% transfer)

#### 5.2 Emotional Intelligence Variation
- **DONE**: Add emotional_intelligence stat to mob genetics (10-95 range)
- **DONE**: High EI mobs should have more nuanced emotional responses (70-90% reaction intensity)
- **DONE**: Low EI mobs should have more extreme, volatile emotions (120-150% reaction intensity)
- **DONE**: EI should affect how quickly emotions stabilize (learning through regulation, regression through overwhelm)

#### 5.3 Mood System
- **DONE**: Add overall mood derived from emotion averages (-100 to +100 scale)
- **DONE**: Mood should affect all interactions globally (apply_mood_modifier function, ±20% effect)
- **DONE**: Extreme moods should trigger special behaviors (socials, emotion adjustments)
- **DONE**: Weather/time of day should influence mood (weather ±15, time ±5)

#### 5.4 Emotion-Based Skills
- **TODO**: Intimidation skill that increases target's fear
- **TODO**: Charm skill that increases target's happiness/love
- **TODO**: Manipulation skill that exploits emotional state
- **TODO**: Empathy skill that reveals mob's emotional state

### 6. Balance & Tuning (ONGOING)

#### 6.1 Decay Rates
- **TODO**: Review and tune passive emotion decay rates
- **TODO**: Different emotions should decay at different rates
- **TODO**: Extreme emotions should decay faster than moderate ones
- **TODO**: Add config options for decay rate multipliers

#### 6.2 Threshold Tuning
- **TODO**: Review all emotional thresholds (currently hardcoded)
- **TODO**: Make thresholds configurable via cedit
- **TODO**: Add per-mob emotional sensitivity settings
- **TODO**: Test and balance emotion-driven behaviors

#### 6.3 Performance Optimization
- **TODO**: Profile emotion update performance
- **TODO**: Consider caching emotion calculations
- **TODO**: Optimize passive emotion updates for large mob counts
- **TODO**: Add emotion system performance monitoring

### 7. Player Feedback & Testing (MEDIUM PRIORITY)

#### 7.1 Visibility
- **TODO**: Add help file explaining emotion system to players
- **TODO**: Add command to see mob's current emotional state (sense_motive/empathy)
- **TODO**: Add logging for emotion-triggered behaviors (for debugging)
- **TODO**: Add builder documentation for emotion-aware quest design

#### 7.2 Testing Tools
- **TODO**: Add wizard commands to manually set mob emotions
- **TODO**: Add emotion event logging for testing
- **TODO**: Create test scenarios for each emotion trigger
- **TODO**: Add emotion visualization tools for builders

### 8. Integration with Existing Systems (MEDIUM PRIORITY)

#### 8.1 Alignment System
- **TODO**: Emotions should influence alignment shifts over time
- **TODO**: High anger/hatred should push toward evil
- **TODO**: High compassion/love should push toward good
- **TODO**: Alignment should influence baseline emotions

#### 8.2 Faction System
- **TODO**: Emotions should affect faction standings
- **TODO**: High loyalty to faction should resist betrayal
- **TODO**: Emotional bonds should override faction hostility sometimes
- **TODO**: Faction enemies should start with negative emotions

#### 8.3 Deity/Religion System
- **TODO**: Religious mobs should have modified emotional responses
- **TODO**: Prayers/blessings should affect emotions
- **TODO**: Divine intervention should reset certain emotions
- **TODO**: Religious conflicts should trigger appropriate emotions

### 9. Special Cases & Edge Cases (LOW PRIORITY)

#### 9.1 Extreme Emotional States
- **DONE**: Define behaviors for maxed-out emotions (100) - special actions + temporary affects
  - Fear 100: AFF_PARALIZE for 1 tick
  - Anger 100: Berserk state (extra attack, +25% damage, -3 accuracy) for 2-4 ticks
- **DONE**: Define behaviors for minimized emotions (0) - fearless, paranoid, callous, depressed states
- **DONE**: Handle conflicting extreme emotions (high anger + high fear) - fight-or-flight resolution, 5 conflict types
- **DONE**: Add "emotional breakdown" state for too many high emotions - triggers at 4+ extremes or 450+ intensity

#### 9.2 Emotional Immunity
- **TODO**: Some mob types should be immune to certain emotions (undead, constructs)
- **TODO**: Add MOB_FLAG for emotional immunity
- **TODO**: Psychopaths should have reduced empathic emotions
- **TODO**: Enlightened beings should have muted negative emotions

#### 9.3 Cultural Variations
- **TODO**: Different mob races could have different emotional baselines
- **TODO**: Cultural norms should affect emotional expression
- **TODO**: Honor-bound cultures should have higher pride/shame sensitivity
- **TODO**: Tribal cultures should have stronger loyalty responses

## Implementation Priority

### Phase 1 (COMPLETED - Hybrid System v2.0)
1. ✅ Emotion-driven combat flee behavior (1.1) - uses hybrid system
2. ✅ Emotion persistence/memory system (3.1) - hybrid model implemented
3. ✅ Basic emotion indicators in descriptions (4.1) - uses hybrid system per viewer
4. ✅ Trading behavior influenced by emotions (1.2) - uses hybrid system per player
5. ✅ Quest behavior integration (1.3) - uses hybrid system per player
6. ✅ Love-based following (1.4) - uses hybrid system to target specific players
7. ✅ High anger affecting attack behavior (1.1) - extra attacks and damage bonus
8. ✅ Pain affecting combat effectiveness (1.1) - accuracy and damage penalties
9. ✅ Group behavior emotional integration (1.5) - loyalty, friendship, and envy affecting group dynamics

### Phase 2 (COMPLETED - Contagion & Advanced Mechanics)
1. ✅ Emotion contagion basics (5.1) - crowd, group, and leader layers implemented
2. ✅ Emotional Intelligence variation (5.2) - 120-150% volatile / 70-90% stable
3. ✅ Mood system (5.3) - overall mood, weather/time influence, extreme mood behaviours
4. ✅ Environmental emotion triggers (2.2) - dangerous areas, safe zones, spells
5. ✅ Quest emotion triggers (2.4) - completion, failure, betrayal
6. ✅ Economic emotion triggers (2.5) - robbery, fair trade, selling valuable items

### Phase 3 (Short-term - Next Updates)
1. Additional emotion triggers for communication (2.1)
2. Balance and tuning (6.1, 6.2)
3. Player feedback tools (7.1, 7.2)
4. Faction system integration (8.2)
5. Reputation system integration (3.2)

### Phase 4 (Long-term)
1. Emotion-based skills (5.4)
2. NPC dialogue adaptation (4.2)
3. Special cases handling (9.1, 9.2, 9.3)
4. Complete system integration (8.1, 8.3)

## Notes

### Hybrid System Architecture
- **Mood Layer**: Represents general emotional state (current emotion_* fields)
  - Influenced by environment, time, general experiences
  - Provides baseline for all interactions
- **Relationship Layer**: Per-entity emotional memory (emotion_memory buffer)
  - Tracks specific feelings toward individual players/mobs
  - Weighted by recency and event importance
  - Retrieved via `get_effective_emotion_toward()`
- **Decision Making**: Always use `get_effective_emotion_toward()` for personalized behavior
  - Shop pricing, quest acceptance, combat decisions, etc.
  - Returns: mood_emotion + relationship_modifier
- **Display**: Show effective emotions toward viewer for realistic perception

### Development Guidelines
- All enhancements should respect the CONFIG_MOB_CONTEXTUAL_SOCIALS flag
- Performance must be monitored when adding emotion-based decisions
- Backward compatibility must be maintained
- Each enhancement should be individually toggleable for testing
- Documentation must be updated for each implemented feature
- Always use hybrid system functions for per-entity decisions

## Related Files

- `src/utils.c` - Core emotion update functions
- `src/utils.h` - Emotion function declarations
- `src/mobact.c` - Mob behavior and emotion activity
- `src/structs.h` - Emotion data structures
- `src/act.wizard.c` - Stat command display
- `src/act.social.c` - Social interaction handling
- `src/fight.c` - Combat emotion triggers
- `src/magic.c` - Spell emotion triggers
- `src/act.item.c` - Item transaction triggers
- `src/act.offensive.c` - Combat action triggers
- `src/act.other.c` - Miscellaneous action triggers

## Contributing

When implementing any of these TODOs:
1. Update this document to mark the TODO as completed
2. Add tests for the new functionality
3. Update relevant documentation
4. Run performance tests for behavior-changing features
5. Update the changelog with the enhancement
