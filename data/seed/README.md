# Seed data

`components.json` and `kits.json` are loaded into the local SQLite database
on first run (see `seed_loader.cpp`). Each entry has a `key` used only
within these seed files, to let `kits.json` reference `components.json`
entries before either has a real database id.

## Sourcing

Body tubes, nose cones, transitions, parachutes, and streamers (2,376 of
the 2,383 components) are imported from the
[OpenRocket component database](https://github.com/openrocket/openrocket-database)
(`orc/*.orc`), licensed Apache License 2.0, Copyright the OpenRocket
project contributors -- every manufacturer file in that repository as of
2026-08-16: BMS, Rocketarium, Apogee Components, Blue Tube, competition
chutes, Estes (classic + PS2), Giant Leap Rocketry, LOC Precision, Madcow,
MPC, Public Missiles, Quest, SEMROC, and Top Flight.

Masses are the source file's own recorded value where given; where absent,
computed from that file's real wall thickness/geometry and material
density (bulk volume x density for solid nose cones/transitions marked
`Filled`, surface density x canopy/panel area x1.25 for parachutes/
streamers to account for unmodeled shroud lines and attachment points).
Components with neither a recorded mass nor a computable one (e.g. a
hollow nose cone with no material density on file) are excluded rather
than guessed.

Fin sets and motor mounts are still estimated -- confirmed absent as a
standalone cataloged part across all 15 source files, not just Estes'.
Rocket kits ship fins as die-cut sheet stock rather than a discrete part,
and motor-mount assemblies (engine tube + centering rings) aren't recorded
as a single combined part either. Treat these seven entries (`fin-20-3`,
`fin-50-3`, `fin-55-4`, `fin-60-4`, `mount-18`, `mount-24`, `mount-29`) as a
reasonable starting point to refine further, not a source of truth.

`kits.json`'s four example kits (Alpha III, Wizard, Big Bertha, Ventris)
reference real imported Estes parts by their generated key, plus the
estimated fin set/motor mount for that size class.
