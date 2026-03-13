# AGENTS.md - Development Guide for Cruzkanoid Agents

This document is for coding agents working in this repository.
Prefer existing DOS/Turbo C conventions over modern C defaults.

## Project Overview
- Language: C (Turbo C era, large memory model)
- Platform: DOS (run via DOSBox-x)
- Output binary: `CRUZKAN.EXE`
- Core files:
  - `CRUZKAN.C` / `CRUZKAN.H` - gameplay and state
  - `VIDEO.C` / `VIDEO.H` - VGA drawing
  - `AUDIO.C` / `AUDIO.H` - speaker/SB/OPL audio
  - `MOUSE.C` / `MOUSE.H` - mouse input via INT 33h
  - `DMAW.C` / `DMAW.H` - WAV DMA playback support

## Build Commands

### Recommended: build and run
```bash
./make_and_run.sh
```
This starts DOSBox-x, mounts the repo, runs `ADDPATH.BAT`, calls `BUILD_R.BAT`,
and runs the game if compilation succeeds.

### Build only
```bash
./make.sh
```
This uses DOSBox-x and `BUILD.BAT`.

### Build with paged compiler output
Run inside DOSBox:
```bat
BUILD_D.BAT
```
Useful when compiler output is long (`| more`).

### Direct compile command (inside DOS)
```bat
tcc -ml -Ic:\tc\include -Lc:\tc\lib CRUZKAN.C AUDIO.C VIDEO.C MOUSE.C DMAW.C
```

### Run command
```bash
dosbox-x CRUZKAN.EXE
```

## Lint / Static Analysis
- No linter is configured in this repository.
- No `clang-format` or `clang-tidy` config exists.
- No lint CI job is present.
- Treat linting as manual conformance to local style and this file.

## Test Commands
- No automated unit/integration test framework exists.
- There is no command for "run all tests".
- There is no command for "run a single test" in the automated sense.

## How to Run a Single Test Scenario (Manual)
Use one focused gameplay behavior as the "single test":
1. Build and run with `./make_and_run.sh`.
2. Verify exactly one behavior only, for example:
   - Paddle movement acceleration/friction
   - Ball bounce against wall/paddle
   - Brick collision and score update
   - Audio toggles (`S` for SFX, `M` for music)
   - Mouse controls (move/fire/pause)
3. Record pass/fail notes for that single scenario.

## Quick Regression Checklist
After gameplay changes, verify at least:
- Launch ball with `Space`
- Lose one life and confirm reset behavior
- Clear level and confirm transition
- Confirm audio fallback if Sound Blaster path fails

## Environment and Runtime Notes
- Emulator: DOSBox-x
- Toolchain: Turbo C (2.01 compatible environment)
- DOS PATH bootstrap script: `ADDPATH.BAT`
- Optional SB config in DOS session:
```bat
SET BLASTER=A220 I7 D1
```

## Source File Rules
- Keep `*.C` and `*.H` files in CRLF line endings.
- Convert line endings with:
```bash
unix2dos *.C *.H
```
- Prefer ASCII text for source files.
- Avoid introducing non-ASCII unless already required.

## Import / Include Guidelines
- Include order:
  1. system headers (`<dos.h>`, `<stdio.h>`, etc.)
  2. blank line
  3. local headers
- Preserve existing include casing per file (`"DMAW.H"`, `"audio.h"`, etc.).
- Keep header guards in `FILENAME_H` style.

## Formatting Guidelines
- Match surrounding formatting in each file.
- Keep braces and indentation consistent with nearby code.
- Avoid broad reformatting in functional changes.
- Use small helper functions for repeated logic.
- Keep global declarations grouped near top of source files.

## Type and Memory Model Guidelines
- Build uses Turbo C large model (`-ml`).
- Public cross-module APIs in headers generally use `far`.
- Preserve `far` pointers and segment-sensitive logic.
- Prefer existing primitive types already used in repo:
  - `int`
  - `unsigned char`
  - `unsigned int`
  - `float`
- Avoid C99+ features likely to break Turbo C compatibility.

## Naming Conventions
- Macros/constants: `UPPER_SNAKE_CASE`
- Functions: `snake_case`
- Variables: `snake_case`
- Typedef struct names: `PascalCase`
- File names: DOS-style uppercase base names with `.C` / `.H`

## State and Encapsulation
- Use `static` for file-local helpers and state.
- Keep shared types in headers, implementation details in `.C` files.
- Do not leak new globals unless cross-module state is required.

## Error Handling Guidelines
- Check return values from hardware initialization paths.
- Use explicit guard branches and return codes.
- Keep failure paths simple and deterministic.
- Preserve graceful fallbacks (for example: SB unavailable -> PC speaker).
- Do not introduce exception-like abstractions.

## Hardware-Specific Expectations
- Graphics target is VGA Mode 13h (320x200, 256 colors).
- Rendering uses direct framebuffer memory (`0xA000:0000` style).
- BIOS/DOS interrupts and I/O ports are normal in this codebase.
- Be conservative in timing-sensitive and DMA/audio code paths.

## Agent Workflow Checklist
1. Read related files and mirror existing patterns.
2. Make minimal, targeted edits.
3. Preserve CRLF in edited C sources/headers.
4. Build via `./make.sh` or `./make_and_run.sh`.
5. Execute at least one focused manual test scenario.
6. Report what was validated and what was not validated.

## Repository Instruction Files (Cursor / Copilot)
- `.cursorrules`: not found
- `.cursor/rules/`: not found
- `.github/copilot-instructions.md`: not found

If these files are added later, update this document and treat them as
repository-specific agent instructions.
