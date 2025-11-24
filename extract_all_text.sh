#!/bin/bash
# 
# Script to extract text from all MUD world files in a directory
# Usage: ./extract_all_text.sh <world_directory> <output_file>
#
# Example: ./extract_all_text.sh lib/world training_data.txt
#

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <world_directory> <output_file>"
    echo ""
    echo "Example: $0 lib/world training_data.txt"
    echo ""
    echo "This script extracts text from all .wld, .mob, and .obj files"
    echo "in the specified world directory and outputs to a single file."
    exit 1
fi

WORLD_DIR="$1"
OUTPUT_FILE="$2"
SCRIPT_DIR="$(dirname "$0")"
TEXTEXTRACT="$SCRIPT_DIR/bin/textextract"

# Check if textextract utility exists
if [ ! -f "$TEXTEXTRACT" ]; then
    echo "Error: textextract utility not found at $TEXTEXTRACT"
    echo "Please compile it first: cd src/util && make textextract"
    exit 1
fi

# Check if world directory exists
if [ ! -d "$WORLD_DIR" ]; then
    echo "Error: World directory '$WORLD_DIR' not found"
    exit 1
fi

# Clear the output file
> "$OUTPUT_FILE"

echo "Starting text extraction from $WORLD_DIR..."
echo "Output file: $OUTPUT_FILE"
echo ""

# Process room files (.wld)
echo "Processing room files (.wld)..."
if [ -d "$WORLD_DIR/wld" ]; then
    for file in "$WORLD_DIR/wld"/*.wld; do
        if [ -f "$file" ]; then
            echo "  Processing: $file"
            "$TEXTEXTRACT" wld "$file" "$OUTPUT_FILE"
        fi
    done
else
    echo "  No wld directory found, skipping room files"
fi

# Process mob files (.mob)
echo "Processing mob files (.mob)..."
if [ -d "$WORLD_DIR/mob" ]; then
    for file in "$WORLD_DIR/mob"/*.mob; do
        if [ -f "$file" ]; then
            echo "  Processing: $file"
            "$TEXTEXTRACT" mob "$file" "$OUTPUT_FILE"
        fi
    done
else
    echo "  No mob directory found, skipping mob files"
fi

# Process object files (.obj)
echo "Processing object files (.obj)..."
if [ -d "$WORLD_DIR/obj" ]; then
    for file in "$WORLD_DIR/obj"/*.obj; do
        if [ -f "$file" ]; then
            echo "  Processing: $file"
            "$TEXTEXTRACT" obj "$file" "$OUTPUT_FILE"
        fi
    done
else
    echo "  No obj directory found, skipping object files"
fi

echo ""
echo "Text extraction completed!"
echo "Output saved to: $OUTPUT_FILE"

# Show some statistics
if [ -f "$OUTPUT_FILE" ]; then
    lines=$(wc -l < "$OUTPUT_FILE")
    words=$(wc -w < "$OUTPUT_FILE")
    chars=$(wc -c < "$OUTPUT_FILE")
    echo ""
    echo "Statistics:"
    echo "  Lines: $lines"
    echo "  Words: $words"
    echo "  Characters: $chars"
fi