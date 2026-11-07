# Domain Context & Glossary

This document records the resolved domain terms and system invariants for Weatherdeck.

## Core Domain Terms

### 1. Location
A geographic place identity defined by fixed-precision coordinates (`lat`, `lon` normalized to 4 decimal places, approximately 11 meters resolution), accompanied by a display name, country name/code, administrative region subdivision, and timezone.
- **Identity Invariant**: Two locations are considered identical if and only if their normalized coordinate keys (`QString::asprintf("%.4f,%.4f", lat, lon)`) match. This eliminates duplicate bookmarks arising from slight floating-point differences between geocoding and forecast provider payloads.

### 2. Dashboard State
The coherent, display-ready presentation state exposed to QML through the `DashboardState` (`QObject`) property on `Backend`.
- **Properties**:
  - `locationName`: Current location display string (`City, Country`).
  - `updated`: Localized timestamp of last successful forecast snapshot.
  - `status`: Informational or error status prompt for the user.
  - `loading`: Boolean flag indicating in-flight acquisition.
  - `fahrenheit`: Active temperature and wind unit system (`°F / mph` vs `°C / km/h`).
  - `pinned`: Whether the currently selected location is in the user's pinned locations.
  - `current`: Typed `CurrentConditions` gadget.
  - `airQuality`: Typed `AirQuality` gadget with explicit availability.
  - `hourly`: List of typed `HourlyForecast` gadgets.
  - `daily`: List of typed `DailyForecast` gadgets.
- **Presentation Rule**: QML binds strictly to `backend.dashboard` and its typed sub-properties; no string-keyed maps are exposed.

### 3. Forecast
Meteorological predictions comprising current conditions, hourly projections (up to 24 hours), and daily outlooks (up to 7 days).
- **Alignment Invariant**: Parallel hourly arrays (`time`, `temperature_2m`, `precipitation_probability`, `weather_code`, `relative_humidity_2m`, `uv_index`) and daily arrays (`time`, `temperature_2m_max`, `temperature_2m_min`, `precipitation_probability_max`, `uv_index_max`, `weather_code`) must be validated for alignment. If provider payloads are truncated or misaligned, the parser clamps safely to the minimum consistent length across all series, preventing index out-of-bounds errors.
- **Weather Code & Glyphs**: Numeric WMO weather codes are mapped deterministically into localized descriptions (`Clear`, `Cloudy`, `Fog`, `Drizzle`, `Rain`, `Snow`, `Showers`, `Thunderstorm`) and unicode glyphs (`☀`, `☁`, `≋`, `☂`, `❄`, `⚡`).

### 4. Air Quality
Environmental metrics encompassing European Air Quality Index (AQI) and fine particulate matter (PM2.5).
- **Explicit Availability**: `AirQuality` explicitly tracks `available` (`bool`). If the air quality provider fails or is unreachable, `available` is set to `false`, and the UI displays `--` for AQI and PM2.5 without interrupting or invalidating the weather forecast.

### 5. Saved Location
A user bookmark representing a pinned place for rapid cycling.
- **Deduplication**: Pinned locations are deduplicated using `Location::normalizedKey()`. Adding an existing location is a no-op.
- **Cycling**: `cyclePin(direction)` cycles forward (`+1`) and backward (`-1`) with wrap-around through the bookmark collection.
- **Storage Seam**: Persisted through the `LocationStorage` interface (`QSettingsLocationStorage` in production, `InMemoryLocationStorage` in tests).
