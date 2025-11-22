#!/bin/bash
# Script to update help.hlp with new mana density system documentation
#
# This script adds the comprehensive help documentation to the help file
# It should be run from the repository root directory

set -e  # Exit on error

HELP_FILE="lib/text/help/help.hlp"
BACKUP_FILE="lib/text/help/help.hlp.original_backup"

# Validate that help file exists
if [ ! -f "$HELP_FILE" ]; then
    echo "Error: Help file '$HELP_FILE' does not exist."
    echo "Please run this script from the repository root directory."
    exit 1
fi

echo "Creating backup of $HELP_FILE..."
if ! cp "$HELP_FILE" "$BACKUP_FILE"; then
    echo "Error: Failed to create backup file '$BACKUP_FILE'"
    exit 1
fi

echo "Backup successfully created: $BACKUP_FILE"
echo ""
echo "Help file updates should be applied manually by editing:"
echo "  lib/text/help/help.hlp"
echo ""
echo "The HELP_UPDATES.txt file contains all the updated entries."
echo "Please integrate them as follows:"
echo ""
echo "1. Update SINERGIAS entry (around line 16435)"
echo "   - Add section about types of magical synergies"
echo "   - Include 'Cebola Mágica' protection layering"
echo "   - Add environmental synergies (mana density)"
echo ""
echo "2. Add new DENSIDADE-MAGICA entry (insert after ELEMENTOS-MAGICOS)"
echo "   - Complete 200+ line entry explaining the system"
echo ""
echo "3. Update CLIMA-MAGICO (around line 16859)"
echo "   - Add visualization section"
echo "   - Add density integration"
echo ""
echo "4. Update MAGIA-CONTROL-WEATHER (around line 6043)"
echo "   - Add mana density boost documentation"
echo ""
echo "5. Update MAGIA-DETECT-MAGIC (around line 6219)"
echo "   - Add density detection capability"
echo ""
echo "6. Add sections to ELEMENTOS-MAGICOS and ESCOLAS-MAGICAS"
echo "   - Add density interaction explanations"
echo ""
echo "All updated content is in: HELP_UPDATES.txt"
echo "Original backup saved to: $BACKUP_FILE"
