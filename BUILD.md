# Building Cruzkanoid DOS Clone from Linux

## Option 1: Cross-compile with DJGPP (Recommended)

### Install DJGPP cross-compiler

**Debian/Ubuntu:**
```bash
sudo apt-get install gcc-djgpp binutils-djgpp
```

**Arch Linux:**
```bash
yay -S djgpp-gcc
```

**Manual Installation:**
Download from http://www.delorie.com/djgpp/

### Compile the game

```bash
i586-pc-msdosdjgpp-gcc -o cruzkan.exe cruzkan.c
```

### Run in DOSBox

```bash
sudo apt-get install dosbox
dosbox cruzkan.exe
```

## Option 2: Use DOSBox with Turbo C/Borland C

1. Install DOSBox:
   ```bash
   sudo apt-get install dosbox
   ```

2. Download Turbo C 2.01 (free from Borland):
   - Get it from: https://archive.org/details/turboc201

3. Mount and compile:
   ```bash
   dosbox
   # Inside DOSBox:
   mount c ~/workspace/cruzkanoid
   mount d ~/downloads/tc
   d:
   cd tc\bin
   c:
   tcc -ml cruzkan.c
   cruzkan.exe
   ```

## Option 3: Use Docker with DJGPP

```bash
docker run -v $(pwd):/src -w /src dockcross/linux-x86 \
  i586-pc-msdosdjgpp-gcc -o cruzkan.exe cruzkan.c
```

## Option 4: Build inside DOSBox-X with compiler

DOSBox-X has better compiler support than regular DOSBox.

```bash
sudo apt-get install dosbox-x
```

## Testing

After compilation, run in DOSBox:

```bash
dosbox cruzkan.exe
```

Or configure DOSBox to auto-run:

```bash
dosbox -c "mount c ." -c "c:" -c "cruzkan.exe"
```

## Troubleshooting

**Issue:** `i586-pc-msdosdjgpp-gcc: command not found`
- Install DJGPP: `sudo apt-get install gcc-djgpp binutils-djgpp`

**Issue:** Game runs too fast/slow in DOSBox
- Edit DOSBox config: `~/.dosbox/dosbox-*.conf`
- Adjust `cycles` setting (try `cycles=10000` or `cycles=auto`)

**Issue:** No keyboard response
- Make sure DOSBox window has focus
- Try different DOSBox versions

**Issue:** Compilation errors with DJGPP
- The current code uses Turbo C/Borland C specifics
- Use the included `cruzkan.c` as-is for Turbo C
- DJGPP version would need modifications (see below)

## Quick Test Script

```bash
#!/bin/bash
# build.sh
set -e

if command -v i586-pc-msdosdjgpp-gcc &> /dev/null; then
    echo "Compiling with DJGPP..."cruzkan    i586-pc-msdosdjgpp-gcc -o cruzkan.exe cruzkan.c
    echo "Success! Run with: dosbox cruzkan.exe"
else
    echo "DJGPP not found. Install with:"
    echo "  sudo apt-get install gcc-djgpp binutils-djgpp"
    exit 1
fi
```

Make executable: `chmod +x build.sh`
Run: `./build.sh`
