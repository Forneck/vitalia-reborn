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

**Violent/Disturbing (10):**
despine, haircut, halo, knuckle, needle, shock, shiskabob, spork, vice

**Offensive Gestures (7):**
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
| playful_socials | 9 | tickle, poke, tease, giggle | High happiness + excitement |
| sad_socials | 5 | cry, sob, weep, sulk | High sadness |
| proud_socials | 5 | strut, flex, boast, brag | High pride |
| envious_socials | 3 | envy, eye, greed | High envy + wealthy target |
| respectful_socials | 3 | salute, curtsey, kneel | Respecting authority/reputation |
| resting_socials | 6 | comfort, pat, calm, console | Target resting + compassion |
| neutral_socials | 10 | ponder, shrug, blink, think | Default/moderate emotions |
| confused_socials | 6 | boggle, puzzle, wonder | High curiosity |
| excited_socials | 3 | bounce, whoo, cheers | High happiness + excitement |

**Total: 102 unique socials used by mobs**

---

## Suggested New Categories

### High Priority
1. **grateful_socials** - After receiving help (thank, bow, applaud)
2. **mocking_socials** - Taunting enemies (mock, taunt, jeer)
3. **submissive_socials** - Fear-based submission (grovel, kneel, cower)

### Medium Priority
4. **curious_socials** - Investigation (peer, sniff, examine)
5. **triumphant_socials** - Victory celebration (cheer, flex, roar)

### Low Priority
6. **protective_socials** - Guarding allies (guard, shield, stand)
7. **mourning_socials** - Grief response (cry, mourn, wail)

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
