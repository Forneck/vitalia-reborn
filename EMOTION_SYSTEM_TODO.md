# Emotion System Enhancement TODOs

This document outlines potential enhancements to the mob emotion system in Vitalia Reborn.

## Current Status
The emotion system tracks 20 emotions across 5 categories and updates them based on:
- Social interactions (socials)
- Combat (attacking, being attacked, witnessing death)
- Support actions (healing, rescue, assist)
- Item transactions (giving, stealing)
- Passive regulation (emotion decay)

## Enhancement Categories

### 1. Emotion-Driven Mob Behavior (HIGH PRIORITY)

Currently, emotions are tracked and updated but only minimally influence mob decisions. Emotions should drive more behaviors:

#### 1.1 Combat Behavior
- **TODO**: High fear (>70) should increase flee probability during combat
- **TODO**: High anger (>70) should increase attack frequency/damage
- **TODO**: High courage (>70) should reduce flee probability
- **TODO**: Pain level should affect combat effectiveness (accuracy/damage penalties)
- **TODO**: Horror level (>80) should trigger panic flee regardless of courage

#### 1.2 Trading Behavior
- **TODO**: High trust (>60) should give better shop prices
- **TODO**: Low trust (<30) should make shopkeepers refuse service
- **TODO**: High greed should increase shop prices for buying
- **TODO**: High friendship should give discounts

#### 1.3 Quest Behavior
- **TODO**: High curiosity should make mobs more likely to offer quests
- **TODO**: High loyalty should make mobs remember past quest helpers
- **TODO**: High trust should make quest rewards better
- **TODO**: Low trust should make mobs refuse to give quests

#### 1.4 Social Initiation
- **TODO**: Mobs with high happiness should initiate positive socials more often
- **TODO**: Mobs with high anger should initiate negative socials
- **TODO**: Mobs with high sadness should withdraw and social less
- **TODO**: Mobs with high love (>80) should follow players they love

#### 1.5 Group Behavior
- **TODO**: High loyalty should make mobs stay in group even when hurt
- **TODO**: Low loyalty should make mobs abandon group when scared
- **TODO**: High friendship should make mobs more likely to join groups
- **TODO**: High envy should make mobs refuse to group with better-equipped players

### 2. Additional Emotion Triggers (MEDIUM PRIORITY)

Actions that should trigger emotion updates but currently don't:

#### 2.1 Communication Actions
- **TODO**: Being insulted/cursed at (chat/say) → anger, shame
- **TODO**: Being praised/thanked (chat/say) → happiness, pride
- **TODO**: Being ignored (player walks away mid-conversation) → sadness, anger
- **TODO**: Receiving tells/whispers from strangers → curiosity or suspicion

#### 2.2 Environmental Triggers
- **TODO**: Witnessing player death → fear, sadness (if friendly), satisfaction (if enemy)
- **TODO**: Seeing powerful equipment → envy
- **TODO**: Entering dangerous areas → fear increase
- **TODO**: Returning to safe areas → fear decrease, happiness increase

#### 2.3 Magic/Spell Effects
- **TODO**: Being cursed/harmed by spells → anger, fear, pain
- **TODO**: Being blessed/buffed → happiness, trust, gratitude
- **TODO**: Witnessing offensive magic → fear, horror (for non-combatants)

#### 2.4 Quest-Related Triggers
- **TODO**: Quest completion → happiness, trust, friendship increase
- **TODO**: Quest failure → anger, disappointment, trust decrease
- **TODO**: Quest betrayal (killing quest giver) → horror, anger for witnesses

#### 2.5 Economic Actions
- **TODO**: Being robbed (shopping) → anger, distrust
- **TODO**: Receiving fair trade → trust, happiness
- **TODO**: Selling valuable items to mob → greed response

### 3. Emotion Persistence & Memory (HIGH PRIORITY)

#### 3.1 Emotion History
- **TODO**: Track emotion history per player (last 10 interactions)
- **TODO**: Long-term relationships should build over multiple encounters
- **TODO**: Negative events should be remembered longer than positive ones
- **TODO**: Major events (rescue, theft, ally death) should have lasting impact

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
- **TODO**: Nearby mobs should be influenced by each other's emotions
- **TODO**: Fear should spread among grouped mobs
- **TODO**: Excitement/happiness should be contagious in crowds
- **TODO**: Leader's emotions should influence followers more

#### 5.2 Emotional Intelligence Variation
- **TODO**: Add emotional_intelligence stat to mob genetics
- **TODO**: High EI mobs should have more nuanced emotional responses
- **TODO**: Low EI mobs should have more extreme, volatile emotions
- **TODO**: EI should affect how quickly emotions stabilize

#### 5.3 Mood System
- **TODO**: Add overall mood derived from emotion averages
- **TODO**: Mood should affect all interactions globally
- **TODO**: Extreme moods should trigger special behaviors
- **TODO**: Weather/time of day should influence mood

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
- **TODO**: Define behaviors for maxed-out emotions (100)
- **TODO**: Define behaviors for minimized emotions (0)
- **TODO**: Handle conflicting extreme emotions (high anger + high fear)
- **TODO**: Add "emotional breakdown" state for too many high emotions

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

### Phase 1 (Immediate - Next Update)
1. Emotion-driven combat flee behavior (1.1)
2. Emotion persistence/memory system (3.1)
3. Basic emotion indicators in descriptions (4.1)

### Phase 2 (Short-term)
1. Trading behavior influenced by emotions (1.2)
2. Additional emotion triggers for spells/magic (2.3)
3. Emotion display improvements (4.1)
4. Balance and tuning (6.1, 6.2)

### Phase 3 (Medium-term)
1. Quest behavior integration (1.3)
2. Emotion contagion basics (5.1)
3. Player feedback tools (7.1, 7.2)
4. Faction system integration (8.2)

### Phase 4 (Long-term)
1. Advanced emotion mechanics (5.2, 5.3, 5.4)
2. NPC dialogue adaptation (4.2)
3. Special cases handling (9.1, 9.2, 9.3)
4. Complete system integration (8.1, 8.3)

## Notes

- All enhancements should respect the CONFIG_MOB_CONTEXTUAL_SOCIALS flag
- Performance must be monitored when adding emotion-based decisions
- Backward compatibility must be maintained
- Each enhancement should be individually toggleable for testing
- Documentation must be updated for each implemented feature

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
