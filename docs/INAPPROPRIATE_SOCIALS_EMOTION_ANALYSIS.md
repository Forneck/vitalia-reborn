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

### Currently Missing Emotions (Not in System)

These emotions would be needed to properly handle inappropriate socials but are **NOT currently implemented**:

1. **disgust** - For revulsion at inappropriate sexual/bodily actions
2. **shame/embarrassment** - For receiving unwanted intimate socials
3. **pain** - For violent physical socials
4. **horror** - For extremely violent/disturbing actions
5. **arousal** (if implementing mature content) - For sexual socials
6. **humiliation** - For degrading actions
7. **offense** - For insulting gestures

### Currently Available Emotions (In System)

These emotions exist and could handle some inappropriate socials:

1. ✅ **anger** - Can handle most violent and offensive actions
2. ✅ **fear** - Can handle threatening/violent actions
3. ✅ **love** - Could handle romantic (but not sexual) actions
4. ✅ **trust** (decrease) - For boundary violations
5. ✅ **friendship** (decrease) - For offensive behavior
6. ✅ **happiness** (decrease) - For upsetting actions

---

## Current Response Coverage

### Inappropriate Socials in Current Response System

Out of 34 inappropriate socials, only **1 is currently handled**:

1. ✅ **slap** - Listed in `negative_socials[]` (utils.c line 4840)
   - Triggers: anger increase, trust decrease, friendship decrease
   - **Note**: `punch`, `kick`, and `shove` (also violent) are in the system too

### Socials Related to Violence Already Handled

These violent actions ARE in the negative_socials response system:
- ✅ **slap** (line 4840)
- ✅ **punch** (line 4840)  
- ✅ **kick** (line 4840)
- ✅ **shove** (line 4840)

### Uncategorized Inappropriate Socials: 33

These socials have **NO emotional response** from mobs:
- sex, seduce, fondle, grope, french, earlick, licks, massage, pant, moan, titter, vampire
- custard, dive, flick, whip, spank
- despine, shiskabob, needle, shock, vice, haircut, halo, spork, knuckle
- boo, suckit-up, moon, modest, hula, pushup, rose, newidea

**Impact**: Players can use these socials on mobs without triggering any emotional response.

---

## Recommendations

### Priority 1: CRITICAL - Block Extremely Inappropriate Socials

Add a filter in `update_mob_emotion_from_social()` to completely reject these:

```c
/* Blocked socials - too inappropriate for mobs */
const char *blocked_socials[] = {
    "sex", "seduce", "fondle", "grope", "french",
    "despine", "shiskabob", "vice",
    NULL
};

/* Check if social is blocked */
for (i = 0; blocked_socials[i] != NULL; i++) {
    if (!strcmp(social_name, blocked_socials[i])) {
        /* Optionally trigger aggressive response for severe violations */
        if (!strcmp(social_name, "sex") || !strcmp(social_name, "grope") || 
            !strcmp(social_name, "fondle")) {
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(20, 40));
            adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(30, 50));
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(25, 45));
            
            /* Mob might attack for serious violations */
            if (mob->ai_data->emotion_anger >= 70 && rand_number(1, 100) <= 30) {
                /* Trigger attack or flee based on courage */
                if (mob->ai_data->emotion_courage >= 50) {
                    /* Attack the offender */
                    set_fighting(mob, actor);
                } else {
                    /* Flee from the situation */
                    mob->ai_data->emotion_fear += rand_number(20, 40);
                }
            }
        }
        return; /* Exit - social blocked */
    }
}
```

### Priority 2: HIGH - Add Violent Socials to Negative Category

Add to `negative_socials[]` array in `update_mob_emotion_from_social()`:

```c
const char *negative_socials[] = {
    /* ... existing socials ... */
    "needle", "shock", "whip", "spank", "vampire",
    "haircut", /* if forced */
    NULL
};
```

### Priority 3: MEDIUM - Add Context-Dependent Socials

Add to appropriate categories:

**To loving/positive_socials** (intimate, consensual):
- "massage" (from trusted source)
- "rose" (romantic gesture)

**To playful_socials**:
- "titter" (giggling)
- "flick" (playful flick)
- "spork" (silly weapon)

**To negative_socials** (offensive):
- "suckit-up" (rude command)
- "moon" (offensive gesture)

**To neutral_socials**:
- "boo" (surprise scare)
- "newidea" (lightbulb moment)
- "hula" (dance)
- "pushup" (exercise)

### Priority 4: FUTURE - Add New Emotions

To properly handle all inappropriate content, consider adding:

```c
/* In struct mob_ai_data */
int emotion_disgust;    /* Disgust at inappropriate actions */
int emotion_shame;      /* Shame/embarrassment */
int emotion_arousal;    /* For mature content (if desired) */
```

---

## Testing Inappropriate Social Responses

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
