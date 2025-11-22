# Multiline Aliases in Vitalia Reborn

## Overview

Vitalia Reborn already supports multiline aliases using semicolon (`;`) separation. This feature allows you to create aliases that execute multiple commands in sequence.

## Syntax

```
alias <name> <command1>;<command2>;<command3>;...
```

## Examples

### Basic Movement Alias
```
alias chess n;n;n;n;w;w;w;w
```
When you type `chess`, it will execute:
- `n` (north)
- `n` (north) 
- `n` (north)
- `n` (north)
- `w` (west)
- `w` (west)
- `w` (west)
- `w` (west)

### Combat Preparation Alias
```
alias prep cast 'bless';cast 'armor';cast 'shield'
```

### Information Gathering Alias
```
alias info score;time;who;weather
```

## How It Works

1. **Creation**: When you create an alias containing semicolons (`;`), it's automatically marked as a "complex alias"
2. **Execution**: When triggered, the alias system splits the command string on semicolons and queues each command for execution
3. **Order**: Commands are executed in the exact order specified in the alias

## Advanced Features

### Variable Substitution
You can use variables in multiline aliases:
- `$1`, `$2`, etc. - substitutes specific arguments
- `$*` - substitutes all arguments

Example:
```
alias goto$1 recall;get $1;wear $1;score
```

### Viewing Aliases
- `alias` - List all defined aliases
- `alias <name>` - Remove an alias by providing just the name

## Implementation Details

- Uses `ALIAS_SEP_CHAR` (`;`) for command separation
- Implemented in `src/interpreter.c` via `perform_complex_alias()` function
- Aliases are stored per-character and persist across sessions
- No limit on the number of commands in a multiline alias (within reason)

## Notes

- Each command in the sequence executes independently
- If one command fails, subsequent commands will still execute
- Commands are queued and processed by the main game loop
- Aliases cannot call other aliases recursively (to prevent infinite loops)