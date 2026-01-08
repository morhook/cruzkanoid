# Building Cruzkanoid DOS Clone

## Use DOSBox-x with Turbo C/Borland C

1. Install DOSBox-x:
   ```bash
   sudo apt-get install dosbox-x
   ```

2. Download Turbo C 2.01 (free from Borland):
   - Get it from: https://archive.org/details/turboc201

3. Mount and compile:
   ```bash
   dosbox
   # Inside DOSBox-x:
   mount c ~/workspace/cruzkanoid
   mount d ~/downloads/tc
   d:
   cd tc\bin
   c:
   tcc -ml cruzkan.c audio.c
   cruzkan.exe
   ```

## Testing

After compilation, run in DOSBox-x:

```bash
dosbox-x cruzkan.exe
```

Or configure DOSBox-x to auto-run:

```bash
dosbox -c "mount c ." -c "c:" -c "cruzkan.exe"
```

## Troubleshooting

**Issue:** Game runs too fast/slow in DOSBox
- Edit DOSBox config: `~/.dosbox/dosbox-*.conf`
- Adjust `cycles` setting (try `cycles=10000` or `cycles=auto`)

**Issue:** No keyboard response
- Make sure DOSBox-x window has focus
- Try different DOSBox-x versions
