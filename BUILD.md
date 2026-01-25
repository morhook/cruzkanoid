# Building Cruzkanoid DOS Clone

## Use DOSBox-x with Turbo C/Borland C

1. Install DOSBox-x:
   ```bash
   sudo apt-get install dosbox-x
   ```

2. Download Turbo C 2.01 (free from Borland):
   - Get it from: https://archive.org/details/turboc201

3. Mount and open IDE:
   ```bash
   dosbox-x
   # Inside DOSBox-x:
   mount c ~/workspace/cruzkanoid
   mount d ~/downloads/tc
   SET PATH=%PATH%;d:\tc\bin
   c:
   tc
   ```

### Niceties

To ease opening TC.EXE, using compilers, linking, etc, from commandline, you can use our ADDPATH.BAT script

```
dosbox-x -c "mount c $HOME/workspace/cruzkanoid/" -c "c:" -c "addpath.bat"
```

Also you can compile directly from commandline with the following command inside DOS

```
tcc -ml -Ic:\tc\include -Lc:\tc\lib CRUZKAN.C AUDIO.C VIDEO.C MOUSE.C DMAW.C
```

So putting everthing together you can run

```
dosbox-x -c "mount c $(pwd)" -c "c:" -c "call ADDPATH.BAT" -c "tcc -ml -Ic:\\tc\\include -Lc:\\tc\\lib CRUZKAN.C AUDIO.C VIDEO.C MOUSE.C DMAW.C" -c "cruzkan" -fs
```

Or even better we have a .BAT that automates most of it
```
dosbox-x -c "mount c $(pwd)" -c "c:" -c "call ADDPATH.BAT" -c "BUILD.BAT" -fs
```

And last but not least we can do a more advanced .BAT that builds, verifies it did it correctly and runs the app:

```
dosbox-x -c "mount c $(pwd)" -c "c:" -c "call ADDPATH.BAT" -c "BUILD_R.BAT" -fs
```

## Sound Blaster (optional, but mandatory to take care of your ears)

The game will try to use Sound Blaster digital audio if it can reset the DSP. It reads the standard `BLASTER` environment variable (for example: `A220 I5 D1`).

In DOSBox-X, ensure Sound Blaster is enabled and the base/IRQ/DMA match `BLASTER` (common defaults are `A220 I7 D1`, depending on config).

Depending your Address/IRQ port/DMA, you might need to set it manually inside DOSBox-X with something like:

```bat
SET BLASTER=A220 I7 D1
```

## Testing

After compilation, run in DOSBox-x:

```bash
dosbox-x CRUZKAN.EXE
```

If you get Turbo C linker errors like “Fixup overflow”, rebuild after these changes: the exported APIs in `VIDEO.H` and `audio.h` now use `far` so the linker can place code in multiple segments without changing the project memory model.

## Troubleshooting

**Issue:** Game runs too fast/slow in DOSBox
- Edit DOSBox config: `~/.dosbox/dosbox-*.conf`
- Adjust `cycles` setting (try `cycles=10000` or `cycles=auto`)

**Issue:** No keyboard response
- Make sure DOSBox-x window has focus
- Try different DOSBox-x versions

## Contributions

Contributions are welcome and never granted! If you want to know more about DOS programming it's a topic I'm passionate about!

Respect the CRLF code convention and we are good to go!
