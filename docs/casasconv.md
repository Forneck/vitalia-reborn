# casasconv - Casa Conversion Utility

## Description

`casasconv` is a utility program that converts text-based house definition files (`casas.txt`) into the binary `hcontrol` format used by Vitalia Reborn's house system.

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
casasconv <casas.txt> <hcontrol>
```

**Parameters:**
- `<casas.txt>` - Input text file containing house definitions
- `<hcontrol>` - Output binary file (usually `lib/etc/hcontrol`)

## Input File Format

The `casas.txt` file should contain one house definition per line with the following format:

```
vnum atrium exit_dir owner_id built_on last_payment num_guests [guest1 guest2 ...]
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `vnum` | integer | Virtual number of the house room |
| `atrium` | integer | Virtual number of the house atrium |
| `exit_dir` | integer | Direction of house exit (0=north, 1=east, 2=south, 3=west, 4=up, 5=down) |
| `owner_id` | long | Player ID of the house owner |
| `built_on` | timestamp | Unix timestamp when the house was built |
| `last_payment` | timestamp | Unix timestamp of the last house payment |
| `num_guests` | integer | Number of guests (0-10) |
| `guest1-N` | long | Guest player IDs (optional, based on num_guests) |

### Example Input File

```
# Example casas.txt file
# Lines starting with # are comments and are ignored

# Casa do jogador 12345 com 2 convidados
1000 999 2 12345 946684800 946684800 2 23456 34567

# Casa do jogador 98765 sem convidados
3001 3000 1 98765 946684800 946684800 0

# Casa do jogador 11111 com 1 convidado
3009 3008 3 11111 946684800 946684800 1 22222
```

## Example Usage

```bash
# Convert casas.txt to hcontrol format
./bin/casasconv casas.txt lib/etc/hcontrol.new

# Backup existing hcontrol file
cp lib/etc/hcontrol lib/etc/hcontrol.backup

# Replace with new file
mv lib/etc/hcontrol.new lib/etc/hcontrol
```

## Important Notes

1. **Always backup your existing `hcontrol` file before replacing it!**
2. The house room (`vnum`) and atrium must exist in your world files
3. The owner and guest player IDs must correspond to valid player characters
4. The utility validates that `num_guests` is between 0 and 10
5. Empty lines and lines starting with `#` are ignored
6. The utility will stop after converting 400 houses (MAX_HOUSES limit)

## Output

The utility provides feedback during conversion:
- Lists each house converted with its details
- Reports the total number of houses converted
- Warns about any validation issues encountered

## Troubleshooting

### "formato invalido" warnings
- Check that each line has at least 7 fields
- Ensure numeric values are valid integers

### Missing guest IDs
- If `num_guests` is N, ensure N guest IDs follow on the same line
- The utility will adjust `num_guests` if fewer IDs are found

## See Also

- `house.c` - House system implementation
- `house.h` - House system structures and definitions
- `do_hcontrol` - In-game house management command
