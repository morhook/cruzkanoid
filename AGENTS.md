# AGENTS.md - Development Guidelines for Cruzkanoid

## Build Commands

### Primary Build Command
```bash
./make_and_run.sh
```
This script builds and runs the game using DOSBox-x with Turbo C.

### Manual Build Commands
```bash
# Build using DOSBox-x (recommended)
dosbox-x -c "mount c $(pwd)" -c "c:" -c "call ADDPATH.BAT" -c "BUILD.BAT" -fs

# Direct compilation command (inside DOSBox)
tcc -ml -Ic:\tc\include -Lc:\tc\lib CRUZKAN.C AUDIO.C VIDEO.C MOUSE.C

# Run compiled game
dosbox-x CRUZKAN.EXE
```

### Testing
- No automated test framework exists
- Manual testing by running the game after compilation
- Test game mechanics: paddle movement, ball physics, brick collision, scoring, sound

## Code Style Guidelines

### File Encoding and Line Endings
- **CRITICAL**: Always use CRLF line endings for C source files
- Use `unix2dos` command instead of custom scripts to convert line endings
- This ensures compatibility with Turbo C on DOS

### Header Structure
```c
#include <standard_system_headers.h>
#include <dos.h>  // DOS-specific headers

#include "local_header.h"
#include "another_local.h.h"

#define CONSTANT_NAME value
#define ANOTHER_CONSTANT value

// Global variables
Type global_var = init_value;
```

### Naming Conventions
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `SCREEN_WIDTH`, `PADDLE_MAX_SPEED`)
- **Functions**: `snake_case` (e.g., `mouse_init`, `draw_paddle`)
- **Variables**: `snake_case` (e.g., `ball_stuck`, `current_level`)
- **Types**: `PascalCase` for typedefs (e.g., `Ball`, `Paddle`, `Brick`)
- **File names**: `UPPERCASE.C` and `UPPERCASE.H` (DOS convention)

### Function Declarations
- Exported functions use `far` keyword for Turbo C memory model compatibility
- Header guards use `FILENAME_H` format
- Function ordering: init → update → draw → shutdown

```c
// In header files
#ifndef VIDEO_H
#define VIDEO_H

void far set_palette_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
void far draw_rect(int x, int y, int width, int height, unsigned char color);

#endif /* VIDEO_H */
```

### Memory Model
- Use large memory model (`-ml` flag) for Turbo C compilation
- `far` keyword required for functions accessed across segments
- Direct VGA memory manipulation at `0xA000:0000`

### Code Organization
- **Main game logic**: `CRUZKAN.C`
- **Video/graphics**: `VIDEO.C`/`VIDEO.H`
- **Input handling**: `MOUSE.C`/`MOUSE.H`
- **Audio system**: `AUDIO.C`/`AUDIO.H`
- **Shared definitions**: `CRUZKAN.H`

### Variable Declarations
- Global variables at top of file after includes
- Static variables for file-scope
- Initialize variables at declaration when possible

```c
// Global state
Ball ball;
Paddle paddle;
int score = 0;
int lives = 3;

// File-scope static
static int key_vx = 0;
static int rng_seeded = 0;
```

### Comments and Documentation
- Use C-style `/* */` comments, avoid C++ `//` comments
- Comment complex algorithms and hardware interactions
- Document VGA register access and interrupt usage

### Error Handling
- Check return values from hardware initialization
- Graceful fallbacks (e.g., Sound Blaster → PC speaker)
- Use simple error codes, avoid exceptions

### Hardware-Specific Guidelines
- VGA Mode 13h (320x200, 256 colors)
- Direct memory access through `far` pointers
- BIOS interrupts for system calls
- Port I/O for VGA registers (`outp`, `inp`)

### Build Dependencies
- Turbo C 2.01 compiler
- DOSBox-x for emulation
- Large memory model compilation
- Include paths: `c:\tc\include`
- Library paths: `c:\tc\lib`

## Development Workflow

1. Edit source files using modern editor
2. Convert line endings: `unix2dos *.c *.h`
3. Build using `./make_and_run.sh`
4. Test in DOSBox-x
5. Repeat

## Sound Configuration
- Game reads `BLASTER` environment variable
- Default: `A220 I7 D1` (address, IRQ, DMA)
- Falls back to PC speaker if Sound Blaster unavailable

## Common Issues
- **"Fixup overflow" errors**: Rebuild with `far` function declarations
- **Game too fast/slow**: Adjust DOSBox `cycles` setting
- **No keyboard**: Ensure DOSBox-x window has focus
- **Line ending issues**: Always use `unix2dos` before building