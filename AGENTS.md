# Repository Guidelines

## Project Structure & Module Organization

Weatherdeck is a native Qt 6 / Qt Quick Omarchy application. Application logic resides in the flat `src/` directory structured around domain modules and abstract seams:
- `backend.*` — lean presentation mediator facade exposed to QML via `backend`.
- `dashboard_model.*` & `location.*` — typed domain models (`CurrentConditions`, `HourlyForecast`, `DailyForecast`, `AirQuality`, `Location`) and safe Open-Meteo payload parsers with series alignment checks.
- `dashboard_state.*` — `DashboardState` QObject carrying the coherent presentation snapshot for QML bindings.
- `weather_acquisition.*` — acquisition seam (`WeatherAcquisition`) with production `QtWeatherAcquisition` (HTTP via `QNetworkAccessManager`) and `FakeWeatherAcquisition` for deterministic tests.
- `dashboard_snapshot.*` — `DashboardSnapshotCoordinator` managing generation counters, dropping stale network returns, and handling partial failures.
- `saved_locations.*` — bookmark manager (`SavedLocations`) with normalized-coordinate deduplication (`%.4f,%.4f`) and persistence seam (`LocationStorage`, `QSettingsLocationStorage`, `InMemoryLocationStorage`).
- `Main.qml` — presentation view and keyboard navigation binding exclusively to `backend.dashboard`.
- `theme.*` and `translator.*` — integration with Omarchy system theme and dynamic localization. QML and translation resources are listed in `src/resources.qrc`.

Tests live in `tests/` and compile directly against source files. Packaging is under `pkgbuild/`: `PKGBUILD`, the launcher entry, and the SVG icon. Shell entry points belong in `bin/`. Do not commit generated `build/`, `pkgbuild/pkg/`, `pkgbuild/src/`, or package archives.

## Build, Test, and Development Commands

- `bin/build` — runs qmake6 and make, producing `build/weatherdeck`.
- `bin/test` — builds and executes the Qt Test suite offscreen.
- `QT_QPA_PLATFORM=offscreen ./build/weatherdeck` — useful for checking QML load errors without opening a window.
- `bin/install` — builds an Arch package and installs it with `makepkg -fsi`; this may request sudo.

Run commands from the repository root. Use `makepkg -f` inside `pkgbuild/` to rebuild a package after packaging-only changes.

## Coding Style & Naming Conventions

Use C++17 and Qt idioms: four-space indentation, braces on the same line as declarations, `Q_PROPERTY` for state exposed to QML, and `Q_INVOKABLE` for UI actions. Keep fallible work in C++ and layout in QML. Use lower-case file names and `camelCase` methods/properties; Qt classes use `PascalCase`.

All user-visible text must use `tr()` or `qsTr()`. Preserve Omarchy theme, language, RTL, and keyboard-first behavior. Never write to user originals or hide network errors.

## Testing Guidelines

Add a focused Qt Test slot in `tests/weatherdeck_tests.cpp` for each backend behavior or parser change. Test successful responses plus invalid JSON/API error cases. Run `bin/test` before submitting UI or backend changes; manually verify the launcher entry after package changes.

## Commit & Pull Request Guidelines

The repository has no commit history yet. Use short, imperative commit subjects such as `Add hourly forecast parser`. Keep commits scoped. Pull requests should explain behavior changes, list test commands run, link relevant issues, and include a screenshot for visible UI changes.
