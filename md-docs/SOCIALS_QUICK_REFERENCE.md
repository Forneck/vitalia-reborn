# Socials Quick Reference Guide

## Question: Can Mobs Use Socials with Each Other?

**Answer: YES ✓**

The code in `src/mobact.c` (function `mob_emotion_activity()`) already supports mob-to-mob social interactions. No modifications needed.

### How It Works

1. Every 4 seconds (PULSE_MOB_EMOTION), mobs check for social opportunities
2. Mobs iterate through ALL characters in their room (including other mobs)
3. Based on emotions, alignment, and reputation, mobs select appropriate socials
4. Mobs execute socials using `do_action()` - works with any character target

### Configuration

Enable in-game with immortal `cedit` command:
- `mob_contextual_socials` = 1 (enable feature)
- `mob_emotion_social_chance` = 10-20 (percentage chance per tick)

---

## Inappropriate Socials (34 total)

### Should NOT Be Used by Mobs Automatically

**Sexual Content (17):**
custard, dive, earlick, fondle, french, grope, joint, licks, massage, moan, pant, seduce, sex, spank, titter, vampire, whip

**Violent/Disturbing (9):**
despine, haircut, halo, knuckle, needle, shock, shiskabob, spork, vice

**Offensive Gestures (8):**
boo, flick, hula, modest, newidea, pushup, rose, suckit-up

### Recommendation
Set `min_level_char = 31` (immortal only) for these socials in `lib/misc/socials.new`

---

## Silly/Nonsense Socials (12 total)

### Potentially Immersion-Breaking

**Bodily Functions (7):**
belch, booger, burp, drool, fart, puke, raspberry

**Absurd Actions (5):**
abc, howl, moon, noogie, wedgie

### Recommendation
- Restrict for normal mobs
- Allow only for "comedic" mob types (jesters, clowns, drunk NPCs)
- Exception: `howl` appropriate for wolves/werewolves

---

## Current Emotion Categories (14)

| Category | Count | Example Socials | Usage Trigger |
|----------|-------|----------------|---------------|
| positive_socials | 18 | smile, bow, applaud, wave | High reputation, friendly alignment |
| negative_socials | 12 | frown, glare, sneer, spit | Low reputation, anger, alignment conflict |
| aggressive_socials | 8 | threaten, challenge, bite, slap | High anger, combat situations |
| fearful_socials | 9 | cower, whimper, cringe, panic | High fear + low courage |
| loving_socials | 13 | hug, cuddle, kiss, embrace | High love or friendship+trust |
| playful_socials | 9 | tickle, poke, tease, bounce*, giggle | High happiness + moderate excitement |
| sad_socials | 5 | cry, sob, weep, sulk | High sadness |
| proud_socials | 5 | strut, flex, boast, brag | High pride |
| envious_socials | 3 | envy, eye, greed | High envy + wealthy target |
| respectful_socials | 3 | salute, curtsey, kneel | Respecting authority/reputation |
| resting_socials | 6 | comfort, pat, calm, console | Target resting + compassion |
| neutral_socials | 10 | ponder, shrug, blink, think | Default/moderate emotions |
| confused_socials | 6 | boggle, puzzle, wonder | High curiosity |
| excited_socials | 3 | bounce*, whoo, cheers | High happiness + very high excitement |
| **grateful_socials** ✨ | 6 | thanks, bow, applaud, salute | happiness ≥60, friendship ≥70, trust ≥60 |
| **mocking_socials** ✨ | 5 | mock, sneer, snicker, jeer | anger ≥60, pride ≥60, courage ≥50 |
| **submissive_socials** ✨ | 7 | cower, grovel, kneel, flinch | fear ≥80, courage ≤20 |
| **curious_socials** ✨ | 6 | peer, wonder, sniff, gaze | curiosity ≥75, excitement 40-69 |
| **triumphant_socials** ✨ | 5 | cheers, flex, roar, strut | pride ≥85, courage ≥70, excitement ≥60 |
| **protective_socials** ✨ | 4 | embrace, pat, comfort | compassion ≥70, loyalty ≥60, target injured |
| **mourning_socials** ✨ | 5 | cry, sob, weep, despair | sadness ≥75, excitement <30 |

✨ = **NEW CATEGORIES IMPLEMENTED**

*Note: 'bounce' appears in both excited and playful categories in code

**Total: 21 emotion categories, ~120 unique socials used by mobs**

---

## Implementation Status ✅

### ALL SUGGESTED CATEGORIES HAVE BEEN IMPLEMENTED

The 7 suggested categories below have been fully implemented in `src/mobact.c`:

1. ✅ **grateful_socials** - Implemented (thanks, bow, applaud, backclap, beam, salute)
2. ✅ **mocking_socials** - Implemented (mock, sneer, snicker, jeer, taunt)
3. ✅ **submissive_socials** - Implemented (cower, grovel, bow, kneel, whimper, cringe, flinch)
4. ✅ **curious_socials** - Implemented (peer, ponder, wonder, sniff, gaze, stare)
5. ✅ **triumphant_socials** - Implemented (cheers, flex, roar, battlecry, strut)
6. ✅ **protective_socials** - Implemented (embrace, pat, comfort, console)
7. ✅ **mourning_socials** - Implemented (cry, sob, weep, sulk, despair)

See `docs/SOCIALS_ANALYSIS.md` for complete implementation details and emotion thresholds.

---

## Testing Mob-to-Mob Socials

### In-Game Test
1. Create two AI-enabled mobs in a room
2. Use `cedit` to enable `mob_contextual_socials`
3. Set `mob_emotion_social_chance` to 50% for faster testing
4. Watch mobs interact with socials
5. Use `stat mob <mobname>` to check emotions

### Expected Behavior
- Mobs with opposite alignments use negative socials
- Happy mobs use positive/playful socials
- Fearful mobs use fearful/submissive socials
- No inappropriate or silly socials appear

---

## Code References

- **Main implementation**: `src/mobact.c` (lines 167-350)
- **Social execution**: `src/act.social.c` (lines 27-109)
- **Configuration**: `src/db.c` (CONFIG_MOB_CONTEXTUAL_SOCIALS)
- **Social data**: `lib/misc/socials.new`

---

## Summary

✅ **Mobs can already use socials with each other** - verified and working  
⚠️ **34 inappropriate socials** need filtering/restrictions  
⚠️ **12 silly socials** should be limited to comedic mobs  
✅ **14 emotion categories** provide diverse mob behavior  
💡 **7 new categories** suggested for enhanced interactions

For complete details, see `docs/SOCIALS_ANALYSIS.md`
