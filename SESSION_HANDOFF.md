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
4. ✅ Rocket builder UI
5. ✅ Simulation + telemetry/chart UI — **just finished this session**, see below
6. ⬜ Map + launch-site + live weather integration (next up)
7. ⬜ 3D trajectory view + save/load + installer polish

## What just happened this session (all committed; push pending user confirmation)

- **Phase 5**: `SimulationWorker` (`app/src/workers/`, `QThread` subclass
  wrapping `core::Simulation::run`; result read via `result()` after
  `QThread`'s own `finished()` signal rather than a custom signal carrying
  `Telemetry` by value — that fought with moc over metatype-registration
  ordering, see the comment in `simulation_worker.hpp`), a hand-rolled
  `ChartWidget` (`app/src/widgets/`, plain `QPainter`, no QtCharts — palette
  driven so it stays correct across all four themes, dashed flight-phase
  markers, hover tooltip), and `FlightPanel` (`app/src/panels/`) wired into
  the Flight tab: launch-condition inputs (rail length/angle, ground
  wind — Phase 6 will replace wind with live weather, ejection delay), a
  Fly button, three charts (altitude/velocity/acceleration-G) with phase
  markers, and a summary-stats readout matching `core::SummaryStats`.
- **Found + fixed a real bug while verifying the above**: the running
  app's *actual* AppData database still had the original ~24-part
  hand-curated catalog, not the 2,383-part/14-manufacturer OpenRocket
  import from last session — `seedIfEmpty()`'s `componentCount() > 0`
  guard meant any database that existed before the catalog replacement
  would never pick up the new seed data. Fixed by adding a `seed_meta`
  table + `kSeedVersion` constant; renamed to `seedIfNeeded()`, which now
  wipes and reloads components/kits (leaving cached ThrustCurve motor data
  alone) whenever the stored version doesn't match. Verified against a
  copy of the real GUI database via `apogee_seed_tool`, not just a fresh
  `:memory:` test db. Bump `kSeedVersion` in `data/src/seed_loader.cpp`
  next time `components.json`/`kits.json` changes meaningfully.
- Also added a **Manufacturer filter** (`QSortFilterProxyModel`) to
  `PartsBrowserPanel`'s Seeded Parts Catalog table, since the plain table
  was sorted by part type with no way to browse by brand — the user's
  original ask that led to finding the seeding bug above.
- ⚠️ **Native-window UI automation is unreliable in this sandbox**,
  worse than previously noted: a click aimed at the app's Flight tab
  landed on an unrelated Chrome tab instead, and `SetForegroundWindow`
  didn't reliably bring the app window into a desktop screenshot either.
  The Flight tab's Fly → charts flow is implementation-verified (clean
  build, all Catch2 tests green, correct wiring read back file-by-file)
  and the seeding fix is verified against real persisted data, but the
  interactive click-through was **not** visually confirmed this session.
  Worth an actual manual click-through next session before relying on it
  further.

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
  via compiling. (That function is now `seedIfNeeded()` — see this
  session's entry above for the follow-up bug this one had.)
- A **portable standalone build** exists at
  `C:\Users\reach\Dance\project_apogee\dist\` (exe + Qt DLLs + plugin
  folders, deliberately **no** `qt.conf` — see the bullet above for why —
  so it's genuinely relocatable). Desktop and Start Menu shortcuts
  ("Project Apogee") point at it; taskbar pinning needs one manual step
  (Windows blocks programmatic pinning since 10 1809+ — right-click a
  shortcut and choose "Pin to taskbar"). ⚠️ **`dist\` is a manual
  snapshot, not auto-updated** — after any build, refresh it by copying
  `build\apogee_studio.exe` + `build\*.dll` + the plugin subfolders
  (`generic iconengines imageformats networkinformation platforms
  sqldrivers styles tls`) into `dist\`, excluding `qt.conf`. (Not
  refreshed this session — Phase 5's build is only in `build\` so far.)

## Known follow-ups / not yet done

- RocketBuilderPanel's (Design tab) part-slot combo boxes still have
  hundreds of entries per type (e.g. 765 nose cones) with no filter — Qt
  combos support type-ahead search so it's functional, but a
  searchable/filterable picker would be nicer. PartsBrowserPanel's
  read-only catalog table got a manufacturer filter this session; the
  Design tab's actual selection combos did not.
- Native-window UI automation (clicks, screenshots, `SetForegroundWindow`)
  is unreliable in this sandbox — see this session's entry above. When
  re-verifying UI changes, prefer non-visual checks against the real data
  (headless tool output, DB inspection) over interactive click sequences
  where possible; a manual click-through by the user is worth doing for
  anything not covered that way.
- Phase 5's Flight tab (Fly → charts → summary stats) has not had an
  actual interactive click-through yet — worth doing early next session.

## How to resume

Just say "cleared for phase 6" (or whatever's next) and go — the assistant
should re-read this file's context, confirm the build's still green
(`.\build.ps1 -BuildGui`), and continue from there. Always confirm with the
user before `git push` (standing instruction), and test before pushing.
