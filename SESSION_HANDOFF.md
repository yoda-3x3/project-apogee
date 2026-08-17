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
5. ✅ Simulation + telemetry/chart UI — user click-tested and confirmed working
6. ✅ Map + launch-site + live weather integration — **just finished this
   session**, see below; user click-tested and confirmed working
7. ⬜ 3D trajectory view + save/load + installer polish (next up)

## What just happened this session (all committed; push pending user confirmation)

- **Phase 6**: `LaunchSite` (`app/src/models/`, mirrors the `RocketDesign`
  pattern) is the shared model for coordinates/elevation/rail geometry/
  manual-vs-live wind, owned by the new `LaunchSitePanel` and read by
  `FlightPanel` (which took a `LaunchSite&` constructor param, dropping its
  own rail/wind spin boxes from Phase 5). `MapTileWidget`
  (`app/src/widgets/`) is a hand-rolled slippy map on a `QGraphicsView` +
  `QGraphicsScene`: Esri World Imagery XYZ tiles placed directly at their
  Web Mercator pixel coordinates (so panning is just normal scrolling, no
  manual re-centering math), manual drag-to-pan/click-to-set-marker (had
  to hand-roll this instead of `ScrollHandDrag` to distinguish a click
  from a drag), wheel zoom, `QNetworkDiskCache` for on-disk tile caching.
  `LaunchSitePanel` wires the map to lat/lon spin boxes and a **Fetch
  Weather** button that calls the already-existing (Phase 2)
  `WeatherService` synchronously on the UI thread — same blocking-call
  pattern `PartsBrowserPanel` already uses for ThrustCurve search, kept
  for consistency even though `http_transport.hpp`'s own comment flags
  that as something a GUI should ideally background-thread. After a
  flight, `FlightPanel` emits `flightCompleted(eastM, northM)`;
  `LaunchSitePanel` converts that to a landing lat/lon via a flat-earth
  approximation and drops a marker on the map.
- **Real bug hit mid-implementation**: `QPointer<T>` only works for
  `QObject`-derived types, and `QGraphicsPixmapItem` isn't one (the
  `QGraphicsItem` hierarchy deliberately isn't `QObject`-based) — used it
  to guard an in-flight tile request's lambda against the item having
  been deleted by a zoom change, caught at compile time (not a subtle
  runtime bug), fixed with a `sceneGeneration_` counter instead: bumped on
  every scene rebuild, captured by the request lambda, checked before
  touching the raw item pointer at all.
- Elevation is a plain manual spin box (no live elevation lookup — out of
  scope for this phase); rail azimuth stays defaulted to 0 (north) with no
  UI control, matching Phase 5's existing simplification.
- User did a manual interactive click-through this session (map pan/zoom/
  click, Fetch Weather, live-wind toggle, fly with a landing marker
  appearing on the map) and confirmed it works — first phase this project
  has had an actual confirmed interactive test, not just build+test+code-
  review verification. Native-window automation is still unreliable for
  the assistant to do itself in this sandbox (see "Known follow-ups").

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
  via compiling. That function is now `seedIfNeeded()`, after a follow-up
  bug: its `componentCount() > 0` "already seeded" guard meant any
  database that predated a seed-data change (e.g. the OpenRocket catalog
  import replacing the original ~25-part set) would never pick up new
  seed data — fixed with a `seed_meta`/`kSeedVersion` version check,
  verified against a copy of the real (stale) GUI database, not just a
  fresh `:memory:` test db.
- `QPointer<T>` only works for `QObject`-derived types —
  `QGraphicsPixmapItem` isn't one, so it can't guard an in-flight
  `QNetworkReply` callback against its target item having been deleted
  (e.g. by a map zoom level change). Used a manually-bumped generation
  counter instead (`MapTileWidget::sceneGeneration_`): captured by the
  request lambda, checked before the raw item pointer is touched at all.
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
  read-only catalog table has a manufacturer filter (Phase 5 session); the
  Design tab's actual selection combos still don't.
- Native-window UI automation (clicks, screenshots, `SetForegroundWindow`)
  is unreliable for the *assistant* to drive in this sandbox — clicks have
  landed on unrelated host windows more than once. A manual click-through
  by the user is the reliable verification path; that's what confirmed
  both Phase 5 and Phase 6 this session. Prefer non-visual checks
  (headless tool output, DB inspection) for anything the assistant needs
  to self-verify.
- `LaunchSitePanel::onFetchWeatherClicked()` calls `WeatherService`
  synchronously on the UI thread (briefly blocks while the NWS/Open-Meteo
  requests complete) — matches `PartsBrowserPanel`'s existing ThrustCurve
  search pattern, but `data/include/data/http_transport.hpp`'s own comment
  says GUI callers should ideally wrap these in a background `QThread`
  (`SimulationWorker`-style). Neither panel does that yet; worth
  revisiting if either fetch is ever slow enough to be annoying.
- Launch-site elevation is a manual spin box, not looked up automatically
  (e.g. via Open-Meteo's elevation API) — fine for now, flagged as a
  possible nicety. Rail azimuth stays hardcoded to 0 (north) with no UI
  control, same simplification Phase 5 already made.
- `MapTileWidget`'s on-disk tile cache (`QNetworkDiskCache`) depends on
  Esri's response headers actually enabling HTTP caching — not verified
  either way; if tiles seem to always re-fetch across app restarts, check
  that first.
- No "center map on landing marker" convenience after a flight — if the
  landing point drifts outside the currently-panned view, the user has to
  manually pan/zoom to find it.

## How to resume

Just say "cleared for phase 7" (or whatever's next) and go — the assistant
should re-read this file's context, confirm the build's still green
(`.\build.ps1 -BuildGui`), and continue from there. Always confirm with the
user before `git push` (standing instruction), and test before pushing.
