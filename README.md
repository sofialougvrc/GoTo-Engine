# Goto-Engine

Coordinate engine and tracking brain for a computerized GoTo telescope mount.
Takes a target object from a catalog, works out exactly where it is in the
sky right now from a given location, tracks it continuously as Earth
rotates, and drives a two-axis mount to follow it — with a swappable mount
backend (simulated now, real stepper motors later).

Inspired by how commercial GoTo mounts work: pick a target, the mount finds
and tracks it automatically. This is that system, built from scratch —
coordinate math, tracking loop, and eventually motor control and camera-based
self-calibration.

## Status
Phases 1–7 complete and verified — full coordinate engine, catalog, virtual
mount, tracking loop, and CLI all working end to end in simulation. No
hardware yet; everything below was validated against reference values, not
just internal tests.

**Verified:**
- Sidereal time (JD, GMST, LST) tested against frozen USNO reference values
- RA/Dec → Alt/Az transform tested against USNO-derived Alt/Az references,
  plus zenith/pole/below-horizon boundary cases
- Full Messier catalog (110 objects) loading correctly
- Virtual mount + tracking loop producing monotonic movement over
  simulated time
- GPIO-backed stepper mount layer tested with fake GPIO call logs
- TMC2209 UART config serialization tested with fake UART writes
- CLI live-tracks a real object end to end:
  `goto-engine track M42 --lat 38.9072 --lon -77.0369 --ticks 2`

## Why
The hard problem in a GoTo mount isn't spinning motors — it's the math. A
star's position is fixed in equatorial coordinates (RA/Dec), but where it
actually appears from a given point on Earth changes continuously as the
planet rotates. Solving and testing that math first, against known
reference values, before any hardware exists, was the point of Phases 1-7.

## Architecture
Full design notes in [docs/PLAN.md](docs/PLAN.md).

## Roadmap
- [x] Sidereal time / UTC handling
- [x] RA/Dec → Alt/Az coordinate transform
- [x] Object catalog (full Messier catalog, 110 objects)
- [x] Virtual mount + tracking loop
- [x] CLI target selection + live tracking
- [x] Test suite validated against USNO reference values
- [x] Stepper mount software backend with fake GPIO tests
- [x] Real Pi GPIO backend scaffold using lgpio
- [ ] Hardware validation on Raspberry Pi + TMC2209 + motors
- [ ] Plate solving (camera-based self-calibration — blocked on hardware)

## Stack
C++20, GoogleTest · nlohmann/json (catalog data)

## Raspberry Pi GPIO
The real GPIO backend is `gte::LgpioGpio`, which implements the same
`GpioInterface` used by `FakeGpio`. On Raspberry Pi OS, install lgpio:

```bash
sudo apt update
sudo apt install liblgpio-dev
```

CMake auto-enables the backend when it finds `lgpio.h` and `liblgpio`:

```bash
cmake -S . -B work/cmake-build
cmake --build work/cmake-build
```

For the Makefile path on the Pi:

```bash
make test LGPIO_CXXFLAGS=-DGTE_ENABLE_LGPIO LGPIO_LIBS=-llgpio
```

Pass the GPIO chip number when constructing `LgpioGpio`; Raspberry Pi 5
systems commonly use `/dev/gpiochip4`, but some newer setups expose the
40-pin header on `/dev/gpiochip0`.
