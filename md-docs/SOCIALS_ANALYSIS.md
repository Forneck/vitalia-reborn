# Socials System Analysis - Vitalia Reborn

## Executive Summary

This document provides a comprehensive analysis of the socials system in Vitalia Reborn, specifically addressing:
1. **Mob-to-mob social interactions** - verification and capabilities
2. **Inappropriate socials** - identification and recommendations
3. **Silly/nonsense socials** - categorization and usage guidelines
4. **Emotion categories** - current implementation and potential expansions

**Key Finding**: ✅ Mobs can already use socials with each other. The existing implementation in `mob_emotion_activity()` (mobact.c) does not filter by IS_NPC, allowing mobs to target other mobs naturally.

---

## 1. Mob-to-Mob Social Capability

### Current Implementation Status: ✅ FULLY FUNCTIONAL

**Code Analysis** (`src/mobact.c` lines 358-415):
```c
void mob_emotion_activity(void)
{
    // ... 
    for (potential_target = world[IN_ROOM(ch)].people; potential_target;
         potential_target = potential_target->next_in_room) {
        if (potential_target == ch)
            continue;
        
        // No IS_NPC filter - mobs can target ANY character including other mobs
        if (!CAN_SEE(ch, potential_target))
            continue;
        
        mob_contextual_social(ch, potential_target);
        break;
    }
}
```

**Verification Points**:
- ✅ Target iteration includes all characters in room (line 388)
- ✅ No IS_NPC filter prevents mob-to-mob interactions
- ✅ `CAN_SEE()` check works for mob-to-mob visibility
- ✅ `do_action()` social execution handles any char_data target
- ✅ Social messages display correctly for mob-to-mob interactions

**Configuration**:
- Controlled by `CONFIG_MOB_CONTEXTUAL_SOCIALS` flag
- Social frequency controlled by `CONFIG_MOB_EMOTION_SOCIAL_CHANCE` (default probability)
- Must be enabled in game configuration (cedit menu)

---

## 2. Inappropriate Socials Analysis

### Identified Inappropriate Socials: 34

These socials contain sexual, violent, or otherwise inappropriate content that should NOT be used by mobs automatically:

#### Sexual/Suggestive Content (17 socials):
- `custard` - contains suggestive content
- `dive` - contains diving into body parts
- `earlick` - licking ears (suggestive)
- `fondle` - inappropriate touching
- `french` - french kissing (sexual)
- `grope` - inappropriate touching
- `joint` - drug reference
- `licks` - suggestive licking
- `massage` - potentially suggestive
- `moan` - sexual sound
- `pant` - heavy breathing (suggestive)
- `seduce` - explicitly sexual
- `sex` - explicitly sexual
- `spank` - physical punishment (potentially sexual)
- `titter` - giggling in suggestive context
- `vampire` - biting necks (suggestive)
- `whip` - BDSM reference

#### Violent/Disturbing Content (10 socials):
- `despine` - removing spine (extremely violent)
- `haircut` - forceful cutting
- `halo` - religious imagery that may be inappropriate
- `knuckle` - knuckle sandwich (punch)
- `needle` - stabbing with needle
- `shock` - electric shock
- `shiskabob` - impaling on sword
- `spork` - stabbing with spork
- `vice` - crushing in vice grip

#### Inappropriate Gestures (7 socials):
- `boo` - contains potentially offensive "boo-bies" reference
- `flick` - flicking inappropriate body parts
- `hula` - may contain inappropriate gyrating
- `modest` - exposing/covering private areas
- `newidea` - lightbulb appearing (may be immersion-breaking)
- `pushup` - may contain inappropriate positioning
- `rose` - may contain inappropriate romantic gesture ("between teeth")
- `suckit-up` - rude/offensive phrase

### Recommendations for Inappropriate Socials

**Option 1: Minimum Level Restriction**
Add high `min_level_char` values (e.g., 31 - immortal only) to restrict to admin/roleplay only:
```
~seduce seduce 0 5 31 0
```

**Option 2: Mob-Usable Flag (Future Enhancement)**
Extend the social_messg structure to include a `mob_usable` flag:
```c
struct social_messg {
    // ... existing fields ...
    bool mob_usable;  /* If FALSE, mobs cannot use this social automatically */
};
```

**Option 3: Remove from Game**
Consider removing the most egregious socials entirely from socials.new if they don't fit the game's tone.

**Recommended Action**:
Implement Option 1 immediately (set min_level_char = 31) for all inappropriate socials, and consider Option 2 as a future enhancement.

---

## 3. Silly/Nonsense Socials Analysis

### Identified Silly/Nonsense Socials: 12

These socials are humorous, absurd, or potentially immersion-breaking:

#### Bodily Function Humor (7 socials):
- `belch` - belching/burping sounds
- `booger` - picking nose and flicking boogers
- `burp` - burping
- `drool` - drooling excessively
- `fart` - passing gas
- `puke` - vomiting
- `raspberry` - blowing raspberries (fart noise with mouth)

#### Absurd/Nonsensical (5 socials):
- `abc` - "ABCD LSD, Gummy Bears are chasing me" (drug reference + absurdity)
- `howl` - may be appropriate for werewolves but silly for most mobs
- `moon` - showing buttocks
- `noogie` - rubbing knuckles on head
- `wedgie` - pulling underwear up

### Categorization for Silly Socials

**Category A: Juvenile Humor** (should be rare on mobs)
- bodily function socials: belch, booger, burp, drool, fart, puke, raspberry
- physical pranks: moon, noogie, wedgie

**Category B: Potentially Appropriate for Specific Mobs**
- `howl` - appropriate for wolves, werewolves, wild beasts
- `abc` - could be used by insane/deranged characters only

### Recommendations for Silly/Nonsense Socials

**Tiered Approach**:

1. **Completely Restrict from Normal Mobs** (Category A):
   - Set high min_level_char or add to exclusion list
   - Only allow for specially flagged "comic relief" mobs (jesters, clowns, comedians)

2. **Situational Use** (Category B):
   - `howl` - only for canine/wolf mob types (can be implemented via spec_proc)
   - `abc` - only for mobs with "insane" or "deranged" flags

3. **Create Mob Personality Flags** (Future Enhancement):
   ```c
   #define MOB_SERIOUS      (1 << 0)  /* Never uses silly socials */
   #define MOB_COMEDIC      (1 << 1)  /* Can use silly socials */
   #define MOB_ANIMALISTIC  (1 << 2)  /* Can use howl, growl, etc. */
   ```

**Recommended Action**:
Add a silly_socials exclusion list in mob_contextual_social() that prevents normal mobs from using these unless they have special flags.

---

## 4. Current Emotion Categories in Mob System

### Implemented Categories (21 total, ~120 socials)

The current implementation in `mobact.c` includes these emotion-based social categories:

#### Core Emotional Categories (14 original)

#### 1. **positive_socials** (18 socials)
**Usage**: High reputation targets, good alignment, friendly mobs
- bow, smile, nods, waves, applaud, agree, beam, clap, grin, greet, thanks, thumbsup, welcome, winks, backclap, sweetsmile, happy

#### 2. **negative_socials** (12 socials)
**Usage**: Low reputation targets, angry mobs, alignment conflicts
- frown, glare, spit, sneer, accuse, growl, snarl, curse, mock, hate, steam, swear

#### 3. **neutral_socials** (10 socials)
**Usage**: Default behavior, moderate emotions, observing
- ponder, shrugs, peer, blink, wonder, think, wait, scratch, yawn, stretch

#### 4. **resting_socials** (6 socials)
**Usage**: Target is resting/sitting, compassionate mobs
- comfort, pat, calm, console, cradle, tuck

#### 5. **fearful_socials** (9 socials)
**Usage**: High fear + low courage mobs
- cower, whimper, cringe, flinch, gasp, panic, shake, worry, shivers

#### 6. **loving_socials** (13 socials)
**Usage**: High love or high friendship + high trust
- hug, cuddle, kiss, bearhug, blush, caress, embrace, flirt, love, swoon, charm, huggle, ghug

#### 7. **proud_socials** (5 socials)
**Usage**: High pride mobs
- strut, flex, boast, brag, pose

#### 8. **envious_socials** (3 socials)
**Usage**: High envy + target has good equipment/reputation
- envy, eye, greed

#### 9. **playful_socials** (9 socials)
**Usage**: High happiness + moderate excitement
- tickle, poke, tease, bounce, giggle, dance, skip, joke, sing

#### 10. **aggressive_socials** (8 socials)
**Usage**: High anger or alignment-based aggression
- threaten, challenge, growl, snarl, bite, slap, battlecry, warscream

#### 11. **sad_socials** (5 socials)
**Usage**: High sadness
- cry, sob, weep, sulk, sad

#### 12. **confused_socials** (6 socials)
**Usage**: High curiosity + moderate happiness
- boggle, blink, puzzle, wonder, scratch, think

#### 13. **excited_socials** (3 socials)
**Usage**: High happiness + very high excitement
- bounce, whoo, cheers

#### 14. **respectful_socials** (3 socials)
**Usage**: Evil mobs respecting reputable evil, good mobs respecting heroes
- salute, curtsey, kneel

#### New Enhanced Categories (7 additional) - **NEWLY IMPLEMENTED**

#### 15. **grateful_socials** (6 socials) ✨ NEW
**Usage**: High happiness + very high friendship + high trust
**Emotion Trigger**: happiness ≥ 60, friendship ≥ 70, trust ≥ 60
- thanks, bow, applaud, backclap, beam, salute

#### 16. **mocking_socials** (5 socials) ✨ NEW
**Usage**: High anger + high pride but not fully aggressive
**Emotion Trigger**: anger ≥ 60, pride ≥ 60, courage ≥ 50, not fighting
- mock, sneer, snicker, jeer, taunt

#### 17. **submissive_socials** (7 socials) ✨ NEW
**Usage**: Very high fear + very low courage
**Emotion Trigger**: fear ≥ 80, courage ≤ 20
- cower, grovel, bow, kneel, whimper, cringe, flinch

#### 18. **curious_socials** (6 socials) ✨ NEW
**Usage**: Very high curiosity + moderate excitement
**Emotion Trigger**: curiosity ≥ 75, excitement 40-69
- peer, ponder, wonder, sniff, gaze, stare

#### 19. **triumphant_socials** (5 socials) ✨ NEW
**Usage**: Very high pride + high courage after victory
**Emotion Trigger**: pride ≥ 85, courage ≥ 70, excitement ≥ 60
- cheers, flex, roar, battlecry, strut

#### 20. **protective_socials** (4 socials) ✨ NEW
**Usage**: High compassion + loyalty towards injured/weak target
**Emotion Trigger**: compassion ≥ 70, loyalty ≥ 60, target injured or resting
- embrace, pat, comfort, console

#### 21. **mourning_socials** (5 socials) ✨ NEW
**Usage**: High sadness + low excitement (grief/mourning behavior)
**Emotion Trigger**: sadness ≥ 75, excitement < 30
- cry, sob, weep, sulk, despair

---

## 5. Implementation Summary - NEW CATEGORIES ✨

### What Was Added

**7 new emotion-based social categories** have been implemented in `src/mobact.c`:

1. **grateful_socials** - For mobs expressing gratitude after receiving help
2. **mocking_socials** - For taunting/mocking behavior without full aggression  
3. **submissive_socials** - For extreme fear/submission behavior
4. **curious_socials** - For investigative/wondering behavior
5. **triumphant_socials** - For victory celebrations
6. **protective_socials** - For guarding/defending allies
7. **mourning_socials** - For grief responses

### Emotion-Based Selection Logic

The social selection now uses a sophisticated priority system based on emotion thresholds:

**Highest Priority (Extreme Emotions)**:
- Submissive (fear ≥ 80, courage ≤ 20)
- Fearful (fear ≥ 70, courage < 40)
- Triumphant (pride ≥ 85, courage ≥ 70, excitement ≥ 60)

**High Priority (Strong Emotions)**:
- Proud (pride ≥ 75)
- Protective (compassion ≥ 70, loyalty ≥ 60, target injured)
- Mourning (sadness ≥ 75, excitement < 30)
- Grateful (happiness ≥ 60, friendship ≥ 70, trust ≥ 60)
- Mocking (anger ≥ 60, pride ≥ 60, courage ≥ 50)
- Curious (curiosity ≥ 75, excitement 40-69)

**Medium Priority**: Envy, Love, Aggression, Sadness, Excitement, Playful

**Lower Priority**: Positive, Negative, Neutral, Confused, Respectful, Resting

### Total Social Coverage

- **21 emotion categories** (14 original + 7 new)
- **~120 unique socials** used by mobs (some socials appear in multiple categories)
- **Emotion-driven selection** ensures appropriate social responses based on mob's emotional state

---

## 6. Suggested Additional Emotion Categories

### Status: COMPLETED ✅

All 7 suggested categories from the original analysis have been implemented:

1. ✅ **grateful_socials** - Implemented
2. ✅ **mocking_socials** - Implemented
3. ✅ **submissive_socials** - Implemented (renamed from submissive)
4. ✅ **curious_socials** - Implemented
5. ✅ **triumphant_socials** - Implemented
6. ✅ **protective_socials** - Implemented
7. ✅ **mourning_socials** - Implemented

---

## 7. Technical Implementation Details

### Code Changes Made

#### File: `src/mobact.c`

**Added 7 new social arrays** (lines ~194-200):
```c
const char *grateful_socials[] = {"thanks", "bow", "applaud", "backclap", "beam", "salute", NULL};
const char *mocking_socials[] = {"mock", "sneer", "snicker", "jeer", "taunt", NULL};
const char *submissive_socials[] = {"cower", "grovel", "bow", "kneel", "whimper", "cringe", "flinch", NULL};
const char *curious_socials[] = {"peer", "ponder", "wonder", "sniff", "gaze", "stare", NULL};
const char *triumphant_socials[] = {"cheers", "flex", "roar", "battlecry", "strut", NULL};
const char *protective_socials[] = {"embrace", "pat", "comfort", "console", NULL};
const char *mourning_socials[] = {"cry", "sob", "weep", "sulk", "despair", NULL};
```

**Enhanced emotion-based selection logic** with priority-ordered checks in `mob_contextual_social()`:
- Submissive behavior for extreme fear (fear ≥ 80, courage ≤ 20)
- Triumphant behavior for victory/pride (pride ≥ 85, courage ≥ 70, excitement ≥ 60)
- Protective behavior for compassionate mobs with injured allies
- Mourning behavior for grief (sadness ≥ 75, excitement < 30)
- Grateful behavior for friendly mobs (happiness ≥ 60, friendship ≥ 70, trust ≥ 60)
- Mocking behavior for angry but not aggressive mobs (anger ≥ 60, pride ≥ 60, courage ≥ 50)
- Curious behavior for investigative mobs (curiosity ≥ 75, excitement 40-69)

### Future Enhancements (Not Yet Implemented)

#### A. Add Inappropriate Social Filter
```c
// In mob_contextual_social(), add exclusion check
static const char *inappropriate_socials[] = {
    "seduce", "sex", "grope", "fondle", "french", "despine", 
    "spank", "whip", /* ... etc ... */
    NULL
};

// Check before executing social
for (int i = 0; inappropriate_socials[i] != NULL; i++) {
    if (!strcmp(social_list[social_index], inappropriate_socials[i]))
        return;  // Skip inappropriate social
}
```

#### B. Add Silly Social Personality Check
```c
// Only allow silly socials for mobs with MOB_COMEDIC flag
static const char *silly_socials[] = {
    "burp", "fart", "belch", "puke", "booger", /* ... etc ... */
    NULL
};

bool is_silly = false;
for (int i = 0; silly_socials[i] != NULL; i++) {
    if (!strcmp(social_list[social_index], silly_socials[i])) {
        is_silly = true;
        break;
    }
}

if (is_silly && !MOB_FLAGGED(ch, MOB_COMEDIC))
    return;  // Skip silly social for serious mobs
```

---

## 8. Testing the New Categories

#### C. Expand Emotion Categories
Add new social arrays in `mob_contextual_social()`:
```c
const char *grateful_socials[] = {"thanks", "bow", "applaud", "backclap", "beam", "salute", NULL};
const char *mocking_socials[] = {"mock", "sneer", "snicker", "jeer", "taunt", NULL};
const char *submissive_socials[] = {"cower", "grovel", "bow", "kneel", "whimper", "cringe", "flinch", NULL};
```

And add selection logic:
```c
// High gratitude after receiving help
else if (mob_gratitude >= 70 && recently_helped) {
    social_list = grateful_socials;
}
// High anger with high pride - mock/taunt
else if (mob_anger >= 60 && mob_pride >= 50 && !aggressive) {
    social_list = mocking_socials;
}
// High fear with low courage - submit
else if (mob_fear >= 70 && mob_courage < 30) {
    social_list = submissive_socials;
}
```

---

## 8. Testing Recommendations

### Test Cases for Mob-to-Mob Socials

1. **Basic Interaction Test**
   - Place two AI-enabled mobs in a room
   - Enable CONFIG_MOB_CONTEXTUAL_SOCIALS
   - Observe if mobs perform socials toward each other
   - Expected: Mobs should use contextual socials based on emotions

2. **Emotion-Based Test**
   - Create mobs with different emotion profiles (angry, happy, fearful)
   - Verify correct social categories are used
   - Expected: Angry mobs use negative/aggressive socials, happy mobs use positive

3. **Alignment Test**
   - Place good and evil mobs together
   - Observe social selection based on alignment conflicts
   - Expected: Good/evil mobs show negative socials to opposite alignment

4. **Inappropriate Social Test**
   - Enable debugging to log social selection
   - Verify inappropriate socials are never selected by mobs
   - Expected: No sexual or violent socials used automatically

### Console Commands for Testing
```
cedit
[navigate to experimental features]
Set mob_contextual_socials = 1
Set mob_emotion_social_chance = 50  (50% chance per tick)
```

---

## 9. Configuration Recommendations

### Suggested Default Configuration

```
# In lib/etc/config file (or cedit menu):

# Enable mob contextual socials
mob_contextual_socials = 1

# Social frequency (default: 10% per emotion tick)
mob_emotion_social_chance = 10

# Emotion update frequency
mob_emotion_update_chance = 20

# Enable emotion-based responses
mob_emotion_responses = 1
```

### Performance Considerations

- **Current frequency**: PULSE_MOB_EMOTION (4 seconds) × 10% chance = ~1 social per 40 seconds per mob
- **Recommended**: Keep chance at 5-15% to avoid spam
- **High-traffic areas**: Consider room flags to disable socials in busy/important areas

---

## 10. Conclusion

### Summary of Findings

✅ **Mob-to-mob socials**: Fully functional, no code changes needed
⚠️ **Inappropriate socials**: 34 identified, need filtering/restrictions
⚠️ **Silly socials**: 12 identified, should be limited to specific mob types
✅ **Emotion categories**: 14 well-implemented categories with 102 socials
💡 **Potential expansions**: 7 additional categories suggested

### Recommended Actions (Priority Order)

1. **CRITICAL**: Implement inappropriate social filter (prevent automatic use)
2. **HIGH**: Add silly social personality check (MOB_COMEDIC flag or similar)
3. **MEDIUM**: Consider adding 3-5 new emotion categories (grateful, mocking, submissive)
4. **LOW**: Add mob_usable flag to social_messg structure for fine-grained control
5. **DOCUMENTATION**: Update player documentation explaining mob social behavior

### Implementation Impact

- **No changes needed for mob-to-mob capability** - already works
- **Safety improvements** - filtering inappropriate content
- **Enhanced immersion** - preventing silly socials on serious mobs  
- **Expanded behavior** - new emotion categories for richer interactions

---

## Appendix A: Complete Social Lists

### All 102 Mob Socials Currently in Use
```
accuse, agree, applaud, backclap, battlecry, beam, bearhug, bite, blink, 
blush, boast, boggle, bounce, bow, brag, calm, caress, challenge, charm, 
cheers, clap, comfort, console, cower, cradle, cringe, cry, cuddle, curse, 
curtsey, dance, embrace, envy, eye, flex, flinch, flirt, frown, gasp, ghug, 
giggle, glare, greed, greet, grin, growl, happy, hate, hug, huggle, joke, 
kiss, kneel, love, mock, nods, panic, pat, peer, poke, ponder, pose, puzzle, 
sad, salute, scratch, shake, shivers, shrugs, sing, skip, slap, smile, snarl, 
sneer, sob, spit, steam, stretch, strut, sulk, swear, sweetsmile, swoon, 
tease, thanks, think, threaten, thumbsup, tickle, tuck, wait, warscream, 
waves, weep, welcome, whimper, whoo, winks, wonder, worry, yawn
```

### All 34 Inappropriate Socials
```
boo, custard, despine, dive, earlick, flick, fondle, french, grope, haircut, 
halo, hula, joint, knuckle, licks, massage, moan, modest, needle, newidea, 
pant, pushup, rose, seduce, sex, shiskabob, shock, spank, spork, suckit-up, 
titter, vampire, vice, whip
```

### All 12 Silly/Nonsense Socials
```
abc, belch, booger, burp, drool, fart, howl, moon, noogie, puke, raspberry, wedgie
```

---

**Document Version**: 1.0  
**Date**: 2025-10-30  
**Author**: AI Analysis System  
**Status**: Complete - Ready for Review
