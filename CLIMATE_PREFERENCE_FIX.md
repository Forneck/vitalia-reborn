# Climate Preference Initialization for Mobs

## Issue Description
All mobs in the game were showing identical climate preferences:
- Clima Favorito: [Cloudless]
- Temperatura Favorita: [Very Cold]
- Clima Nativo: [Temperate]
- Horas de Exposição ao Clima: [31 horas]

This lack of variety prevented mobs from having diverse emotional responses to weather conditions.

## Solution
Implemented dynamic climate preference initialization based on spawn conditions and mob level.

## Changes Made

### 1. Modified `init_mob_ai_data()` (src/quest.c)
- Initialize all climate fields to -1 (unset/no preference)
- Initialize SAD (Seasonal Affective Disorder) tendency to -1
- When SAD is unset, calculate using formula: `random(0, max(0, 110 - level))`
- Higher level mobs get lower SAD susceptibility (more emotionally resilient)

### 2. Created `initialize_mob_climate_preferences()` (src/quest.c)
- Called when mob is placed in room during zone reset
- Sets preferences based on current room/zone conditions:
  - **preferred_weather_sky**: Current sky condition in spawn room
  - **preferred_temperature_range**: Current temperature in spawn room
  - **native_climate**: Climate type of spawn zone
- Only initializes if not already set (allows for mob file overrides)

### 3. Modified Zone Reset (src/db.c)
- Added call to `initialize_mob_climate_preferences()` after `char_to_room()`
- Ensures every spawned mob gets preferences based on its spawn location

### 4. Added Builder Customization (src/db.c)
New espec fields in mob files for builder customization:
- **PreferredWeather**: -1 to 4 (0=cloudless, 1=cloudy, 2=raining, 3=lightning, 4=snowing, -1=none)
- **PreferredTemperature**: -1 to 4 (0=very cold, 1=cold, 2=comfortable, 3=hot, 4=very hot, -1=none)
- **NativeClimate**: -1 to 4 (0=temperate, 1=rainy, 2=tropical, 3=arctic, 4=desert, -1=neutral)
- **SeasonalAffective**: 0 to 100 (SAD severity, where 0=no SAD)

### 5. Added Function Declaration (src/quest.h)
- Declared `initialize_mob_climate_preferences()` for use in db.c

## Temperature Ranges
The system categorizes temperature into 5 ranges based on current weather:
- **0 (Very Cold)**: < 0°C
- **1 (Cold)**: 0-10°C
- **2 (Comfortable)**: 10-25°C
- **3 (Hot)**: 25-35°C
- **4 (Very Hot)**: > 35°C

## Climate Types
The system recognizes 5 climate types:
- **0 (Temperate)**: Balanced climate
- **1 (Rainy)**: Frequent rainfall
- **2 (Tropical)**: Hot and humid
- **3 (Arctic)**: Cold and icy
- **4 (Desert)**: Hot and dry

## Expected Behavior

### Automatic Initialization
- **Desert Zone Mob**: Will prefer hot/very hot temperatures, cloudless weather, and desert climate
- **Arctic Zone Mob**: Will prefer cold/very cold temperatures and arctic climate
- **Tropical Zone Mob**: Will prefer hot temperatures and tropical climate
- **Mob Spawned During Rain**: Will prefer rainy weather

### Level-Based SAD
- **Level 1 Mob**: SAD range 0-109 (highly susceptible)
- **Level 50 Mob**: SAD range 0-60 (moderately susceptible)
- **Level 100 Mob**: SAD range 0-10 (low susceptibility)
- **Level 110+ Mob**: SAD = 0 (immune)

### Builder Override
Builders can specify exact preferences in mob files:
```
#100
nome do mob~
descrição~
~
descrição longa~
~
flags...
...
PreferredWeather: 2
PreferredTemperature: 3
NativeClimate: 4
SeasonalAffective: 50
E
```

## Impact on Gameplay
- Mobs now have diverse emotional responses to weather based on their preferences
- Climate preferences affect mood through `affect_mob_by_weather()` in utils.c
- Preferred conditions boost happiness and reduce negative emotions
- Non-preferred conditions can increase discomfort, fear, or anger
- SAD affects winter emotional responses (increased sadness/fear in winter)

## Technical Details

### Initialization Flow
1. Mob created via `read_mobile()` → ai_data created
2. `init_mob_ai_data()` called → sets defaults to -1, initializes SAD
3. Mob placed in room via `char_to_room()` in zone reset
4. `initialize_mob_climate_preferences()` called → sets preferences based on room
5. Mob now has unique climate preferences based on spawn location

### Memory Management
- All climate fields are part of `mob_ai_data` struct
- Memory allocated once per mob instance in `read_mobile()`
- No additional memory overhead (fields already existed)

### Performance
- Minimal performance impact
- Climate initialization happens once per mob at spawn time
- No ongoing calculations or overhead

## Testing
- ✓ Code compiles without errors
- ✓ Functions properly linked in executable
- ✓ No compiler warnings in modified files
- ✓ CodeQL security scan passes (0 alerts)
- ✓ Bounds checks verified
- ✓ SAD initialization logic verified

## Files Modified
- `src/quest.c`: Added initialization functions
- `src/quest.h`: Added function declaration
- `src/db.c`: Added zone reset hook and espec parsing

## Backwards Compatibility
- Fully backwards compatible with existing mob files
- Mobs without climate espec fields will auto-initialize based on spawn conditions
- Existing mob files with climate fields will use specified values
- No changes required to existing world files

## Future Enhancements
Potential future improvements:
- Add builder documentation for new espec fields
- Add in-game builder commands to set climate preferences
- Consider adding climate adaptation over time (mobs learn to prefer local conditions)
- Add more granular temperature ranges
- Consider seasonal preference changes
