# Parnassus Zone Integration Documentation

## Overview
Parnassus (Zone 211) is a mystical tarot reading zone that provides high-level content for players level 26-100. It features an interactive fortune-telling system using the Rider-Waite tarot deck.

## Zone Details
- **Zone Number**: 211
- **Vnum Range**: 21100-21199
- **Level Range**: 26-100 (original tbaMUD levels 8-30, scaled using formula: current_level = tbadoc_level × 3.33)
- **Theme**: Mystical/Tarot/Fortune Telling
- **Language**: Brazilian Portuguese
- **Location**: Connected to Thewster, Avenida Baía das Folhas (room 13404)

## Zone Content

### Rooms (18 total)
- 21100: Documentation room (builder reference)
- 21101: Outside the building (entrance from Thewster)
- 21102: Waiting room with receptionist Ana
- 21103: Brightly-lit corridor (transition room)
- 21110-21125: Sibyl's reading sequence (office + 15 card rooms)
- 21130-21145: Esmerelda's reading sequence (office + 15 card rooms)
- 21150-21165: Jaelle's reading sequence (office + 15 card rooms)
- 21199: Purge room (zone maintenance)

### NPCs (4 total)
All scaled to level 100 for high-level content:
- 21101: Sibyl - Tarot reader (level 100)
- 21102: Esmerelda - Tarot reader (level 100)
- 21103: Jaelle - Tarot reader (level 100)
- 21104: Ana - Receptionist (level 100)

### Objects (82 total)
- 21101-21178: 78 tarot cards (complete Rider-Waite deck)
  - Minor Arcana: Aces through Kings of Wands, Cups, Swords, Pentacles
  - Major Arcana: 0 (The Fool) through XXI (The World)
- 21180: Large glass (for milk)
- 21181: Urn filled with milk (self-replicating)
- 21182: Bokolyi bread (self-replicating)
- 21198: Quill pen
- 21199: Reverser (determines if cards appear upside down)

### Triggers (11 total)
- 21100: Test trigger (not used)
- 21101: Load Cards - Main tarot reading trigger
- 21102: Look Card - Card interpretation trigger
- 21103: Clear the Cards - Cleanup after reading completion
- 21104: Reset the Fortuneteller - Prepare for next client
- 21105: Dealer Greets - Welcome message
- 21106: Receptionist juggles appointments - Appointment system
- 21107: Tarot Receptionist greets - Greeting and instructions
- 21108: Leaving Tarot - Handles player leaving/disconnect
- 21109: (Quill pen trigger reference)
- 21110: (Self-replicating food/drink trigger)

## World Integration

### Connection Details
**From Thewster to Parnassus:**
- Room 13404 (Avenida Baía das Folhas) → West → Room 21101 (Outside Parnassus)

**From Parnassus to Thewster:**
- Room 21101 (Outside Parnassus) → East → Room 13404 (Avenida Baía das Folhas)

### Path from Midgaard (Recall Point)
1. From Temple of Midgaard, travel to Thewster
2. Navigate to Central Square (room 13400)
3. Go west to Avenida Baía das Folhas (room 13404)
4. Go west to Parnassus entrance (room 21101)

## Game Mechanics

### Appointment System
1. Player enters waiting room and speaks with Ana
2. Say "appointment" to request a reading
3. Ana checks availability of three readers
4. Player chooses available reader (Sibyl, Esmerelda, or Jaelle)
5. Ana opens appropriate door and escorts player

### Tarot Reading Process
1. Player enters reader's office
2. Reader greets and gives instructions
3. Player says "shuffle" to prepare the deck
4. Player says "deal" when ready
5. Reader lays out 10 cards in Celtic Cross pattern
6. Player goes upward to begin reading
7. Player navigates through 10 card rooms
8. Each room explains card position in spread
9. Player uses "look card" to see meaning
10. Card may appear normal or reversed (upside down)
11. At final card, player goes down to exit
12. Cards and reader reset for next client

### Self-Replicating Food System
- Milk urn (21181) and bokolyi bread (21182) in waiting room
- When item is taken, another appears automatically
- Prevents players from starving during long readings
- Convenience feature for low-level players without money

## Translation Notes

### Language Consistency
- All player-facing text translated to Brazilian Portuguese
- Maintains fantasy/medieval theme throughout
- Card meanings simplified for game context
- Trigger messages fully localized

### Cultural Adaptations
- "Bokolyi" retained as exotic fantasy food name
- Tarot card names use Portuguese: "Ás" (Ace), "Rei" (King), etc.
- Major Arcana use Portuguese names: "O Louco" (The Fool), "A Morte" (Death)
- Suit names: Paus (Wands), Copas (Cups), Espadas (Swords), Pentagramas (Pentacles)

## Level Scaling Implementation

### Formula Applied
```
current_level = tbadoc_level × 3.33
Max level capped at 100
```

### Before and After
- Original mob level: 30
- Scaled mob level: 30 × 3.33 = 99 → **100 (capped)**
- Original zone range: 8-30 (avg 19.0)
- Scaled zone range: **26-100** (appropriate for high-level content)

## Files Created/Modified

### New Files
- `lib/world/mob/211.mob` - 4 NPCs
- `lib/world/obj/211.obj` - 82 objects
- `lib/world/qst/211.qst` - Empty (no quests)
- `lib/world/shp/211.shp` - Empty (no shops)
- `lib/world/trg/211.trg` - 11 triggers
- `lib/world/wld/211.wld` - 18 rooms
- `lib/world/zon/211.zon` - Zone configuration

### Modified Files
- `lib/world/wld/134.wld` - Added west exit from room 13404
- `lib/world/mob/index` - Added 211.mob
- `lib/world/mob/index.mini` - Added 211.mob
- `lib/world/obj/index` - Added 211.obj
- `lib/world/obj/index.mini` - Added 211.obj
- `lib/world/qst/index` - Added 211.qst
- `lib/world/qst/index.mini` - Added 211.qst
- `lib/world/shp/index` - Added 211.shp
- `lib/world/shp/index.mini` - Added 211.shp
- `lib/world/trg/index` - Added 211.trg
- `lib/world/trg/index.mini` - Added 211.trg
- `lib/world/wld/index` - Added 211.wld
- `lib/world/wld/index.mini` - Added 211.wld
- `lib/world/zon/index` - Added 211.zon
- `lib/world/zon/index.mini` - Added 211.zon
- `changelog.txt` - Added zone integration entry
- `lib/text/help/help` - Added Parnassus help entry

## Testing Recommendations

### Basic Functionality
- [ ] Build server without errors
- [ ] Zone loads correctly on server start
- [ ] Rooms accessible via connection from Thewster
- [ ] NPCs spawn correctly in designated rooms
- [ ] Objects load in correct rooms

### Interactive Features
- [ ] Appointment system functions (Ana responds to "appointment")
- [ ] Reader selection works (Sibyl, Esmerelda, Jaelle)
- [ ] Card shuffling and dealing works
- [ ] Card rooms are accessible and navigable
- [ ] "look card" shows correct meanings
- [ ] Reversed cards display correctly
- [ ] Reading completion cleans up properly
- [ ] Multiple concurrent readings work

### Edge Cases
- [ ] Player disconnect during reading (triggers cleanup)
- [ ] Player goes AFK during reading (handled gracefully)
- [ ] Player tries to quit during reading (prevented with message)
- [ ] All three readers busy (proper message to player)
- [ ] Self-replicating food works correctly

### Portuguese Translation
- [ ] All room descriptions in Portuguese
- [ ] All mob descriptions in Portuguese
- [ ] All object descriptions in Portuguese
- [ ] All trigger messages in Portuguese
- [ ] Help file accessible with "help parnassus"

## Future Enhancements

### Potential Improvements
1. Add quest integration (fortune-telling quest line)
2. Add special rewards for completing readings
3. Add rare card drops for collectors
4. Add achievements for multiple readings
5. Expand with more fortune-telling methods
6. Add seasonal events (special readings)
7. Integration with player statistics (personalized readings)

### Maintenance Notes
- Zone designed to be low-maintenance
- Self-cleaning (cards purge after reading)
- No shop inventory to manage
- No quest tracking needed
- Trigger-driven automation reduces admin work

## Credits
- **Original Builder**: Questor (tbaMUD community)
- **Original Zone**: Parnassus Tarot Zone
- **Translation**: VitaliaMUD Development Team
- **Integration**: December 2024
- **Source**: tbadoc/world zone 211

## License
This zone is derived from tbaMUD content and follows the same DikuMUD license as the base MUD system.

---
*Last Updated: December 10, 2024*
*Version: 1.0*
*VitaliaMUD - Vitalia Reborn Project*
