# Sector System Documentation

## Overview

The sector system in Vitalia Reborn defines different terrain types that affect gameplay, particularly movement costs and environmental effects. Each room in the game world is assigned a sector type that determines how difficult it is to traverse.

## Sector Types

Sectors are defined in `src/structs.h` and range from 0 to 16 (17 total types):

| ID | Sector Name | Portuguese | Movement Cost | Description |
|----|-------------|------------|---------------|-------------|
| 0  | SECT_INSIDE | Interior | 1 | Indoor locations |
| 1  | SECT_CITY | Cidade | 1 | City streets and urban areas |
| 2  | SECT_FIELD | Campo | 2 | Open fields and grasslands |
| 3  | SECT_FOREST | Floresta | 3 | Forested areas with trees |
| 4  | SECT_HILLS | Colinas | 4 | Hilly terrain |
| 5  | SECT_MOUNTAIN | Montanha | 6 | Mountain terrain (difficult) |
| 6  | SECT_WATER_SWIM | Água (Nadável) | 4 | Swimmable water |
| 7  | SECT_WATER_NOSWIM | Água (Requer Barco) | 1 | Water requiring a boat |
| 8  | SECT_UNDERWATER | Subaquático | 5 | Underwater areas |
| 9  | SECT_FLYING | Voando | 1 | Flying/in-air areas |
| 10 | SECT_CLIMBING | Escalando | 6 | Climbable surfaces (difficult) |
| 11 | SECT_AIR_FLOW | Corrente de Ar | 2 | Air currents |
| 12 | SECT_QUICKSAND | Areia Movediça | 3 | Quicksand areas |
| 13 | SECT_LAVA | Lava | 2 | Lava terrain |
| 14 | SECT_ICE | Gelo | 1 | Icy surfaces |
| 15 | SECT_DESERT | Deserto | 3 | Desert terrain |
| 16 | SECT_ROAD | Estrada | 1 | Roads and paths (easiest travel) |

## Movement Cost System

### How Movement Costs Work

Movement costs are stored in the `movement_loss[]` array in `src/constants.c`. When a character moves between rooms, the game calculates the movement points required using the formula:

```c
need_movement = (movement_loss[SECT(was_in)] + movement_loss[SECT(going_to)]) / 2;
```

This means the cost is the average of the source and destination sector costs.

### Special Cases

1. **Dead or Light-Affected Characters**: No movement cost
2. **Flying Characters** (AFF_FLYING): Only 1 movement point regardless of terrain
3. **Immortals**: No movement cost
4. **Weather Effects**: Weather conditions can increase movement costs (see `get_weather_movement_modifier()` in `src/spells.c`)

### Movement Cost Guidelines

- **Easy Travel (1 point)**: Indoor, City, Roads, Ice, Flying, Water with Boat
- **Moderate Travel (2-3 points)**: Fields, Air Currents, Lava, Forests, Quicksand, Desert
- **Difficult Travel (4-6 points)**: Hills, Swimming, Underwater, Mountains, Climbing

## Map Display System

### Map Symbols

The ASCII map system (`src/asciimap.c`) displays different symbols for each sector type:

| Sector | Normal Symbol | World Map Symbol | Legend Text |
|--------|---------------|------------------|-------------|
| INSIDE | `[.]` | `.` | Interior |
| CITY | `[C]` (white) | `C` | Cidade |
| FIELD | `[,]` (green) | `,` | Campo |
| FOREST | `[Y]` (green) | `Y` | Floresta |
| HILLS | `[m]` (magenta) | `m` | Colinas |
| MOUNTAIN | `[M]` (red) | `M` | Montanha |
| WATER_SWIM | `[~]` (cyan) | `~` | Água |
| WATER_NOSWIM | `[=]` (blue) | `=` | Barco |
| FLYING | `[^]` (cyan) | `^` | Voando |
| UNDERWATER | `[U]` (blue) | `U` | Mergulhando |
| CLIMBING | `[|]` (cyan) | `|` | Escalando |
| AIR_FLOW | `[V]` (white) | `V` | Corrente Ar |
| QUICKSAND | `[A]` (yellow) | `A` | Areia |
| LAVA | `[L]` (red) | `L` | Lava |
| ICE | `[O]` (cyan) | `O` | Gelo |
| DESERT | `[D]` (yellow) | `D` | Deserto |
| ROAD | `[R]` (yellow-brown) | `R` | Estrada |

### Map Legend

The map legend is automatically generated in the `perform_map()` function and displays on the right side of the map when players use the `map` command.

## Implementation Details

### Adding a New Sector Type

To add a new sector type to the game:

1. **Define the sector constant** in `src/structs.h`:
   ```c
   #define SECT_NEWSECTOR 17  /**< description */
   ```
   
2. **Update NUM_ROOM_SECTORS** in `src/structs.h`:
   ```c
   #define NUM_ROOM_SECTORS 18
   ```

3. **Add movement cost** in `src/constants.c`:
   ```c
   int movement_loss[] = {
       // ... existing entries ...
       2  /* New Sector */
   };
   ```

4. **Add to sector_types array** in `src/constants.c`:
   ```c
   const char *sector_types[] = {
       // ... existing entries ...
       "New Sector", "\n"
   };
   ```

5. **Add map display info** in `src/asciimap.c`:
   ```c
   static struct map_info_type map_info[] = {
       // ... existing entries ...
       {SECT_NEWSECTOR, "\tc[\tGN\tc]\tn"},  /* Symbol for normal map */
   };
   
   static struct map_info_type world_map_info[] = {
       // ... existing entries ...
       {SECT_NEWSECTOR, "\tGN"},  /* Symbol for world map */
   };
   ```

6. **Add to map legend** in `src/asciimap.c` in the `perform_map()` function:
   ```c
   count += sprintf(buf + count, "\tn%s New Sector\\\\", map_info[SECT_NEWSECTOR].disp);
   ```

7. **Test thoroughly** by creating test rooms with the new sector and verifying:
   - Movement costs are calculated correctly
   - Map displays the correct symbol
   - Legend shows the new sector
   - Weather effects work properly (if applicable)

## Files Related to Sectors

- **`src/structs.h`**: Sector type definitions (SECT_* constants)
- **`src/constants.c`**: Movement costs, sector type names
- **`src/asciimap.c`**: Map display symbols and legend
- **`src/act.movement.c`**: Movement logic and cost calculation
- **`src/graph.c`**: Pathfinding with movement costs
- **`src/spells.c`**: Weather-related movement modifiers

## Common Issues and Solutions

### Issue: "Too tired" message when entering sector

**Cause**: Movement cost is undefined or reading garbage memory (array out of bounds).

**Solution**: Ensure the `movement_loss[]` array has an entry for every sector type (0 to NUM_ROOM_SECTORS-1).

### Issue: Sector not showing in map legend

**Cause**: Legend entry not added to the `perform_map()` function.

**Solution**: Add the sector to the legend display code in `src/asciimap.c`.

### Issue: Wrong symbol displayed on map

**Cause**: Mismatch between sector ID and map_info array index.

**Solution**: Verify that `map_info[]` and `world_map_info[]` arrays have entries for all sector types in the correct order.

## Historical Note

The sectors SECT_DESERT (15) and SECT_ROAD (16) were added to the game but initially caused a bug where players would always get "too tired" messages when trying to move through them. This was because the `movement_loss[]` array only had 15 entries (indices 0-14), causing array out-of-bounds access when the movement system tried to read costs for sectors 15 and 16. This was fixed by properly adding entries for these sectors with appropriate movement costs (Desert=3, Road=1) and adding them to the map legend display.
