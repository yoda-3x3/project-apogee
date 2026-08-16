# Seed data

`components.json` and `kits.json` are a small hand-curated starter catalog
loaded into the local SQLite database on first run (see `seed_loader.cpp`).

Dimensions are representative approximations in the general range of real
Estes BT-20/BT-50/BT-60/BT-80-class hobby parts, **not** transcribed from a
current manufacturer technical data sheet. Stability-margin accuracy is only
as good as these numbers -- treat this catalog as a working starting point
to expand and correct against real data sheets, not a source of truth.

Each entry has a `key` used only within these seed files, to let
`kits.json` reference `components.json` entries before either has a real
database id.
