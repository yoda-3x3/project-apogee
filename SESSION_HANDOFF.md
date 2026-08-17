# Project Apogee — session handoff

Paste this into a new chat to pick up where this session left off.

Project root: `C:\Users\reach\Dance\project_apogee`
GitHub: https://github.com/yoda-3x3/project-apogee (public, pushed, clean)
Plan file: `C:\Users\reach\.claude\plans\help-me-make-a-prancy-storm.md`

## What this app is

A free, open-source C++/Qt6 model rocket flight simulator: design a rocket
from a real parts catalog, see live CG/CP/stability, fly it through a full
6DOF physics sim with live weather, see telemetry. Built in phases, each
committed and pushed after the user confirms.

## Toolchain (see BUILD.md)

- Qt 6.9.3 mingw_64 kit at `C:\Users\reach\Dance\Qt\6.9.3\mingw_64`, built
  with its own bundled MinGW 13.1.0 (`Qt\Tools\mingw1310_64`) — there's a
  second, unrelated MinGW on this machine (winget install, used by a
  different project) that must not get mixed in.
- Always build via `.\build.ps1` (add `-BuildGui` for the GUI, `-Clean` for
  a fresh reconfigure) — it pins the right compiler to PATH for its own
  process only.
- No vcpkg. Everything uses stock Qt (Core/Widgets/Gui/Network/Sql/Svg).

## Phase status (plan has 7 phases total)

1. ✅ Scaffold + theming (Classic/Dark/Light/High-Contrast QSS themes)
2. ✅ Data layer: SQLite schema, repositories, ThrustCurve.org/NWS/
   Open-Meteo clients
3. ✅ 6DOF physics core (`core/`): Vec3/Quaternion/RK4, Barrowman, motor
   model, flight-phase state machine, `Simulation::run`
4. ✅ Rocket builder UI — **just finished this session**, see below
5. ⬜ Simulation + telemetry/chart UI (next up)
6. ⬜ Map + launch-site + live weather integration
7. ⬜ 3D trajectory view + save/load + installer polish

## What just happened this session (all committed + pushed)

- Phase 4 core: `RocketDesign` model (auto-stacks parts into a
  `core::RocketDefinition`, live CG/CP/stability), `RocketDiagramWidget`
  (2D side-profile), `RocketBuilderPanel` (per-slot combos + "Load Kit"),
  `PartsBrowserPanel` (catalog browser + ThrustCurve motor search).
- Restructured `MainWindow` around mode tabs (**Design** / **Launch** /
  **Flight**) instead of dock widgets, per user request — Launch and
  Flight are placeholder tabs until Phases 5–6 fill them in.
- **Seed catalog completely replaced**: imported real parts from every
  manufacturer file in the
  [OpenRocket component database](https://github.com/openrocket/openrocket-database)
  (Apache 2.0) — 2,376 real body tubes/nose cones/transitions/parachutes/
  streamers across 15 manufacturers, masses computed from real geometry +
  material density where not explicitly given. Fin sets and motor mounts
  are still hand-estimated (confirmed absent as a standalone part in the
  source data, not just for Estes). See `data/seed/README.md` for full
  attribution/sourcing detail.
- Motor search UI is now cascading Manufacturer → Model dropdowns (real
  ThrustCurve.org metadata), not free-text fields.
- App icon: hand-drawn rocket `.ico`, embedded in the exe via a Windows
  `.rc` resource (`app/resources/app_icon.ico` / `.rc`).
- A **portable standalone build** exists at `C:\Users\reach\Dance\project_apogee\dist\`
  (exe + Qt DLLs + plugin folders, deliberately **no** `qt.conf`, so it's
  genuinely relocatable — see gotcha below). Desktop and Start Menu
  shortcuts ("Project Apogee") already point at it. **Taskbar pinning
  needs one manual step**: Windows blocks programmatic taskbar pinning
  since 10 1809+ (Access Denied even via Shell COM automation) — right-click
  either shortcut and choose "Pin to taskbar" yourself.
- ⚠️ **`dist/` is a manual snapshot, not auto-updated.** After any further
  build, refresh it: copy `build\apogee_studio.exe` + `build\*.dll` +
  the plugin subfolders (`generic iconengines imageformats
  networkinformation platforms sqldrivers styles tls`) into `dist\`,
  **excluding `qt.conf`** (see next section for why).

## Real bugs found & fixed this project (worth knowing about)

- **`qt.conf` vs. portability**: `build\qt.conf` hardcodes an absolute path
  to this machine's Qt install so console tools (`apogee_tests.exe`,
  `apogee_seed_tool.exe`) can find the SQLite driver plugin (they aren't
  `windeployqt`'d). That's fine for `build\`, but it would silently break
  a *distributed* `apogee_studio.exe` copy on another machine (qt.conf's
  absolute path wins over the locally-deployed plugin folders sitting
  right next to it). This is why `dist\` is a **separate**, qt.conf-free
  folder — don't "simplify" by just zipping `build\`.
- `QSqlDatabase` segfaults deep in `Qt6Sql.dll` with no live
  `QCoreApplication` — confirmed via `gdb` backtrace. Every executable
  touching the DB needs a real `QCoreApplication`/`QApplication` first.
- `qt_standard_project_setup()` only defaults `CMAKE_AUTOMOC`/`AUTOUIC`,
  not `AUTORCC` — a `.qrc` in a plain `add_library()` target (not
  `qt_add_executable()`) silently never compiles without `set(CMAKE_AUTORCC ON)`
  explicitly.
- Qt resources embedded in a **static library** need an explicit
  `Q_INIT_RESOURCE(name)` call in each consuming executable's `main()`,
  or the resource's registration code never gets linked in (silent
  failure, not an error).
- **Barrowman normal-force sign was inverted** — produced a destabilizing
  torque instead of restoring, invisible in a zero-wind test (no AoA ever
  develops) but caused a numerical blowup to NaN within half a second
  under crosswind. Fixed and regression-tested; see Phase 3's commit for
  the two independent hand-derivations that pinned down the correct sign.
- `MainWindow` opened the DB but forgot to call `seedIfEmpty()` — caught
  via actually launching the GUI and seeing an empty parts catalog, not
  via compiling.

## Known follow-ups / not yet done

- RocketBuilderPanel's part-slot combo boxes now have hundreds of entries
  per type (e.g. 765 nose cones) since the catalog import — functional
  (Qt combos support type-ahead search) but a searchable/filterable
  picker would be a nicer UX; not done, flagging for whenever it matters.
- No screenshot-based UI verification is fully reliable in this sandbox —
  clicks/screenshots sometimes land on the host Claude Code window
  instead of the target app. When re-verifying UI changes, prefer: (a)
  registry/QSettings pre-seeding + fresh launch + one screenshot, over
  (b) multi-step click sequences, which are flaky here.
- Phase 5 is next: `SimulationWorker` (QThread wrapper around
  `core::Simulation::run`), hand-rolled `ChartWidget` (QPainter, no
  QtCharts dependency), phase-marker overlays, summary stats panel, wired
  into the Flight tab.

## How to resume

Just say "cleared for phase 5" (or whatever's next) and go — the assistant
should re-read this file's context, confirm the build's still green
(`.\build.ps1 -BuildGui`), and continue from there. Always confirm with the
user before `git push` (standing instruction), and test before pushing.
