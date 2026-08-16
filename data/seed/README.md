# Seed data

`components.json` and `kits.json` are a small hand-curated starter catalog
loaded into the local SQLite database on first run (see `seed_loader.cpp`).

Each entry has a `key` used only within these seed files, to let
`kits.json` reference `components.json` entries before either has a real
database id.

## Sourcing

Nose cone, body tube, and parachute dimensions/masses are sourced from the
[OpenRocket component database](https://github.com/openrocket/openrocket-database)
(`orc/estes_classic.orc`), licensed Apache License 2.0, Copyright the
OpenRocket project contributors. Body tube masses are computed from that
file's real wall thickness and material density; nose cone masses use that
file's own recorded masses where given. This also corrected two outright
wrong part-number/diameter guesses from an earlier hand-estimated version of
this catalog (what was labeled "BT-60"/"BT-80" here didn't match real Estes
BT-60/BT-80 diameters at all -- real BT-60 is 41.58mm, not the 33.8mm
originally guessed).

Fin sets, motor mounts, and transitions are still estimated -- Estes ships
fins as die-cut balsa sheet stock rather than a discrete cataloged part, so
no standalone real fin-set data exists in the source database, and
motor-mount assemblies (engine tube + centering rings) aren't recorded as a
single combined part either. Treat these as a reasonable starting point to
refine further, not a source of truth -- stability-margin accuracy is only
as good as these numbers.
