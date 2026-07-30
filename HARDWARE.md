# Hardware

Parts list for the physical build (Phases 8–9). Software status lives in the main [README](./README.md); this file tracks the hardware side.

## Controller

**Raspberry Pi 5 (4GB), Quad-Core Cortex-A76**
Runs the coordinate engine, tracking loop, and (later) the plate-solving pipeline directly. GPIO is handled via the RP1 southbridge chip, which requires `lgpio` rather than the older `RPi.GPIO`/`pigpio` libraries. Drives both TMC2209 drivers directly over GPIO — no intermediate microcontroller in this build.

## Motor Drivers

**Adafruit TMC2209 Stepper Motor Driver Breakout Board — x2 (one per axis)**
UART-configurable silent stepper driver supporting up to 256x microstepping and StallGuard sensorless stall detection. Takes step/direction signals from the Pi and handles the current-chopping/PWM needed for smooth microstepped motion, without needing a discrete H-bridge or timer-heavy firmware.

## Motors

**STEPPERONLINE NEMA17 Stepper w/ 19:1 Planetary Gearbox (17HS19-1684S-PG19) — x2 (RA/Alt axis, Dec/Az axis)**
Bipolar stepper with an integrated 19.19:1 planetary gearbox, giving 0.094°/step resolution and 3 Nm holding torque. One drives each mount axis; combined with an external gear/belt reduction stage, this provides the total reduction needed for smooth sidereal-rate tracking.

## Camera

**ZWO ASI224MC (refurbished) — 1.2MP CMOS Color Astronomy Camera, USB 3.0**
Entry-level planetary/guide camera used for Phase 9 plate solving — captures a field of stars, which astrometry.net/ASTAP solves against a star index to derive a precise RA/Dec fix, used to self-calibrate mount position instead of relying on manual alignment.

## Power

**ServoCity 12V, 10A Power Supply (NEMA 5-15P input, XT30 MH-FC output)**
Regulated 12VDC supply, 100–240VAC input for regional flexibility, 10A output capacity delivered through an XT30 MH-FC connector. Comfortably covers both NEMA17 steppers under normal TMC2209 current-limit settings (each motor draws well under 2A), with headroom to spare for future accessories on the same rail. Kept separate from the Pi 5's own 5V/5A USB-C supply, grounds tied common.
> Note: the TMC2209 breakouts take screw-terminal power input, not XT30 — a short XT30-to-bare-wire (or XT30-to-screw-terminal) pigtail will be needed to bridge the two.

## Enclosure / Mechanical

**Pololu Stamped Aluminum L-Bracket for NEMA 17 Stepper Motors — x2 (one per axis)**
3mm-thick black anodized aluminum bracket that mounts to a NEMA17 motor's faceplate holes, with slotted mounting holes (2mm of play for fine positioning) and a second face with slots/holes sized for #6, #8, and M4 screws for attaching to the rest of the frame. Comes with four M3×6mm screws for the motor side. This secures each motor to the build but does not include a shaft coupler or the pivoting axis structure itself — those are separate, still-open items below.

**Not yet selected:**
- Rigid or flexible shaft coupler, 8mm bore (matching the PG19 gearbox output shaft) to whatever axle stock is used for each axis.
- Pillow-block bearings (x2-4) sized to that axle diameter.
- The axis frame itself — the part that actually holds and pivots the OTA between the two motors. Likely 3D-printed once shaft/bearing sizes are finalized, using the printer still pending selection above.
- The final gear/belt reduction stage between each stepper's gearbox output and the axis itself — this ratio is currently an unknown constant in the step-calibration math (`final_gear_reduction` placeholder) until this is bench-fitted.

## Wiring notes

- Motor power rail and Pi power should be kept on separate supplies; tie grounds together (common ground) once the power supply is chosen.
- Pi 5 GPIO uses the RP1 chip — confirm whatever GPIO library is used (`lgpio`) supports it before wiring step/dir/enable pins.
- TMC2209 UART config (microstepping, run current) should be set in software per the `Tmc2209Config` module rather than left on default DIP-switch values.
