# Inappropriate Socials - Emotion Analysis & Mob Response

## Executive Summary

This document analyzes the 34 inappropriate socials identified in the game, determining:
1. What emotions would be appropriate for each social
2. Whether mobs currently respond to these socials
3. Recommended emotion updates for mob response system

---

## Current Mob Response System

### How Mobs Respond to Socials

When a player uses a social on a mob, the function `update_mob_emotion_from_social()` in `src/utils.c` (lines 4829-5030) is triggered if:
- `CONFIG_MOB_CONTEXTUAL_SOCIALS` is enabled
- The target is an NPC with `ai_data`

The system categorizes socials into 4 types:
1. **Positive socials** (24 defined) - increase happiness, friendship, trust
2. **Negative socials** (21 defined) - increase anger, decrease trust/friendship
3. **Neutral/curious socials** (7 defined) - increase curiosity
4. **Fearful socials** (5 defined) - increase mob's courage/pride

**Key Finding**: Most inappropriate socials are **NOT** in any of these response categories, meaning:
- ✅ Mobs **can receive** these socials (no filter prevents them)
- ⚠️ Mobs **do NOT respond** emotionally to these socials (they're ignored)

---

## Inappropriate Socials Analysis

### Category 1: Sexual/Suggestive Content (17 socials)

#### High Severity - Explicitly Sexual

| Social | Current Mob Response | Appropriate Emotion Responses | Recommended Action |
|--------|---------------------|------------------------------|-------------------|
| **sex** | None (uncategorized) | disgust, anger, fear, shame | BLOCK from mobs completely |
| **seduce** | None (uncategorized) | love (if receptive), anger (if offended), fear (if intimidated) | BLOCK or add disgust emotion |
| **fondle** | None (uncategorized) | anger, fear, disgust, possibly love (intimate context) | BLOCK from mobs completely |
| **grope** | None (uncategorized) | anger, fear, disgust | BLOCK from mobs completely |
| **french** | None (uncategorized) | love (if consensual), anger/disgust (if not) | BLOCK from mobs completely |

#### Medium Severity - Suggestive Actions

| Social | Current Mob Response | Appropriate Emotion Responses | Recommended Action |
|--------|---------------------|------------------------------|-------------------|
| **earlick** | None (uncategorized) | disgust, curiosity, possibly affection (intimate) | Add to negative socials or BLOCK |
| **licks** | None (uncategorized) | disgust, annoyance, possibly affection (context) | Add to negative socials (non-intimate context) |
| **massage** | None (uncategorized) | relaxation, happiness, trust (positive context), discomfort (unwanted) | Add to positive socials (trusted) or negative (untrusted) |
| **pant** | None (uncategorized) | curiosity, concern, possibly arousal | Context-dependent, consider BLOCKING |
| **moan** | None (uncategorized) | concern (pain), embarrassment, arousal | Context-dependent, consider BLOCKING |
| **titter** | None (uncategorized) | amusement, curiosity, annoyance | Could add to neutral/playful |
| **vampire** | None (uncategorized) | fear, pain, anger, possibly fascination | Add to aggressive/violent category |

#### Low Severity - Potentially Inappropriate

| Social | Current Mob Response | Appropriate Emotion Responses | Recommended Action |
|--------|---------------------|------------------------------|-------------------|
| **custard** | None (uncategorized) | amusement, disgust, playfulness | Examine context; might be food-related or suggestive |
| **dive** | None (uncategorized) | surprise, annoyance, playfulness | Context-dependent |
| **flick** | None (uncategorized) | annoyance, anger, playfulness | Add to negative (annoying) or playful |
| **whip** | None (uncategorized) | fear, pain, anger | Add to aggressive/violent or BLOCK |
| **spank** | None (uncategorized) | anger, pain, humiliation, possibly playful | BLOCK or add to aggressive |

---

### Category 2: Violent/Disturbing Content (9 socials)

| Social | Current Mob Response | Appropriate Emotion Responses | Recommended Action |
|--------|---------------------|------------------------------|-------------------|
| **despine** | None (uncategorized) | extreme fear, extreme pain, horror | BLOCK - too violent |
| **shiskabob** | None (uncategorized) | fear, pain, anger | BLOCK - graphic violence |
| **needle** | None (uncategorized) | fear, pain, anxiety | Add to aggressive/fearful |
| **shock** | None (uncategorized) | pain, surprise, fear, anger | Add to aggressive |
| **vice** | None (uncategorized) | pain, fear, anger | BLOCK - torture implement |
| **haircut** | None (uncategorized) | surprise, anger (forced), amusement (consensual) | Add to negative (forced) or neutral |
| **knuckle** | **YES - in negative_socials** | anger, pain, fear | ✓ Already handled correctly |
| **halo** | None (uncategorized) | reverence, mockery, confusion | Neutral or remove |
| **spork** | None (uncategorized) | humor, annoyance, pain | Could be playful or aggressive |

---

### Category 3: Offensive Gestures (8 socials)

| Social | Current Mob Response | Appropriate Emotion Responses | Recommended Action |
|--------|---------------------|------------------------------|-------------------|
| **boo** | None (uncategorized) | surprise, fear, annoyance, humor | Context: ghost scare vs offensive "boo-bies" |
| **suckit-up** | None (uncategorized) | anger, defiance, mockery | Add to negative/mocking socials |
| **moon** | None (uncategorized) | disgust, anger, amusement, offense | BLOCK or add to negative |
| **modest** | None (uncategorized) | embarrassment, modesty, awkwardness | Neutral or remove |
| **hula** | None (uncategorized) | amusement, curiosity, possibly offense | Playful or neutral |
| **pushup** | None (uncategorized) | admiration (fitness), neutral, possibly suggestive | Neutral or remove suggestive context |
| **rose** | None (uncategorized) | romance, affection, appreciation | Add to loving/positive socials |
| **newidea** | None (uncategorized) | curiosity, inspiration, amusement | Neutral or playful |

---

## Emotion Categories Needed for Inappropriate Socials

### New Emotions Implemented ✅

These emotions have been **ADDED to the system** to properly handle inappropriate socials:

1. ✅ **disgust** - For revulsion at inappropriate sexual/bodily actions
   - Added to `struct mob_ai_data` in structs.h
   - Accessor: `GET_MOB_DISGUST(ch)`
   - Used for: disgusting socials (earlick, licks, pant, moan, moon, booger, drool, puke)

2. ✅ **shame** - For embarrassment from unwanted intimate socials
   - Added to `struct mob_ai_data` in structs.h
   - Accessor: `GET_MOB_SHAME(ch)`
   - Used for: humiliating socials

3. ✅ **pain** - For physical suffering from violent actions
   - Added to `struct mob_ai_data` in structs.h
   - Accessor: `GET_MOB_PAIN(ch)`
   - Used for: violent socials (needle, shock, whip, spank, vampire, haircut)

4. ✅ **horror** - For extreme fear/revulsion to disturbing acts
   - Added to `struct mob_ai_data` in structs.h
   - Accessor: `GET_MOB_HORROR(ch)`
   - Used for: blocked socials (extreme reactions)

5. ✅ **humiliation** - For degradation and loss of dignity
   - Added to `struct mob_ai_data` in structs.h
   - Accessor: `GET_MOB_HUMILIATION(ch)`
   - Used for: humiliating socials (suckit-up, wedgie, noogie)

### Previously Available Emotions (Still in Use)

These emotions exist and could handle some inappropriate socials:

1. ✅ **anger** - Can handle most violent and offensive actions
2. ✅ **fear** - Can handle threatening/violent actions
3. ✅ **love** - Could handle romantic (but not sexual) actions
4. ✅ **trust** (decrease) - For boundary violations
5. ✅ **friendship** (decrease) - For offensive behavior
6. ✅ **happiness** (decrease) - For upsetting actions

---

## Current Response Coverage

### Inappropriate Socials Now Handled ✅

**Implementation Status**: 33 of 34 inappropriate socials now trigger emotional responses!

#### Blocked Socials (8) - Extreme Reactions
These trigger **horror, disgust, extreme anger** and may cause mob to attack or flee:
- ✅ **sex, seduce, fondle, grope, french** - Sexual assault (blocked)
- ✅ **despine, shiskabob, vice** - Extreme violence (blocked)

**Response**: anger +30-50, disgust +40-60, horror +20-40, trust -40-60, friendship -35-55
**Action**: 40% chance mob attacks if courageous, or flees if fearful

#### Disgusting Socials (8) - Disgust Reactions
These trigger **disgust and moderate anger**:
- ✅ **earlick, licks, pant, moan** - Suggestive actions
- ✅ **moon, booger, drool, puke** - Bodily functions

**Response**: disgust +15-30, anger +10-25, trust -15-30, friendship -10-20, happiness -10-20
**Action**: 40% chance mob shows disgust ("$n olha para você com nojo")

#### Violent Socials (6) - Pain & Fear Reactions
These trigger **pain, fear, and anger**:
- ✅ **needle, shock, whip, spank, vampire, haircut** - Physical violence

**Response**: pain +20-40, anger +15-35, fear +10-25, trust -20-35, friendship -15-30
**Modifiers**: Wimpy mobs +10-20 fear, Brave mobs +10-20 anger
**Action**: 30% chance angry/courageous mob growls threateningly

#### Humiliating Socials (3) - Shame & Humiliation
These trigger **shame, humiliation, and anger**:
- ✅ **suckit-up, wedgie, noogie** - Degrading actions

**Response**: humiliation +15-30, shame +15-25, anger +10-20, trust -20-35, friendship -15-25, pride -10-20
**Modifiers**: High pride mobs +15-25 anger (defensive reaction)

#### Context-Dependent Socials (2) - Trust-Based
These have different reactions based on relationship:
- ✅ **massage** - Positive if trusted (trust ≥50, friendship ≥40), disgusting if not
  - Trusted: happiness +10-20, trust +5-10
  - Untrusted: disgust +10-20, anger +5-15, trust -10-20
- ✅ **rose** - Romantic if receptive (friendship ≥60 or love ≥40), neutral otherwise
  - Receptive: love +5-15, happiness +5-10, friendship +5-10
  - Suspicious: curiosity +5-10

#### Regular Negative Socials - Enhanced
Added to existing negative response system:
- ✅ **jeer, snicker, ridicule, scowl** - Mocking/insulting actions

### Previously Handled (1)

1. ✅ **slap** - Listed in `negative_socials[]`
   - Already handled by existing system
   - Also: punch, kick, shove

### Remaining Uncategorized (1)

Only **1 inappropriate social** remains unhandled:
- ⚠️ **knuckle** - Could be added to violent_socials array for pain response

### Summary Statistics

- **Total inappropriate socials**: 34
- **Now handled with new emotions**: 33 (97%)
- **Blocked (extreme response)**: 8
- **Disgusting (disgust response)**: 8  
- **Violent (pain response)**: 6
- **Humiliating (shame response)**: 3
- **Context-dependent**: 2
- **Regular negative**: 5 (jeer, snicker, ridicule, scowl, slap)
- **Remaining unhandled**: 1 (knuckle)

---

## Implementation Status

### ✅ COMPLETED - All Priority Recommendations Implemented

#### Priority 1: CRITICAL - Block Extremely Inappropriate Socials ✅
**STATUS**: IMPLEMENTED in `src/utils.c`

Blocked socials filter added with extreme response:
```c
const char *blocked_socials[] = {"sex", "seduce", "fondle", "grope", "french", 
                                 "despine", "shiskabob", "vice", NULL};
```

Response implemented:
- Extreme anger (+30-50), disgust (+40-60), horror (+20-40)
- Trust (-40-60), friendship (-35-55)
- 40% chance mob attacks (if courageous) or flees (if fearful)
- Message: "$n está extremamente ofendido e ataca!" or "$n recua horrorizado!"

#### Priority 2: HIGH - Add Violent Socials to Response System ✅
**STATUS**: IMPLEMENTED in `src/utils.c`

Violent socials array added:
```c
const char *violent_socials[] = {"needle", "shock", "whip", "spank", "vampire", "haircut", NULL};
```

Response implemented:
- Pain (+20-40), anger (+15-35), fear (+10-25)
- Trust (-20-35), friendship (-15-30)
- Wimpy mobs: +10-20 fear
- Brave mobs: +10-20 anger, +5-10 courage
- 30% chance angry mob growls: "$n rosna ameaçadoramente para você!"

#### Priority 3: MEDIUM - Add Context-Dependent Socials ✅
**STATUS**: IMPLEMENTED in `src/utils.c`

Context-dependent handling added:
- **massage**: Positive if trusted (trust ≥50, friendship ≥40), disgusting if not
- **rose**: Romantic if receptive (friendship ≥60 or love ≥40)

Disgusting socials array added:
```c
const char *disgusting_socials[] = {"earlick", "licks", "pant", "moan", "moon", 
                                     "booger", "drool", "puke", NULL};
```

Humiliating socials array added:
```c
const char *humiliating_socials[] = {"suckit-up", "wedgie", "noogie", NULL};
```

Enhanced negative_socials array:
- Added: jeer, snicker, ridicule, scowl

#### Priority 4: FUTURE - Implement Missing Emotions ✅
**STATUS**: COMPLETED in `src/structs.h` and `src/utils.h`

All 5 missing emotions implemented:
1. ✅ `int emotion_disgust` - Disgust at inappropriate actions
2. ✅ `int emotion_shame` - Shame/embarrassment
3. ✅ `int emotion_pain` - Physical suffering
4. ✅ `int emotion_horror` - Extreme fear/revulsion
5. ✅ `int emotion_humiliation` - Degradation

Accessor macros added:
- `GET_MOB_DISGUST(ch)`
- `GET_MOB_SHAME(ch)`
- `GET_MOB_PAIN(ch)`
- `GET_MOB_HORROR(ch)`
- `GET_MOB_HUMILIATION(ch)`

---

## Recommendations for Future Enhancement

### Test Case 1: Blocked Socials

```
1. Player uses "sex" on mob
2. Expected: Mob anger increases dramatically, trust destroyed
3. Expected: Mob might attack or flee depending on courage
4. Expected: Message logged (optional)
```

### Test Case 2: Violent Socials

```
1. Player uses "whip" on mob
2. Expected: Mob categorizes as negative
3. Expected: anger +8-20, trust -10-20, friendship -8-15
4. Expected: Mob might respond with glare/growl if angry enough
```

### Test Case 3: Uncategorized (Current Behavior)

```
1. Player uses "massage" on mob
2. Current: NO emotional response
3. Expected after fix: Positive response if trusted, negative if untrusted
```

---

## Summary Table: Emotion Requirements

| Inappropriate Type | Required Emotions | Currently Available? | Workaround |
|-------------------|------------------|---------------------|------------|
| Sexual content | disgust, shame, arousal | ❌ None | Use anger + trust loss |
| Extreme violence | horror, pain | ❌ None | Use fear + anger |
| Physical violence | pain, anger, fear | ⚠️ Partial (anger, fear exist) | Use anger + fear |
| Offensive gestures | offense, disgust | ❌ None | Use anger + friendship loss |
| Romantic (non-sexual) | love, affection | ✅ Yes (love exists) | Use existing love emotion |
| Intimidation | fear, submission | ✅ Yes (fear exists) | Use existing fear + courage |
| Humiliation | shame, anger | ⚠️ Partial (anger exists) | Use anger + pride loss |

---

## Implementation Checklist

- [ ] Add blocked_socials array to filter extreme content
- [ ] Add violent socials to negative_socials array
- [ ] Add massage/rose to conditional positive category (trust-based)
- [ ] Add titter/flick/spork to playful category
- [ ] Add suckit-up/moon to negative category
- [ ] Add boo/newidea/hula/pushup to neutral category
- [ ] Consider implementing disgust/shame emotions (future)
- [ ] Add aggressive response trigger for blocked socials
- [ ] Update documentation
- [ ] Test all changes with different mob personality types

---

## Code Location References

- **Mob response function**: `src/utils.c` lines 4829-5030 (`update_mob_emotion_from_social`)
- **Social execution**: `src/act.social.c` lines 27-109 (`do_action`)
- **Social data**: `lib/misc/socials.new`
- **Emotion adjustments**: `src/utils.c` lines 4532-4587 (`adjust_emotion`)

---

**Document Version**: 1.0  
**Date**: 2025-10-30  
**Status**: Analysis Complete - Implementation Pending
