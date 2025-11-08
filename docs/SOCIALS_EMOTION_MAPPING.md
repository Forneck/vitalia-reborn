# Socials and Emotion System Mapping

This document describes how player socials affect NPC emotions in the Vitalia Reborn MUD emotion system.

## Overview

When players perform socials on NPCs with AI enabled (`CONFIG_MOB_CONTEXTUAL_SOCIALS`), the NPC's emotions are updated based on the type of social and the existing relationship context. The function `update_mob_emotion_from_social()` in `src/utils.c` handles this mapping.

## Social Categories and Emotion Effects

### 1. Positive Socials (31 socials)
**Emotion Changes:**
- ✅ +happiness (5-15)
- ✅ +friendship (5-12)
- ✅ +trust (3-10)
- ❌ -anger (3-8)
- ❌ -fear (2-6)

**Special Bonuses:**
- High reputation actors (≥60): Extra +friendship, +trust
- Very high reputation (≥80): +love (3-8)
- Compassionate mobs (≥60): Extra +happiness
- Comforting actions (comfort, pat, hug): -pain, -sadness
- Same alignment bonus: Extra +friendship, +loyalty

**Socials:** bow, smile, nod, wave, applaud, clap, greet, wink, grin, comfort, pat, hug, cuddle, kiss, nuzzle, squeeze, stroke, snuggle, thank, worship, giggle, chuckle, laugh, cackle, bounce, dance, sing, tango, whistle, yodel, curtsey

---

### 2. Negative Socials (13 socials)
**Emotion Changes:**
- ✅ +anger (8-20)
- ❌ -trust (10-20)
- ❌ -friendship (8-15)
- ❌ -happiness (5-15)

**Special Modifiers:**
- Wimpy mobs (>50): Extra +fear
- Brave mobs (>60): Extra +pride, +courage
- Low reputation actors (<30): Extra +anger
- Taunt: +envy (5-15)
- Opposite alignment: Extra +anger, -trust

**Socials:** frown, glare, spit, accuse, curse, taunt, snicker, slap, punch, snap, snarl, growl, fume

---

### 3. Neutral/Curious Socials (12 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +friendship (2-5) if already friendly (≥50)

**Note:** "look" and "examine" are NOT socials - they are commands from interpreter.c and have been removed from this list.

**Socials:** ponder, shrug, peer, think, stare, point, comb, sneeze, cough, hiccup, yawn, snore

---

### 4. Fearful Socials (7 socials)
**Emotion Changes (for the NPC receiving):**
- ✅ +courage (5-15)
- ✅ +pride (5-12)
- ❌ -fear (5-12) - NPC's own fear decreases
- ✅ +happiness (5-15) if NPC is evil and social is "beg" (sadistic pleasure)

**Effect:** Actor showing fear/submission makes the NPC feel emboldened.

**Socials:** beg, grovel, cringe, cry, sulk, sigh, whine

---

### 5. Disgusting Socials (7 socials)
**Emotion Changes:**
- ✅ +disgust (15-30)
- ✅ +anger (10-25)
- ❌ -trust (15-30)
- ❌ -friendship (10-20)
- ❌ -happiness (10-20)

**NPC Response:** May show visible disgust (40% chance at ≥50 disgust)

**Socials:** drool, puke, burp, fart, lick, moan, sniff

---

### 6. Violent Socials (3 socials)
**Emotion Changes:**
- ✅ +pain (20-40)
- ✅ +anger (15-35)
- ✅ +fear (10-25)
- ❌ -trust (20-35)
- ❌ -friendship (15-30)

**Special Modifiers:**
- Wimpy mobs (>50): Extra +fear
- Brave mobs (>60): Extra +anger, +courage

**NPC Response:** May respond aggressively (30% chance at ≥60 anger and ≥40 courage)

**Socials:** spank, tackle, snowball

---

### 7. Humiliating Socials (3 socials)
**Emotion Changes:**
- ✅ +humiliation (15-30)
- ✅ +shame (15-25)
- ✅ +anger (10-20)
- ❌ -trust (20-35)
- ❌ -friendship (15-25)
- ❌ -pride (10-20)

**Special Modifiers:**
- High pride mobs (≥60): Extra +anger (15-25)

**Socials:** poke, tickle, ruffle

---

### 8. Playful Socials (8 socials)
**Emotion Changes (context-dependent):**

**If friends (friendship ≥50):**
- ✅ +happiness (5-12)
- ✅ +friendship (2-6)

**If acquaintances (trust ≥30):**
- ✅ +curiosity (3-8)
- ✅ +happiness (2-5)

**If strangers:**
- ✅ +anger (2-8)
- ✅ +curiosity (2-5)

**Socials:** pout, smirk, wiggle, twiddle, flip, strut, nudge, purr

---

### 9. Romantic Socials (2 socials)
**Emotion Changes (highly context-dependent):**

**High relationship (friendship ≥60 OR love ≥40):**
- ✅ +love (5-15)
- ✅ +happiness (5-12)
- ✅ +friendship (3-8)

**Moderate relationship (friendship ≥30 AND trust ≥30):**
- ✅ +curiosity (5-12)
- ✅ +love (2-6)

**Low/no relationship:**
- ✅ +curiosity (3-8)
- ✅ +shame (5-12)
- ❌ -trust (5-10)

**Socials:** flirt, love

---

### 10. Severely Inappropriate Socials (3 socials)
**Context-dependent responses based on relationship:**

**Very high intimacy (love ≥80, trust ≥70, friendship ≥70):**
- ✅ +love (10-20)
- ✅ +happiness (10-20)
- ✅ +trust (5-10)
- **Response:** Affectionate (50% chance)

**High intimacy (love ≥60, trust ≥50, friendship ≥60):**
- ✅ +love (5-15)
- ✅ +curiosity (10-20)
- ✅ +shame (5-15)
- ❌ -trust (5-15)
- **Response:** Uncomfortable but not hostile (40% chance)

**Moderate (trust ≥30 OR friendship ≥40):**
- ✅ +disgust (20-40)
- ✅ +shame (15-30)
- ✅ +anger (15-30)
- ❌ -trust (25-45)
- ❌ -friendship (20-35)
- **Response:** Pushes away with disgust (50% chance)

**Low/no relationship:**
- ✅ +disgust (40-60)
- ✅ +horror (20-40)
- ✅ +anger (30-50)
- ✅ +shame (20-35)
- ✅ +humiliation (15-30)
- ❌ -trust (40-60)
- ❌ -friendship (35-55)
- **Response:** May attack (50% chance if courage ≥50 and anger ≥60) or flee

**Socials:** fondle, grope, french

---

### 11. Special Context-Dependent Social: massage
**Effect depends on trust level:**

**Trusted (trust ≥50 AND friendship ≥40):**
- ✅ +happiness (10-20)
- ✅ +trust (5-10)

**Untrusted:**
- ✅ +disgust (10-20)
- ✅ +anger (5-15)
- ❌ -trust (10-20)

---

## Remaining Unmapped Socials

The following socials exist in the game but currently have no emotion system mapping. They will not trigger emotion changes in NPCs:

- bleed, blush, brb, daydream, gasp, groan, pray, scream, shake, shiver, nibble

These are mostly self-directed or neutral actions that don't significantly impact social interactions.

---

## Invalid Entries Removed

The following entries were **removed** from the emotion system because they either don't exist as socials or are commands (not socials):

### Commands (not socials):
- look (command in interpreter.c)
- examine (command in interpreter.c)

### Non-existent socials:
- watch, observe, salute, cheer, praise, respect, welcome, handshake, highfive
- scorn, mock, insult, scoff, dismiss, threaten, kick, shove, eye, jeer, ridicule, scowl
- cower, tremble, whimper, plead
- sex, seduce, despine, shiskabob, vice
- earlick, licks, pant, moon, booger
- needle, shock, whip, vampire, haircut
- suckit-up, wedgie, noogie
- rose, knuckle

---

## Summary Statistics

- **Total socials in game:** 101
- **Socials with emotion mappings:** 86
- **Unmapped socials:** 11 (mostly self-directed/neutral)
- **Invalid entries removed:** 50

---

## Implementation Notes

1. All emotion changes use `adjust_emotion()` which automatically clamps values to valid ranges (typically 0-100)
2. Random ranges are used to create variability in responses (e.g., `rand_number(5, 15)`)
3. Context factors include:
   - Existing relationship levels (trust, friendship, love)
   - Mob personality traits (wimpy_tendency, brave_prevalence)
   - Player reputation
   - Mob alignment vs player alignment
   - Compassion levels
4. Some socials trigger mob responses/actions (e.g., visible reactions, combat initiation)
5. The `CONFIG_MOB_CONTEXTUAL_SOCIALS` config option must be enabled for any of this to work

---

## Code Location

**File:** `src/utils.c`  
**Function:** `update_mob_emotion_from_social()`  
**Line range:** ~5239-5700+

---

*Last updated: 2025-11-08*
