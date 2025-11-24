# UTF-8 Input Filtering Fix - Summary

## Issue
Players could see UTF-8 characters (Portuguese accents like á, é, ã, õ, ç) in game output but could not type them. When players tried to input accented characters, they were filtered out by the server.

## Root Cause
The `process_input()` function in `src/comm.c` (line 2081) used `isascii(*ptr) && isprint(*ptr)` to validate input characters. This check only allows 7-bit ASCII characters (0x00-0x7F) and rejects UTF-8 multi-byte sequences (which use bytes 0x80-0xFF).

## Solution
Added UTF-8-aware input filtering that respects the player's protocol settings:

### New Function: `is_valid_utf8_byte()`
Located in `src/comm.c`, this function:
1. Checks if UTF-8 is enabled via `pProtocol->pVariables[eMSDP_UTF_8]->ValueInt`
2. If UTF-8 is disabled: uses ASCII-only validation (backward compatible)
3. If UTF-8 is enabled: validates UTF-8 multi-byte sequences

### UTF-8 Validation
The function properly validates:
- **2-byte sequences**: Pattern `110xxxxx 10xxxxxx` (covers Latin-1 Supplement, including Portuguese characters)
- **3-byte sequences**: Pattern `1110xxxx 10xxxxxx 10xxxxxx` (covers most of Unicode BMP)
- **4-byte sequences**: Pattern `11110xxx 10xxxxxx 10xxxxxx 10xxxxxx` (covers extended Unicode)
- **Continuation bytes**: Pattern `10xxxxxx` (must follow a start byte)

### Portuguese Characters
Common Portuguese characters are now accepted when UTF-8 is enabled:
- Lowercase: á, à, â, ã, é, ê, í, ó, ô, õ, ú, ç
- Uppercase: Á, À, Â, Ã, É, Ê, Í, Ó, Ô, Õ, Ú, Ç

## Testing
Created test program demonstrating the fix:
- **With UTF-8 enabled**: Words like "café", "ação", "você", "português" fully accepted
- **With UTF-8 disabled**: Only ASCII characters accepted (e.g., "cafe", "acao")

## Security
- CodeQL security scan: **PASSED** (0 alerts)
- Validates UTF-8 sequences to prevent invalid/malformed input
- Backward compatible: ASCII-only mode preserved when UTF-8 is disabled

## Color Code Display Issue (Secondary)
User reported ANSI color codes appearing as `[0;37m` instead of being interpreted.

### Investigation
- Server code correctly includes ESC character (`\x1B`) in all color codes
- `ProtocolOutput()` only filters UTF-8 when disabled, and doesn't strip ESC
- Issue appears to be **client-side** or **network-related**, not server bug

### Possible Causes
1. Client ANSI interpretation disabled or malfunctioning
2. Client in "raw" or "debug" mode
3. Terminal emulator doesn't support ANSI
4. Network proxy/gateway stripping control characters

### Recommendation
- Verify client ANSI settings
- Try different MUD client (MUSHclient, Mudlet, TinTin++)
- Test with direct telnet to isolate issue
- Check for proxies that might strip ESC characters

## Files Modified
- `src/comm.c`: Added UTF-8 validation function and updated input filtering

## Backward Compatibility
✅ Maintained: When UTF-8 is disabled, system behaves exactly as before (ASCII-only)
