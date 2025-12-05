This is a C-based MUD (Multi-User Dungeon) game engine called Vitalia Reborn - a revival of the Brazilian VitaliaMUD from the early 2000s, originally based on CircleMUD and enhanced with tbaMUD improvements. The project focuses on stability, bug fixes, and maintaining the classic MUD experience. Please follow these guidelines when contributing:

## Code Standards

### Required Before Each Commit
- Run `clang-format -i src/*.c src/*.h` to format C source files according to project style
- Build successfully with both build systems to ensure compatibility
- Test memory management when using dynamic allocation (consider using zmalloc functions)

### Development Flow
- **Autotools build**: `./configure && cd src && make`
- **CMake build**: `cmake -B build -S . && cmake --build build`  
- **Format code**: `clang-format -i src/*.c src/*.h`
- **Static analysis**: Enable with `cmake -B build -S . -DSTATIC_ANALYSIS=ON`
- **Memory debugging**: `cmake -B build -S . -DMEMORY_DEBUG=ON && cmake --build build`

## Repository Structure
- `src/`: Core MUD engine source code and main executable
- `src/util/`: Utility programs (asciipasswd, autowiz, shopconv, rebuildIndex, etc.)
- `lib/`: Game world data, player files, configuration, and text files
- `lib/world/`: World files (rooms, objects, NPCs, zones)
- `lib/text/`: Help files, news, and in-game text content
- `bin/`: Built executables (ignored by git)
- `tbadoc/`: Technical documentation and platform-specific build instructions
- `docs/`: Project documentation and guides
- `CMakeLists.txt`: Modern CMake build configuration
- `configure` + `Makefile.in`: Traditional autotools build system

## Key Guidelines
1. **Follow C99 standard** and existing code patterns in the MUD engine
2. **Maintain backward compatibility** with traditional MUD data formats and protocols
3. **Use existing memory management** patterns (prefer zmalloc family functions for debugging)
4. **Preserve game balance** - be cautious with changes affecting gameplay mechanics
5. **Test with sample data** - use existing world files in `lib/world/` to validate changes
6. **Document gameplay changes** in `lib/text/news` when modifying game mechanics
7. **Respect legacy code structure** - this is a revival project maintaining classic MUD architecture
8. **Handle Portuguese content appropriately** - many strings and comments are in Portuguese reflecting the Brazilian origin
9. **References** - Always consult `tbadoc/` and `doc` folder reference for documentation, along the help files.
10. **Self-regulating system** - The systems and mechanics must try to be self-regulating to maintaing the minimum intervencionism while using as few resources as possible

## Platform Considerations
- Primary target is Linux/Unix systems
- Primary environment may use sudo for permissions and access, possible interacting with fewer restrictions
- Windows support via CMake (see `tbadoc/README.CMAKE.md`)
- Multiple legacy platform support documented in `tbadoc/README.*` files
- Both 32-bit and 64-bit architecture support

## MUD-Specific Development Notes
- **World Building**: Changes to world files require understanding of CircleMUD/tbaMUD formats
- **Player Data**: Be extremely careful with changes affecting player file compatibility
- **Networking**: MUD uses traditional telnet-based protocols
- **Real-time Constraints**: Server must handle multiple concurrent players efficiently
- **Game Balance**: Combat, magic, and economic systems require careful consideration of game balance
- **Safety**: New features and modifying existingbones must be throughfull tested to aboid crashhes, and unintended behaviors
- **Immersion**: Try to keep the maximum possible of immersion while maintaining realism and fund.
