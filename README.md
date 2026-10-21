# Fluid Simulation

A 2D liquid simulator in C and SDL3, built on cellular automata. You draw walls and pour 20 liquids
with real relative densities: liquids mix where they touch, mercury sinks through the rest, and a
pressure view shows how much liquid sits above each cell.

The flow rules follow Jon Gallant's article
[2D Liquid Simulator With Cellular Automaton in Unity](https://www.jgallant.com/2d-liquid-simulator-with-cellular-automaton-in-unity/).
This version adds 20 liquids, mixing and density layering on top of those rules.

![Water poured onto a shelf, then oil, then mercury sinking to the bottom; P switches to the pressure view at the end](docs/demo.gif)

*Water poured onto a shelf, then oil, then mercury sinking to the bottom. P switches to the pressure
view at the end. Recorded without a window by [`docs/demo.c`](docs/demo.c), which runs the same
simulation and drawing code as the program.*

## Features

- A 45 × 45 grid of 20 px cells in a single C file (`main.c`)
- 20 liquids, from gasoline (0.72) to mercury (13.56), with their relative densities
- Compression, so a column of liquid pushes up through connected containers
- Liquids that don't mix layer by density (mercury sinks through everything)
- Liquids mix where they touch (colour and density averaged by volume)
- A pressure view

## Controls

| Input | Action |
|-------|--------|
| Left mouse drag | Paint the selected liquid (full cell) or wall. Liquid can't be painted over walls. |
| Right mouse drag | Erase cells (liquid or wall) |
| `Space` | Select wall |
| `W` | Select water (selected at start) |
| `P` | Toggle the pressure view |
| Close the window | Quit |

Painting happens when the mouse moves with a button held, so a click without moving paints nothing.

| Key | Liquid | Density | | Key | Liquid | Density |
|-----|--------|---------|-|-----|--------|---------|
| `W` | water | 1.00 | | `Q` | sea water | 1.03 |
| `1` | gasoline | 0.72 | | `S` | orange juice | 1.04 |
| `2` | ethanol | 0.79 | | `D` | soda | 1.04 |
| `3` | kerosene | 0.81 | | `F` | blood | 1.05 |
| `4` | olive oil | 0.91 | | `G` | glycerin | 1.26 |
| `5` | oil | 0.92 | | `H` | syrup | 1.33 |
| `6` | coconut oil | 0.93 | | `J` | honey | 1.42 |
| `7` | vinegar | 1.01 | | `K` | sulfuric acid | 1.84 |
| `8` | coffee | 1.01 | | `L` | bromine | 3.12 |
| `9` | milk | 1.03 | | `M` | mercury | 13.56 |

Keys follow the letter printed on your keyboard. On AZERTY, `Q`–`M` is the home row and the
number row gives `1`–`9` without Shift.

## Build and run

You need a C compiler, `make`, `pkg-config` and SDL3.

```sh
make          # builds ./main
make run      # builds and starts it
make test     # runs the simulation in a closed box and checks it (no window needed)
make clean
```

The Makefile builds with AddressSanitizer (`-fsanitize=address`) except on Windows. For a plain
build, run `make SANITIZE=`. If LeakSanitizer reports small leaks inside the graphics driver when the
program exits (seen with Mesa), run it with `ASAN_OPTIONS=detect_leaks=0`.

| System | Install | Status |
|--------|---------|--------|
| Fedora | `sudo dnf install gcc make SDL3-devel libasan` | Build and `make test` checked on Fedora 44 (SDL 3.4.18) |
| Ubuntu | `sudo apt install gcc make pkg-config libsdl3-dev` | Build and `make test` checked on Ubuntu 26.04 (SDL 3.4.2). Ubuntu 24.04 has no SDL3 package, so build SDL3 from source there. |
| macOS | `brew install sdl3 pkg-config` | Not tested |
| Windows | MSYS2 UCRT64: `pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-sdl3` | Not tested. The Makefile turns ASan off there. |

## How it works

Each cell is empty, a wall, or holds one liquid with a level (1.0 is full). Each frame runs three
passes 5 times:

1. **Flow** (`simulation_step`, the article's rules). A cell first flows down. A cell below can hold
   a little more than 1.0 (`MAX_COMPRESSION`), and that extra is what makes water rise again on the
   other side of a U-tube. The cell then evens out with its left and right neighbours (a quarter of
   the difference each), and pushes up whatever is still compressed. All flows go into a `diff`
   buffer that is applied at the end, so the scan order doesn't bias the flow. Liquid only flows
   into empty cells or cells of the same liquid. Levels under `MIN_FLOW` are cleared, as in the
   article, so a tiny amount of liquid evaporates.
2. **Mix** (`mix_step`). Two different liquids that touch, with densities less than `MIX_THRESHOLD`
   apart, both become a mixture with the volume-weighted colour and density. With the default
   threshold of 10, mercury is the only liquid that stays separate.
3. **Density** (`density_step`). A denser liquid above a lighter one swaps places with it, so it sinks
   one cell per pass.

The pressure view shades each cell by the amount of liquid stacked above it in its column.
