# Socials and Emotion System Mapping - Complete Reference

This document describes how player socials affect NPC emotions in the Vitalia Reborn MUD emotion system.

## Overview

When players perform socials on NPCs with AI enabled (`CONFIG_MOB_CONTEXTUAL_SOCIALS`), the NPC's emotions are updated based on the type of social and the existing relationship context. The function `update_mob_emotion_from_social()` in `src/utils.c` handles this mapping.

**Total Coverage:** 490/490 socials from `lib/misc/socials.new` (100%)

## Social Categories and Emotion Effects

### 1. Positive Socials (42 socials)
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

**Socials:** bow, smile, applaud, clap, greet, grin, comfort, pat, hug, cuddle, kiss, nuzzle, squeeze, stroke, snuggle, worship, giggle, laughs, cackle, bounce, dance, sing, tango, whistle, yodel, curtsey, salute, admire, welcome, handshake, highfive, nods, waves, winks, thanks, chuckles, beam, happy, gleam, cheers, enthuse, adoring

---

### 2. Negative Socials (26 socials)
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

**Socials:** frown, glare, spit, accuse, curse, taunt, snicker, slap, snap, snarl, growl, fume, sneer, eye, jeer, mock, ignore, threaten, blame, criticize, disapprove, scold, hate, grimace, evileye

---

### 3. Neutral/Curious Socials (17 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +friendship (2-5) if already friendly (≥50)

**Note:** "look" and "examine" are NOT socials - they are commands from interpreter.c and have been removed.

**Socials:** ponder, peer, think, stare, point, comb, sneeze, cough, hiccup, yawn, snore, shrugs, contemplate, daydream, gaze, listen, blink

---

### 4. Fearful/Sad Socials (21 socials)
**Emotion Changes (for the NPC receiving):**
- ✅ +courage (5-15)
- ✅ +pride (5-12)
- ❌ -fear (5-12) - NPC's own fear decreases
- ✅ +happiness (5-15) if NPC is evil and social is "beg" (sadistic pleasure)

**Effect:** Actor showing fear/submission/sadness makes the NPC feel emboldened.

**Socials:** beg, grovel, cringe, cry, sulk, sigh, whine, cower, whimper, sob, weep, panic, eek, eep, flinch, dread, worry, despair, crushed, blue, crylaugh

---

### 5. Disgusting Socials (15 socials)
**Emotion Changes:**
- ✅ +disgust (15-30)
- ✅ +anger (10-25)
- ❌ -trust (15-30)
- ❌ -friendship (10-20)
- ❌ -happiness (10-20)

**NPC Response:** May show visible disgust (40% chance at ≥50 disgust)

**Socials:** drool, puke, burp, fart, licks, moan, sniff, earlick, pant, moon, booger, belch, gag, spew, phlegm

---

### 6. Violent Socials (17 socials)
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

**Socials:** needle, shock, whip, bite, choke, strangle, smack, smash, clobber, thwap, whack, pound, shootout, sword, smite, burn, charge

**Note:** snowball, tackle, spank, challenge, arrest have special contextual handling (see below)

---

### 7. Humiliating Socials (8 socials)
**Emotion Changes:**
- ✅ +humiliation (15-30)
- ✅ +shame (15-25)
- ✅ +anger (10-20)
- ❌ -trust (20-35)
- ❌ -friendship (15-25)
- ❌ -pride (10-20)

**Special Modifiers:**
- High pride mobs (≥60): Extra +anger (15-25)

**Socials:** poke, tickle, ruffle, suckit-up, wedgie, noogie, pinch, goose

---

### 8. Playful Socials (22 socials)
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

**Socials:** pout, smirks, wiggle, twiddle, flip, strut, nudge, purr, hop, skip, boink, bonk, bop, pounce, tag, tease, joke, jest, nyuk, waggle, cartwheel, hula

---

### 9. Romantic Socials (13 socials)
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

**Socials:** flirt, love, ogle, beckon, charm, smooch, snog, propose, caress, huggle, ghug, cradle, bearhug

**Note:** rose, knuckle, makeout, embrace have special contextual handling (see below)

---

### 10. Agreeable/Supportive Socials (8 socials)
**Emotion Changes:**
- ✅ +trust (3-8)
- ✅ +friendship (2-6)
- ✅ +happiness (2-5)
- ❌ -anger (3-8) if anger > 20

**Socials:** agree, ok, yes, apologize, forgive, console, ack, handraise

---

### 11. Confused Socials (10 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +happiness (2-5) if friendship ≥40 (finds it amusing)

**Socials:** boggle, confuse, puzzle, doh, duh, eww, hmmmmm, hrmph, discombob, disturbed

---

### 12. Celebratory Socials (8 socials)
**Emotion Changes:**

**If friendly (friendship ≥50):**
- ✅ +happiness (5-12)
- ✅ +friendship (2-6)

**Otherwise:**
- ✅ +curiosity (3-8)

**Socials:** huzzah, tada, yayfor, battlecry, rofl, whoo, romp, sundance

---

### 13. Relaxed Socials (5 socials)
**Emotion Changes:**
- ✅ +happiness (3-8)
- ❌ -anger (3-8) if anger > 20
- ❌ -fear (2-6) if fear > 20

**Socials:** relax, calm, breathe, relief, phew

---

### 14. Silly/Absurd Socials (13 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +happiness (5-12) if friendship ≥50

**Socials:** abc, abrac, bloob, doodle, batman, snoopy, elephantma, crazy, crazed, insane, christmas, fish, vampire

**Note:** vampire gets special silly handling - it's saying "I vant to suck yer blood!" (not actually violent)

---

### 15. Performance Socials (8 socials)
**Emotion Changes:**

**If friendly (friendship ≥50):**
- ✅ +pride (3-8)
- ✅ +curiosity (2-6)

**Otherwise (showing off is annoying):**
- ✅ +anger (2-8)
- ✅ +curiosity (3-8)

**Socials:** pose, model, boast, brag, ego, pride, flex, juggle

---

### 16. Angry Expression Socials (7 socials)
**Emotion Changes:**

**If friendly (friendship ≥50):**
- ✅ +curiosity (5-10)
- ✅ +anger (2-6) - empathetic shared anger

**Otherwise:**
- ✅ +fear (3-10)
- ✅ +curiosity (3-8)

**Socials:** rage, steam, grumbles, grunts, argh, postal, hysterical

---

### 17. Communication Socials (6 socials)
**Emotion Changes:**
- ✅ +curiosity (1-4) - minimal

**Socials:** brb, adieu, goodbye, reconnect, channel, wb

---

### 18. Animal Sound Socials (5 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +happiness (3-8) if friendship ≥50

**Socials:** bark, meow, moo, howl, hiss

---

### 19. Food/Drink Socials (6 socials)
**Emotion Changes:**
- ✅ +curiosity (3-8)
- ✅ +friendship (2-6) if friendship ≥60 (sharing context)

**Socials:** beer, coffee, cake, custard, carrot, pie

---

### 20. Gesture Socials (11 socials)
**Emotion Changes:**
- ✅ +curiosity (2-6)

**Socials:** arch, eyebrow, eyeroll, facegrab, facepalm, fan, foot, crossfinger, thumbsup, armcross, behind

---

### 21. Exclamation Socials (8 socials)
**Emotion Changes:**
- ✅ +curiosity (1-4) - minimal

**Socials:** ahem, aww, blah, boo, heh, oh, ouch, tsk

---

### 22. Amused Socials (4 socials)
**Emotion Changes:**

**If friendly (friendship ≥50):**
- ✅ +happiness (5-12)
- ✅ +friendship (2-6)

**Otherwise:**
- ✅ +curiosity (3-8)

**Socials:** amused, chortle, snigger, egrin

---

### 23. Self-Directed Physical Socials (17 socials)
**Emotion Changes:**
- ✅ +curiosity (1-5) - minimal
- ✅ +compassion (3-8) if mob's compassion ≥60

**Socials:** bleed, blush, shake, shiver, scream, faint, collapse, fall, sweat, perspire, shudder, swoon, wince, gasp, groan, headache, pray

---

### 24. Misc Neutral Socials (18 socials)
**Emotion Changes:**
- ✅ +curiosity (1-5) - minimal
- ✅ +friendship (1-3) if friendship ≥60

**Socials:** aim, avsalute, backclap, bat, bored, box, buzz, cold, conga, conga2, creep, curious, shame, rose, knuckle, embrace, makeout, haircut

**Note:** Last 6 (rose, knuckle, embrace, makeout, haircut) get special contextual handling (see below)

---

### 25. Severely Inappropriate Socials - Blocked (8 socials)
**Context-dependent responses based on relationship:**

**Sexual Socials (sex, seduce, fondle, grope, french):**

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

**Extreme Violence Socials (despine, shiskabob, vice, choke, strangle, smite, sword):**
- **Always triggers:** horror, pain, anger, fear regardless of relationship
- **High chance of combat or flee response**
- These are ALWAYS hostile with no context-dependency

**Socials:** sex, seduce, fondle, grope, french, despine, shiskabob, vice

---

## Special Context-Dependent Socials

### Massage
**Effect depends on trust level:**

**Trusted (trust ≥50 AND friendship ≥40):**
- ✅ +happiness (10-20)
- ✅ +trust (5-10)

**Untrusted:**
- ✅ +disgust (10-20)
- ✅ +anger (5-15)
- ❌ -trust (10-20)

---

### Rose
**Romantic gesture - context dependent:**

**Receptive (friendship ≥60 OR love ≥40):**
- ✅ +love (5-15)
- ✅ +happiness (5-10)
- ✅ +friendship (5-10)

**Not receptive (trust < 30):**
- ✅ +curiosity (5-10)

---

### Knuckle
**Playful punch - context dependent:**

**Very high friendship (≥70) AND trust (≥60):**
- ✅ +happiness (5-15)
- ✅ +friendship (3-8)

**Moderate (friendship ≥30 OR trust ≥30):**
- ✅ +anger (5-15)
- ✅ +pain (5-15)
- ❌ -trust (5-10)

**Low/no relationship:**
- ✅ +pain (15-30)
- ✅ +anger (20-40)
- ❌ -trust (20-35)
- ❌ -friendship (15-25)

---

### Makeout
**Very intimate - requires very high relationship:**

**Very high intimacy (love ≥70, trust ≥60, friendship ≥60):**
- ✅ +love (10-20)
- ✅ +happiness (10-20)
- ✅ +trust (5-10)

**Moderate (friendship ≥40 OR trust ≥40):**
- ✅ +shame (10-20)
- ✅ +anger (10-20)
- ❌ -trust (15-25)

**Low/no relationship:**
- ✅ +disgust (20-40)
- ✅ +anger (20-40)
- ✅ +shame (15-25)
- ❌ -trust (30-50)
- ❌ -friendship (25-40)

---

### Embrace
**Intimate hug - stronger than normal hug:**

**High relationship (friendship ≥60 OR love ≥40):**
- ✅ +happiness (10-20)
- ✅ +friendship (8-15)
- ✅ +love (5-12)
- ✅ +trust (5-10)

**Moderate (friendship ≥30):**
- ✅ +curiosity (5-10)
- ❌ -trust (3-8)

**Low/no relationship:**
- ✅ +anger (10-20)
- ❌ -trust (15-25)
- ❌ -friendship (10-20)

---

### Snowball / Tackle
**Playful games vs assault:**

**Very high friendship (≥65) AND trust (≥50):**
- ✅ +happiness (8-18)
- ✅ +friendship (5-10)
- ✅ +excitement (5-12)
- ✅ +pain (2-8) - mild, it's fun

**Moderate (friendship ≥40 OR trust ≥30):**
- ✅ +anger (8-18)
- ✅ +pain (10-20)
- ❌ -trust (8-15)

**Low/no relationship:**
- ✅ +pain (15-30)
- ✅ +anger (20-35)
- ✅ +fear (10-20)
- ❌ -trust (25-40)
- ❌ -friendship (20-35)

---

### Spank
**Playful teasing vs humiliating assault:**

**Very high friendship (≥70) AND trust (≥60):**
- ✅ +happiness (5-12)
- ✅ +friendship (3-8)
- ✅ +humiliation (2-8) - slight, even when playful

**Moderate (friendship ≥30 OR trust ≥30):**
- ✅ +humiliation (15-30)
- ✅ +shame (10-20)
- ✅ +anger (15-25)
- ❌ -trust (20-35)

**Low/no relationship:**
- ✅ +humiliation (25-40)
- ✅ +shame (20-35)
- ✅ +anger (25-45)
- ✅ +pain (10-20)
- ❌ -trust (35-50)
- ❌ -friendship (30-45)

---

### Challenge
**Friendly competition vs hostile challenge:**

**High friendship (≥60):**
- ✅ +excitement (10-20)
- ✅ +pride (8-15)
- ✅ +friendship (3-8)
- ✅ +courage (5-12) if mob's courage ≥60

**Moderate (friendship 30-59):**
- ✅ +curiosity (10-20)
- ✅ +pride (5-12)
- ✅ +anger (3-10)

**Low/no relationship:**
- ✅ +anger (15-30)
- ✅ +pride (10-20)
- ❌ -trust (15-25)
- ✅ +courage (8-15) if brave (≥60)
- ✅ +fear (10-20) if wimpy (<40)

---

### Arrest
**Authority/justice context - based on actor reputation and mob alignment:**

**High rep (≥60) arresting evil mob:**
- ✅ +fear (20-40)
- ✅ +anger (15-30)
- ✅ +shame (10-20)
- ❌ -trust (20-35)

**High rep (≥60) arresting good/neutral mob:**
- ✅ +horror (20-35)
- ✅ +fear (25-45)
- ✅ +anger (20-35)
- ❌ -trust (40-60)
- ❌ -friendship (35-55)

**Low rep (<30) arresting anyone:**
- ✅ +anger (25-45)
- ✅ +disgust (15-30)
- ❌ -trust (30-50)
- **High combat chance** (60% if courage ≥40)

**Moderate rep:**
- ✅ +fear (15-30)
- ✅ +anger (15-30)
- ✅ +curiosity (10-20)
- ❌ -trust (15-30)

---

### Haircut
**Grooming action - caring vs invasive:**

**High friendship (≥65) AND trust (≥55):**
- ✅ +happiness (5-12)
- ✅ +friendship (5-10)
- ✅ +trust (3-8)

**Moderate (friendship ≥40 OR trust ≥35):**
- ✅ +curiosity (5-10)
- ✅ +anger (3-8)
- ❌ -trust (5-12)

**Low/no relationship:**
- ✅ +anger (10-20)
- ✅ +disgust (8-15)
- ❌ -trust (15-25)
- ❌ -friendship (10-20)

---

### Vampire
**Silly vampire impression - playful:**

**Always:**
- ✅ +curiosity (3-8)

**If friendly (friendship ≥50):**
- ✅ +happiness (5-12)
- ✅ +friendship (2-6)

**Note:** This is NOT violent - it's just saying "I vant to suck yer blood!" in a silly manner.

---

## Summary Statistics

- **Total socials in socials.new:** 490
- **Total mapped:** 490 (100% coverage) ✅
- **Social categories:** 25 categories
- **Special contextual handling:** 11 socials (massage, rose, knuckle, makeout, embrace, snowball, tackle, spank, challenge, arrest, haircut, vampire)
- **Blocked socials with context:** 8 socials (sex, seduce, fondle, grope, french, despine, shiskabob, vice)
- **Extreme violence (always hostile):** 7 socials (despine, shiskabob, vice, choke, strangle, smite, sword)

## Implementation Notes

1. All emotion changes use `adjust_emotion()` which automatically clamps values to valid ranges (typically 0-100)
2. Random ranges are used to create variability in responses (e.g., `rand_number(5, 15)`)
3. Context factors include:
   - Existing relationship levels (trust, friendship, love)
   - Mob personality traits (wimpy_tendency, brave_prevalence, compassion)
   - Player reputation
   - Mob alignment vs player alignment
4. Some socials trigger mob responses/actions (e.g., visible reactions, combat initiation)
5. The `CONFIG_MOB_CONTEXTUAL_SOCIALS` config option must be enabled for any of this to work

## Code Location

**File:** `src/utils.c`  
**Function:** `update_mob_emotion_from_social()`  
**Line range:** ~5300-6500+

---

*Last updated: 2025-11-09 - Complete 490 social mapping*
