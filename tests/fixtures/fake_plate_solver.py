#!/usr/bin/env python3
import pathlib
import sys

if len(sys.argv) != 2:
    print("usage: fake_plate_solver.py IMAGE", file=sys.stderr)
    raise SystemExit(2)

image = pathlib.Path(sys.argv[1])
if not image.exists():
    print(f"image not found: {image}", file=sys.stderr)
    raise SystemExit(1)

print("Reading input file 1 of 1:", image)
print("Field center: (RA,Dec) = (83.82208333, -5.39111111) deg.")
print("Field size: 1.0 x 1.0 degrees")
