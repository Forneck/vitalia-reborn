# Text Extraction Utility for LLM Training

## Overview

The `textextract` utility extracts descriptive text content from MUD world files (.wld, .mob, .obj) and preprocesses it for Language Model (LLM) training purposes. This tool is designed specifically for the Vitalia Reborn MUD project to create training datasets from the rich Portuguese text content in the game world.

## Features

- Extracts text from three MUD file types:
  - **Room files (.wld)**: Room names, descriptions, exit descriptions, and extra descriptions
  - **Mobile files (.mob)**: Character names, short descriptions, long descriptions, and detailed descriptions  
  - **Object files (.obj)**: Object names, short descriptions, long descriptions, and extra descriptions

- Text preprocessing includes:
  - Removal of MUD-specific formatting characters (`~` terminators)
  - Whitespace normalization
  - Filtering out very short/empty descriptions
  - Basic deduplication of repeated content
  - Portuguese text preservation

## Usage

### Single File Processing

```bash
./bin/textextract <type> <input_file> <output_file>
```

**Parameters:**
- `type`: File type (`wld`, `mob`, or `obj`)
- `input_file`: Path to the input MUD world file
- `output_file`: Path to the output text file (will be appended to)

**Examples:**
```bash
# Extract text from a room file
./bin/textextract wld lib/world/wld/30.wld training_text.txt

# Extract text from a mob file  
./bin/textextract mob lib/world/mob/30.mob training_text.txt

# Extract text from an object file
./bin/textextract obj lib/world/obj/30.obj training_text.txt
```

### Batch Processing

Use the provided shell script to process all files in a world directory:

```bash
./extract_all_text.sh <world_directory> <output_file>
```

**Example:**
```bash
# Process all world files and output to a single training file
./extract_all_text.sh lib/world complete_training_data.txt
```

This will process all `.wld`, `.mob`, and `.obj` files in the specified directory and combine the extracted text into a single output file.

## Building

The utility is built as part of the standard MUD build process:

```bash
# Using autotools
./configure
cd src/util
make textextract

# Or using CMake
cmake -B build -S .
cmake --build build --target textextract
```

## Output Format

The extracted text is formatted for readability and LLM training:

```
=== SALA: Room Name ===
Room description text here.

Exit description text here.

=== PERSONAGEM: Character Name ===
Character description text here.
Character detailed description here.

=== OBJETO: Object Name ===
Object description text here.
Object extra description here.
```

## Technical Details

- Written in C99 following tbaMUD coding standards
- Handles Portuguese character encoding properly
- Memory-safe with proper cleanup
- Minimal dependencies (standard C library only)
- Follows existing MUD utility patterns

## Integration

The `textextract` utility is designed to be:
- **Optional**: Does not affect normal MUD operation
- **Standalone**: Can be used independently of the running MUD server
- **Batch-friendly**: Suitable for automated text processing workflows
- **Extensible**: Easy to modify for different output formats

## LLM Training Considerations

The output is preprocessed to be suitable for language model training:
- Clean, readable Portuguese text
- Consistent formatting
- Minimal technical/game-specific jargon
- Rich descriptive content ideal for training on narrative text
- Preserved context through sectioning

This makes the extracted text suitable for training Portuguese language models, especially those focused on fantasy/gaming narratives or descriptive writing.