# casasconv - Casa Conversion Utility

## Description

`casasconv` is a utility program that converts structured text-based house definition files (`casas.txt`) into the binary `hcontrol` format used by Vitalia Reborn's house system.

This tool is useful for migrating house data from older VitaliaMUD formats or for bulk importing house definitions.

## Building

The utility is automatically built with the MUD using either build system:

### CMake Build
```bash
cmake -B build -S .
cmake --build build --target casasconv
```

The executable will be placed in `bin/casasconv`.

### Autotools Build
```bash
./configure
cd src
make
cd util
make casasconv
```

The executable will be in `src/util/casasconv`.

## Usage

```bash
casasconv <casas.txt> <hcontrol> <atrium_map.txt>
```

**Parameters:**
- `<casas.txt>` - Input structured text file containing house definitions
- `<hcontrol>` - Output binary file (usually `lib/etc/hcontrol`)
- `<atrium_map.txt>` - Mapping file that provides atrium and exit direction for each house

## Input File Formats

### casas.txt Format

The `casas.txt` file uses a structured format with class definitions and house blocks:

```
# Comments start with #

class "class-name" {
    # Class properties (these are ignored by the converter)
    type private;
    save 1;
    object-limit 60;
    guest-limit 2;
    rental-period 30;
    rental-value 1000000;
};

house <vnum> {
    class "class-name";      # Class reference (ignored)
    built <timestamp>;       # Unix timestamp when house was built
    owner <player_id>;       # Player ID of the house owner
    payment <timestamp>;     # Unix timestamp of last payment
    guest <player_id>;       # Guest player ID (can have multiple guest lines)
    guest <player_id>;
};
```

### atrium_map.txt Format

Since the structured casas.txt format doesn't include atrium and exit direction information (which are required by the house system), you must provide a separate mapping file:

```
# vnum atrium exit_dir
1050 1049 2
1051 1049 2
3001 3000 1
```

**Fields:**
- `vnum` - Virtual number of the house room (must match a house vnum in casas.txt)
- `atrium` - Virtual number of the house atrium (the room outside the house)
- `exit_dir` - Direction of house exit (0=north, 1=east, 2=south, 3=west, 4=up, 5=down)

### Example casas.txt

```
# VitaliaMUD house definitions

class "priv-midgaard" {
    type private;
    save 1;
    object-limit 60;
    guest-limit 2;
    rental-period 30;
    rental-value 1000000;
};

house 1050 {
    class "priv-midgaard";
    built 1501425160;
    owner 33;
    payment 1743472136;
    guest 164;
    guest 66;
};

house 1051 {
    class "priv-midgaard";
    built 1505128889;
    owner 310;
    payment 1743472138;
    guest 94;
    guest 142;
};
```

## Example Usage

```bash
# Convert casas.txt to hcontrol format
./bin/casasconv casas.txt hcontrol.new atrium_map.txt

# Backup existing hcontrol file
cp lib/etc/hcontrol lib/etc/hcontrol.backup

# Replace with new file
mv hcontrol.new lib/etc/hcontrol
```

## Important Notes

1. **Always backup your existing `hcontrol` file before replacing it!**
2. The house room (`vnum`) and atrium must exist in your world files
3. The owner and guest player IDs must correspond to valid player characters
4. Each house in casas.txt must have a corresponding entry in atrium_map.txt
5. The utility validates that guest count doesn't exceed 10 (MAX_GUESTS limit)
6. Empty lines and lines starting with `#` are ignored
7. Class definitions are parsed but ignored (not used in the conversion)
8. The utility will stop after converting 400 houses (MAX_HOUSES limit)

## Output

The utility provides feedback during conversion:
- Lists each house converted with its details (owner, atrium, exit direction, guest count)
- Reports the total number of houses converted
- Warns if a house is not found in the atrium mapping file

## Troubleshooting

### "Casa X nao encontrada no mapeamento de atrio"
- The house vnum exists in casas.txt but not in atrium_map.txt
- Add a line to atrium_map.txt: `<vnum> <atrium> <exit_dir>`

### How to determine atrium and exit_dir for a house
1. Look up the house room in your world files (lib/world/wld/*.wld)
2. Find which exit leads out of the house to the main area
3. The atrium is the room that exit leads to
4. The exit_dir is the direction number (0-5) of that exit

## See Also

- `house.c` - House system implementation
- `house.h` - House system structures and definitions
- `do_hcontrol` - In-game house management command
