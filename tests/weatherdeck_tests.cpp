#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QtTest>
#include "backend.h"
#include "dashboard_model.h"
#include "dashboard_snapshot.h"
#include "dashboard_state.h"
#include "location.h"
#include "saved_locations.h"
#include "theme.h"
#include "translator.h"
#include "weather_acquisition.h"

class WeatherdeckTests : public QObject {
    Q_OBJECT
private slots:
    void parsesValidForecastPayload() {
        const QByteArray json = R"({
            "current": {
                "temperature_2m": 21.4,
                "relative_humidity_2m": 63,
                "apparent_temperature": 20.1,
                "precipitation": 0.5,
                "weather_code": 1,
                "wind_speed_10m": 12.3,
                "wind_direction_10m": 120
            },
            "current_units": {
                "temperature_2m": "°C",
                "wind_speed_10m": "km/h"
            },
            "hourly": {
                "time": ["2026-01-01T00:00", "2026-01-01T01:00"],
                "temperature_2m": [21.0, 20.5],
                "precipitation_probability": [5, 10],
                "weather_code": [0, 1],
                "relative_humidity_2m": [60, 65],
                "uv_index": [0.0, 0.0]
            },
            "daily": {
                "time": ["2026-01-01"],
                "weather_code": [0],
                "temperature_2m_max": [25.0],
                "temperature_2m_min": [17.0],
                "precipitation_probability_max": [10],
                "uv_index_max": [4.0]
            }
        })";

        CurrentConditions current;
        QList<HourlyForecast> hourly;
        QList<DailyForecast> daily;
        QString error;

        QVERIFY(DashboardModel::parseForecastPayload(json, &current, &hourly, &daily, &error));
        QVERIFY(error.isEmpty());
        QVERIFY(current.valid);
        QCOMPARE(current.temp, 21.4);
        QCOMPARE(current.feels, 20.1);
        QCOMPARE(current.humidity, 63.0);
        QCOMPARE(current.rain, 0.5);
        QCOMPARE(current.wind, 12.3);
        QCOMPARE(current.direction, 120.0);
        QCOMPARE(current.code, 1);
        QCOMPARE(current.condition, QString("Partly cloudy"));
        QCOMPARE(current.glyph, QString("☁"));
        QCOMPARE(current.iconKey, QString("partly-day"));
        QCOMPARE(current.isDay, true);
        QCOMPARE(current.tempUnit, QString("°C"));
        QCOMPARE(current.windUnit, QString("km/h"));

        QCOMPARE(hourly.size(), 2);
        QCOMPARE(hourly[0].time, QString("00:00"));
        QCOMPARE(hourly[0].temp, 21.0);
        QCOMPARE(hourly[0].glyph, QString("☀"));
        QCOMPARE(hourly[0].iconKey, QString("clear-day"));
        QCOMPARE(hourly[1].time, QString("01:00"));
        QCOMPARE(hourly[1].glyph, QString("☁"));

        QCOMPARE(daily.size(), 1);
        QCOMPARE(daily[0].date, QString("2026-01-01"));
        QCOMPARE(daily[0].max, 25.0);
        QCOMPARE(daily[0].min, 17.0);
        QCOMPARE(daily[0].rain, 10.0);
        QCOMPARE(daily[0].uv, 4.0);
        QCOMPARE(daily[0].glyph, QString("☀"));
        QCOMPARE(daily[0].iconKey, QString("clear-day"));
        QCOMPARE(daily[0].condition, QString("Clear"));
    }

    void mapsWeatherCodesAndGlyphs() {
        QCOMPARE(DashboardModel::conditionForCode(0), QString("Clear"));
        QCOMPARE(DashboardModel::glyphForCode(0), QString("☀"));

        QCOMPARE(DashboardModel::conditionForCode(2), QString("Partly cloudy"));
        QCOMPARE(DashboardModel::glyphForCode(2), QString("☁"));

        QCOMPARE(DashboardModel::conditionForCode(3), QString("Overcast"));

        QCOMPARE(DashboardModel::conditionForCode(45), QString("Fog"));
        QCOMPARE(DashboardModel::glyphForCode(45), QString("≋"));

        QCOMPARE(DashboardModel::conditionForCode(53), QString("Drizzle"));
        QCOMPARE(DashboardModel::glyphForCode(53), QString("☂"));

        QCOMPARE(DashboardModel::conditionForCode(61), QString("Rain"));
        QCOMPARE(DashboardModel::glyphForCode(61), QString("☂"));

        QCOMPARE(DashboardModel::conditionForCode(71), QString("Snow"));
        QCOMPARE(DashboardModel::glyphForCode(71), QString("❄"));

        QCOMPARE(DashboardModel::conditionForCode(80), QString("Showers"));
        QCOMPARE(DashboardModel::glyphForCode(80), QString("⚡"));

        QCOMPARE(DashboardModel::conditionForCode(95), QString("Thunderstorm"));
        QCOMPARE(DashboardModel::glyphForCode(95), QString("⚡"));
    }

    void handlesUnitsAndFormatting() {
        const QByteArray json = R"({
            "current": {
                "temperature_2m": 72.5,
                "relative_humidity_2m": 50,
                "apparent_temperature": 71.0,
                "precipitation": 0.0,
                "weather_code": 0,
                "wind_speed_10m": 8.5,
                "wind_direction_10m": 90
            },
            "current_units": {
                "temperature_2m": "°F",
                "wind_speed_10m": "mph"
            },
            "hourly": {"time": [], "weather_code": [], "temperature_2m": [], "precipitation_probability": [], "relative_humidity_2m": [], "uv_index": []},
            "daily": {"time": [], "weather_code": [], "temperature_2m_max": [], "temperature_2m_min": [], "precipitation_probability_max": [], "uv_index_max": []}
        })";

        CurrentConditions current;
        QList<HourlyForecast> hourly;
        QList<DailyForecast> daily;
        QString error;

        QVERIFY(DashboardModel::parseForecastPayload(json, &current, &hourly, &daily, &error));
        QCOMPARE(current.tempUnit, QString("°F"));
        QCOMPARE(current.windUnit, QString("mph"));
        QCOMPARE(current.temp, 72.5);
    }

    void handlesMalformedAndMisalignedForecastSeries() {
        CurrentConditions current;
        QList<HourlyForecast> hourly;
        QList<DailyForecast> daily;
        QString error;

        // Empty json
        QVERIFY(!DashboardModel::parseForecastPayload("{}", &current, &hourly, &daily, &error));
        QVERIFY(!error.isEmpty());

        // API error response
        const QByteArray apiError = R"({"error": true, "reason": "Latitude must be in range -90 to 90"})";
        QVERIFY(!DashboardModel::parseForecastPayload(apiError, &current, &hourly, &daily, &error));
        QCOMPARE(error, QString("Latitude must be in range -90 to 90"));

        // Missing current object
        const QByteArray noCurrent = R"({"hourly": {}, "daily": {}})";
        QVERIFY(!DashboardModel::parseForecastPayload(noCurrent, &current, &hourly, &daily, &error));

        // Misaligned series: hourly time has 3 items, but temperature has only 1
        const QByteArray misaligned = R"({
            "current": {"temperature_2m": 20, "weather_code": 0},
            "hourly": {
                "time": ["2026-01-01T00:00", "2026-01-01T01:00", "2026-01-01T02:00"],
                "temperature_2m": [20.0],
                "precipitation_probability": [0, 0, 0],
                "weather_code": [0, 0, 0],
                "relative_humidity_2m": [50, 50, 50],
                "uv_index": [0, 0, 0]
            },
            "daily": {
                "time": ["2026-01-01", "2026-01-02"],
                "weather_code": [0],
                "temperature_2m_max": [25.0],
                "temperature_2m_min": [15.0],
                "precipitation_probability_max": [0],
                "uv_index_max": [5.0]
            }
        })";
        QVERIFY(DashboardModel::parseForecastPayload(misaligned, &current, &hourly, &daily, &error));
        // Clamps to min aligned count across parallel arrays
        QCOMPARE(hourly.size(), 1);
        QCOMPARE(daily.size(), 1);
    }

    void parsesAirQualityAndUnavailable() {
        AirQuality aq;
        QString error;

        // Valid air quality
        const QByteArray validAq = R"({"current": {"european_aqi": 34.0, "pm2_5": 11.2}})";
        QVERIFY(DashboardModel::parseAirQualityPayload(validAq, &aq, &error));
        QVERIFY(aq.available);
        QCOMPARE(aq.aqi, 34.0);
        QCOMPARE(aq.pm25, 11.2);

        // Invalid air quality payload
        const QByteArray invalidAq = R"({"error": true, "reason": "No data available"})";
        QVERIFY(!DashboardModel::parseAirQualityPayload(invalidAq, &aq, &error));
        QVERIFY(!aq.available);
        QCOMPARE(error, QString("No data available"));

        // Malformed JSON
        QVERIFY(!DashboardModel::parseAirQualityPayload("not a json", &aq, &error));
        QVERIFY(!aq.available);
    }

    void snapshotSuccess() {
        FakeWeatherAcquisition fake;
        const QByteArray forecastJson = R"({
            "current": {"temperature_2m": 25.0, "weather_code": 0},
            "hourly": {"time": ["2026-01-01T00:00"], "weather_code": [0], "temperature_2m": [25.0], "precipitation_probability": [0], "relative_humidity_2m": [50], "uv_index": [0]},
            "daily": {"time": ["2026-01-01"], "weather_code": [0], "temperature_2m_max": [28.0], "temperature_2m_min": [18.0], "precipitation_probability_max": [0], "uv_index_max": [3]}
        })";
        const QByteArray airJson = R"({"current": {"european_aqi": 25.0, "pm2_5": 8.0}})";
        fake.setForecastResponse(true, forecastJson);
        fake.setAirQualityResponse(true, airJson);

        DashboardSnapshotCoordinator coordinator(&fake);
        DashboardSnapshotData result;
        int readyCount = 0;
        connect(&coordinator, &DashboardSnapshotCoordinator::snapshotReady, [&](const DashboardSnapshotData &data) {
            result = data;
            ++readyCount;
        });

        Location loc;
        loc.name = "Testville";
        loc.country = "TC";
        loc.lat = 10.0;
        loc.lon = 20.0;

        coordinator.requestSnapshot(loc, false);

        QVERIFY(readyCount > 0);
        QCOMPARE(result.locationName, QString("Testville, TC"));
        QCOMPARE(result.current.temp, 25.0);
        QVERIFY(result.airQuality.available);
        QCOMPARE(result.airQuality.aqi, 25.0);
        QCOMPARE(result.airQuality.pm25, 8.0);
    }

    void snapshotPartialAirQualityFailure() {
        FakeWeatherAcquisition fake;
        const QByteArray forecastJson = R"({
            "current": {"temperature_2m": 18.0, "weather_code": 1},
            "hourly": {"time": [], "weather_code": [], "temperature_2m": [], "precipitation_probability": [], "relative_humidity_2m": [], "uv_index": []},
            "daily": {"time": [], "weather_code": [], "temperature_2m_max": [], "temperature_2m_min": [], "precipitation_probability_max": [], "uv_index_max": []}
        })";
        fake.setForecastResponse(true, forecastJson);
        fake.setAirQualityResponse(false, "", "Air API unavailable");

        DashboardSnapshotCoordinator coordinator(&fake);
        DashboardSnapshotData result;
        bool ready = false;
        connect(&coordinator, &DashboardSnapshotCoordinator::snapshotReady, [&](const DashboardSnapshotData &data) {
            result = data;
            ready = true;
        });

        Location loc;
        loc.name = "CloudCity";
        loc.lat = 15.0;
        loc.lon = 25.0;

        coordinator.requestSnapshot(loc, false);

        QVERIFY(ready);
        QCOMPARE(result.current.temp, 18.0);
        QVERIFY(!result.airQuality.available);
    }

    void snapshotFullAcquisitionFailure() {
        FakeWeatherAcquisition fake;
        fake.setForecastResponse(false, "", "Connection refused");

        DashboardSnapshotCoordinator coordinator(&fake);
        QString errorReceived;
        bool failed = false;
        bool loading = true;
        connect(&coordinator, &DashboardSnapshotCoordinator::snapshotFailed, [&](const QString &err) {
            failed = true;
            errorReceived = err;
        });
        connect(&coordinator, &DashboardSnapshotCoordinator::loadingChanged, [&](bool l) {
            loading = l;
        });

        Location loc;
        loc.name = "FailCity";
        loc.lat = 1.0;
        loc.lon = 2.0;

        coordinator.requestSnapshot(loc, false);

        QVERIFY(failed);
        QCOMPARE(errorReceived, QString("Connection refused"));
        QVERIFY(!loading);
    }

    void snapshotManualRefresh() {
        FakeWeatherAcquisition fake;
        fake.setForecastResponse(true, R"({"current":{"temperature_2m":20,"weather_code":0},"hourly":{"time":[]},"daily":{"time":[]}})");
        DashboardSnapshotCoordinator coordinator(&fake);

        Location loc;
        loc.name = "City";
        loc.lat = 12.0;
        loc.lon = 34.0;

        coordinator.requestSnapshot(loc, false);
        const quint64 gen1 = coordinator.generation();

        coordinator.refresh(loc, false);
        const quint64 gen2 = coordinator.generation();

        QCOMPARE(gen2, gen1 + 1);
        QCOMPARE(fake.forecastCallCount(), 2);
    }

    void snapshotStaleGenerationDiscard() {
        FakeWeatherAcquisition fake;
        fake.setDeferCallbacks(true); // Don't invoke callbacks synchronously

        DashboardSnapshotCoordinator coordinator(&fake);
        QList<QString> emittedLocations;
        connect(&coordinator, &DashboardSnapshotCoordinator::snapshotReady, [&](const DashboardSnapshotData &data) {
            emittedLocations.append(data.locationName);
        });

        Location locA;
        locA.name = "CityA";
        locA.lat = 10.0;
        locA.lon = 10.0;

        Location locB;
        locB.name = "CityB";
        locB.lat = 20.0;
        locB.lon = 20.0;

        // 1. Request Loc A (Generation 1)
        coordinator.requestSnapshot(locA, false);

        // 2. Request Loc B (Generation 2)
        coordinator.requestSnapshot(locB, false);

        // Loc A response is returned now (with payload A)
        fake.setForecastResponse(true, R"({"current":{"temperature_2m":10,"weather_code":0},"hourly":{"time":[]},"daily":{"time":[]}})");
        fake.triggerNextForecast();

        // Loc A should be discarded because generation 1 is stale!
        QVERIFY(emittedLocations.isEmpty());

        // Now trigger Loc B (Generation 2)
        fake.setForecastResponse(true, R"({"current":{"temperature_2m":20,"weather_code":0},"hourly":{"time":[]},"daily":{"time":[]}})");
        fake.triggerNextForecast();

        // Loc B should be received
        QCOMPARE(emittedLocations.size(), 1);
        QCOMPARE(emittedLocations[0], QString("CityB"));
    }

    void savedLocationsDeduplication() {
        InMemoryLocationStorage storage;
        SavedLocations saved(&storage);

        Location loc1;
        loc1.name = "Paris";
        loc1.lat = 48.85661;
        loc1.lon = 2.35222;

        Location loc2;
        loc2.name = "Paris Exact";
        loc2.lat = 48.85664; // Same up to 4 decimals: 48.8566, 2.3522
        loc2.lon = 2.35221;

        saved.addPin(loc1);
        QCOMPARE(saved.pins().size(), 1);

        saved.addPin(loc2);
        // Deduplicated because normalized key matches
        QCOMPARE(saved.pins().size(), 1);
        QVERIFY(saved.isPinned(loc1));
        QVERIFY(saved.isPinned(loc2));
    }

    void savedLocationsPinningAndUnpinning() {
        InMemoryLocationStorage storage;
        SavedLocations saved(&storage);

        Location loc;
        loc.name = "Berlin";
        loc.lat = 52.52;
        loc.lon = 13.405;

        QVERIFY(!saved.isPinned(loc));

        // Toggle on
        saved.togglePin(loc);
        QVERIFY(saved.isPinned(loc));
        QCOMPARE(saved.pins().size(), 1);

        // Toggle off
        saved.togglePin(loc);
        QVERIFY(!saved.isPinned(loc));
        QCOMPARE(saved.pins().size(), 0);
    }

    void savedLocationsCycling() {
        InMemoryLocationStorage storage;
        SavedLocations saved(&storage);

        Location loc1; loc1.name = "A"; loc1.lat = 1.0; loc1.lon = 1.0;
        Location loc2; loc2.name = "B"; loc2.lat = 2.0; loc2.lon = 2.0;
        Location loc3; loc3.name = "C"; loc3.lat = 3.0; loc3.lon = 3.0;

        saved.addPin(loc1);
        saved.addPin(loc2);
        saved.addPin(loc3);

        saved.setCurrent(loc1);

        // Cycle forward: A -> B
        saved.cyclePin(1);
        QCOMPARE(saved.current().name, QString("B"));

        // Cycle forward: B -> C
        saved.cyclePin(1);
        QCOMPARE(saved.current().name, QString("C"));

        // Cycle forward: C -> A (wrap-around)
        saved.cyclePin(1);
        QCOMPARE(saved.current().name, QString("A"));

        // Cycle backward: A -> C (wrap-around)
        saved.cyclePin(-1);
        QCOMPARE(saved.current().name, QString("C"));
    }

    void savedLocationsRestoreAndPersistence() {
        InMemoryLocationStorage storage;

        Location currentLoc; currentLoc.name = "Rome"; currentLoc.lat = 41.9; currentLoc.lon = 12.5;
        Location pinLoc; pinLoc.name = "Milan"; pinLoc.lat = 45.46; pinLoc.lon = 9.19;

        {
            SavedLocations saved(&storage);
            saved.setCurrent(currentLoc);
            saved.addPin(pinLoc);
            saved.setFahrenheit(true);
        }

        // Restore in a fresh instance
        {
            SavedLocations fresh(&storage);
            fresh.restore();

            QCOMPARE(fresh.current().name, QString("Rome"));
            QCOMPARE(fresh.pins().size(), 1);
            QCOMPARE(fresh.pins()[0].name, QString("Milan"));
            QVERIFY(fresh.fahrenheit());
        }
    }

    void mediatorSelectedLocationTriggersSnapshot() {
        auto *fake = new FakeWeatherAcquisition();
        auto *storage = new InMemoryLocationStorage();

        const QByteArray forecastJson = R"({
            "current": {"temperature_2m": 22.0, "weather_code": 0},
            "hourly": {"time": []},
            "daily": {"time": []}
        })";
        fake->setForecastResponse(true, forecastJson);

        Location searchLoc;
        searchLoc.name = "Oslo";
        searchLoc.country = "Norway";
        searchLoc.lat = 59.91;
        searchLoc.lon = 10.75;
        fake->setSearchResponse(true, {searchLoc});

        Backend backend(fake, storage);

        backend.search("Oslo");
        QTest::qWait(250); // Allow debounced search timer to trigger
        backend.selectSearchResult(0);

        // After selection, backend coordinates snapshot
        QCOMPARE(backend.dashboard()->locationName(), QString("Oslo, Norway"));
        QCOMPARE(backend.dashboard()->current().temp, 22.0);
    }

    void mediatorUnitChangeRequestsSnapshot() {
        auto *fake = new FakeWeatherAcquisition();
        auto *storage = new InMemoryLocationStorage();

        const QByteArray forecastJson = R"({
            "current": {"temperature_2m": 20.0, "weather_code": 0},
            "hourly": {"time": []},
            "daily": {"time": []}
        })";
        fake->setForecastResponse(true, forecastJson);

        Location loc;
        loc.name = "London";
        loc.lat = 51.5;
        loc.lon = -0.12;
        storage->saveCurrent(loc);

        Backend backend(fake, storage);
        QCOMPARE(fake->forecastCallCount(), 1);
        QVERIFY(!backend.dashboard()->fahrenheit());

        backend.toggleUnits();
        QVERIFY(backend.dashboard()->fahrenheit());
        // Toggle units requests a new snapshot with the new unit
        QCOMPARE(fake->forecastCallCount(), 2);
    }

    void parsesIsDayUvAndHourlyWindow() {
        const QByteArray json = R"({
            "current": {
                "time": "2026-06-01T12:20",
                "temperature_2m": 21.0,
                "weather_code": 0,
                "is_day": 1,
                "uv_index": 5.5
            },
            "hourly": {
                "time": ["2026-06-01T10:00", "2026-06-01T11:00", "2026-06-01T12:00", "2026-06-01T13:00", "2026-06-01T14:00"],
                "weather_code": [1, 0, 0, 3, 2],
                "is_day": [1, 1, 1, 1, 1],
                "temperature_2m": [17.0, 18.0, 19.0, 20.0, 21.0],
                "precipitation_probability": [0, 0, 0, 0, 0],
                "relative_humidity_2m": [60, 60, 60, 60, 60],
                "uv_index": [4, 5, 6, 6, 5]
            }
        })";

        CurrentConditions current;
        QList<HourlyForecast> hourly;
        QList<DailyForecast> daily;
        QString error;

        QVERIFY(DashboardModel::parseForecastPayload(json, &current, &hourly, &daily, &error));
        QCOMPARE(current.isDay, true);
        QCOMPARE(current.uv, 5.5);
        QCOMPARE(current.iconKey, QString("clear-day"));

        // Hours before the current hour are dropped; window starts at 12:00
        QCOMPARE(hourly.size(), 3);
        QCOMPARE(hourly[0].time, QString("12:00"));
        QCOMPARE(hourly[0].temp, 19.0);
        QCOMPARE(hourly[2].time, QString("14:00"));
        QCOMPARE(hourly[2].iconKey, QString("partly-day"));

        // Night codes resolve to night icon keys
        QCOMPARE(DashboardModel::iconKeyForCode(0, false), QString("clear-night"));
        QCOMPARE(DashboardModel::iconKeyForCode(2, false), QString("partly-night"));
        QCOMPARE(DashboardModel::iconKeyForCode(2, true), QString("partly-day"));
        QCOMPARE(DashboardModel::iconKeyForCode(3, true), QString("cloudy"));
        QCOMPARE(DashboardModel::iconKeyForCode(45, true), QString("fog"));
        QCOMPARE(DashboardModel::iconKeyForCode(53, true), QString("drizzle"));
        QCOMPARE(DashboardModel::iconKeyForCode(61, true), QString("rain"));
        QCOMPARE(DashboardModel::iconKeyForCode(66, true), QString("sleet"));
        QCOMPARE(DashboardModel::iconKeyForCode(71, true), QString("snow"));
        QCOMPARE(DashboardModel::iconKeyForCode(80, true), QString("rain"));
        QCOMPARE(DashboardModel::iconKeyForCode(85, true), QString("snow"));
        QCOMPARE(DashboardModel::iconKeyForCode(95, true), QString("thunder"));

        // is_day missing in arrays or current block falls back to day
        const QByteArray legacy = R"({
            "current": {"temperature_2m": 20.0, "weather_code": 61},
            "hourly": {"time": ["2026-06-01T10:00"], "weather_code": [61], "temperature_2m": [18], "precipitation_probability": [0], "relative_humidity_2m": [60], "uv_index": [2]}
        })";
        QVERIFY(DashboardModel::parseForecastPayload(legacy, &current, &hourly, &daily, &error));
        QCOMPARE(current.isDay, true);
        QCOMPARE(current.iconKey, QString("rain"));
        QCOMPARE(hourly.size(), 1); // no current.time: everything before midnight is kept
        QCOMPARE(hourly[0].isDay, true);
        QCOMPARE(hourly[0].iconKey, QString("rain"));
    }

    void exposesAirQualityBands() {
        QCOMPARE(DashboardModel::airQualityBandKey(0.0), QString("good"));
        QCOMPARE(DashboardModel::airQualityBandKey(20.0), QString("good"));
        QCOMPARE(DashboardModel::airQualityBandKey(21.0), QString("fair"));
        QCOMPARE(DashboardModel::airQualityBandKey(40.0), QString("fair"));
        QCOMPARE(DashboardModel::airQualityBandKey(55.0), QString("moderate"));
        QCOMPARE(DashboardModel::airQualityBandKey(70.0), QString("poor"));
        QCOMPARE(DashboardModel::airQualityBandKey(90.0), QString("very-poor"));
        QCOMPARE(DashboardModel::airQualityBandKey(140.0), QString("extreme"));

        QCOMPARE(DashboardModel::airQualityBarFraction(0.0), 0.0);
        QCOMPARE(DashboardModel::airQualityBarFraction(35.0), 0.25);
        QCOMPARE(DashboardModel::airQualityBarFraction(300.0), 1.0);
    }

    void formatsCoordinateDisplay() {
        Location tokyo;
        tokyo.lat = 35.6762;
        tokyo.lon = 139.6503;
        QCOMPARE(tokyo.coordinateDisplay(), QString("35.6762°N, 139.6503°E"));

        Location reykjavik;
        reykjavik.lat = 64.1466;
        reykjavik.lon = -21.9426;
        QCOMPARE(reykjavik.coordinateDisplay(), QString("64.1466°N, 21.9426°W"));

        Location ushuaia;
        ushuaia.lat = -54.8019;
        ushuaia.lon = -68.3030;
        QCOMPARE(ushuaia.coordinateDisplay(), QString("54.8019°S, 68.3030°W"));
    }

    void snapshotCarriesGenerationAndEpoch() {
        FakeWeatherAcquisition fake;
        fake.setForecastResponse(true, R"({"current":{"temperature_2m":12.0,"weather_code":0,"is_day":0},"hourly":{"time":[]},"daily":{"time":[]}})");

        DashboardSnapshotCoordinator coordinator(&fake);
        DashboardSnapshotData result;
        connect(&coordinator, &DashboardSnapshotCoordinator::snapshotReady, [&](const DashboardSnapshotData &data) {
            result = data;
        });

        Location loc;
        loc.name = "Tokyo";
        loc.country = "Japan";
        loc.lat = 35.6762;
        loc.lon = 139.6503;

        coordinator.requestSnapshot(loc, false);

        QVERIFY(result.generation >= 1);
        QVERIFY(result.updatedAt > 0);
        QCOMPARE(result.location.coordinateDisplay(), QString("35.6762°N, 139.6503°E"));
        QCOMPARE(result.current.isDay, false);
        QCOMPARE(result.current.iconKey, QString("clear-night"));
    }

    void mediatorPinListSelectAndUnpin() {
        auto *fake = new FakeWeatherAcquisition();
        auto *storage = new InMemoryLocationStorage();
        fake->setForecastResponse(true, R"({"current":{"temperature_2m":10.0,"weather_code":0},"hourly":{"time":[]},"daily":{"time":[]}})");

        Location tokyo;  tokyo.name = "Tokyo";  tokyo.country = "Japan";  tokyo.lat = 35.68;  tokyo.lon = 139.65;
        Location paris;  paris.name = "Paris";  paris.country = "France"; paris.lat = 48.85;  paris.lon = 2.35;

        storage->savePins({tokyo, paris});
        storage->saveCurrent(tokyo);

        Backend backend(fake, storage);
        // Let the initial snapshot land so pin state is synced
        QTest::qWait(50);

        QCOMPARE(backend.dashboard()->pins().size(), 2);
        QCOMPARE(backend.dashboard()->activePinIndex(), 0);
        QCOMPARE(backend.dashboard()->coordinates(), QString("35.6800°N, 139.6500°E"));
        QVERIFY(backend.dashboard()->generation() >= 1);

        // Select pin index 1: current changes, a snapshot is requested
        backend.selectPin(1);
        QCOMPARE(backend.dashboard()->locationName(), QString("Paris, France"));
        QCOMPARE(backend.dashboard()->activePinIndex(), 1);

        // Unpin index 1: pin list shrinks, current location stays but is no longer pinned
        backend.unpinAt(1);
        QCOMPARE(backend.dashboard()->pins().size(), 1);
        QCOMPARE(backend.dashboard()->activePinIndex(), -1);
        QVERIFY(!backend.dashboard()->pinned());
        QCOMPARE(backend.dashboard()->locationName(), QString("Paris, France"));
    }

    void offscreenQmlLoad() {
        auto *fake = new FakeWeatherAcquisition();
        auto *storage = new InMemoryLocationStorage();

        const QByteArray forecastJson = R"({
            "current": {"time": "2026-01-01T18:00", "temperature_2m": 12.4, "apparent_temperature": 10.2, "relative_humidity_2m": 74, "precipitation": 0.2, "wind_speed_10m": 8.0, "weather_code": 3, "is_day": 0, "uv_index": 0.0},
            "current_units": {"temperature_2m": "°C", "wind_speed_10m": "km/h"},
            "hourly": {
                "time": ["2026-01-01T16:00", "2026-01-01T17:00", "2026-01-01T18:00", "2026-01-01T19:00", "2026-01-01T20:00", "2026-01-01T21:00", "2026-01-01T22:00", "2026-01-01T23:00", "2026-01-02T00:00", "2026-01-02T01:00", "2026-01-02T02:00", "2026-01-02T03:00", "2026-01-02T04:00", "2026-01-02T05:00"],
                "weather_code": [3, 3, 3, 80, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95],
                "is_day": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                "temperature_2m": [13.0, 12.8, 12.4, 11.9, 11.4, 11.0, 10.7, 10.4, 10.2, 10.0, 9.9, 9.8, 9.7, 9.6],
                "precipitation_probability": [10, 10, 10, 20, 40, 60, 70, 70, 70, 60, 50, 40, 25, 15],
                "relative_humidity_2m": [70, 71, 72, 74, 76, 78, 80, 82, 84, 85, 86, 86, 85, 84],
                "uv_index": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
            },
            "daily": {
                "time": ["2026-01-01", "2026-01-02", "2026-01-03", "2026-01-04", "2026-01-05", "2026-01-06", "2026-01-07"],
                "weather_code": [3, 95, 80, 71, 0, 1, 45],
                "temperature_2m_max": [14.0, 12.0, 13.5, 9.0, 8.0, 16.0, 11.0],
                "temperature_2m_min": [7.0, 9.0, 7.0, 4.0, 2.0, 10.0, 3.0],
                "precipitation_probability_max": [20, 70, 50, 30, 0, 5, 40],
                "uv_index_max": [2, 1, 2, 1, 2, 3, 1]
            }
        })";
        const QByteArray airJson = R"({"current": {"european_aqi": 35.0, "pm2_5": 8.4}})";
        fake->setForecastResponse(true, forecastJson);
        fake->setAirQualityResponse(true, airJson);

        Location loc;
        loc.name = "Stockholm";
        loc.country = "Sweden";
        loc.lat = 59.3293;
        loc.lon = 18.0686;
        storage->saveCurrent(loc);

        Location tokyo;    tokyo.name = "Tokyo";    tokyo.country = "Japan";   tokyo.lat = 35.6762;  tokyo.lon = 139.6503;
        Location paris;    paris.name = "Paris";    paris.country = "France";  paris.lat = 48.8566;  paris.lon = 2.3522;
        Location saopaulo; saopaulo.name = "Sao Paulo"; saopaulo.country = "Brazil"; saopaulo.lat = -23.5505; saopaulo.lon = -46.6333;
        storage->savePins({tokyo, paris, saopaulo});

        Translator translator;
        Theme theme;
        Backend backend(fake, storage);

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("translator", &translator);
        engine.rootContext()->setContextProperty("theme", &theme);
        engine.rootContext()->setContextProperty("backend", &backend);

        engine.load(QUrl("qrc:/Main.qml"));

        QCOMPARE(engine.rootObjects().size(), 1);
        QVERIFY(backend.dashboard()->current().valid);
        QCOMPARE(backend.dashboard()->locationName(), QString("Stockholm, Sweden"));

        // When WEATHERDECK_SNAPSHOT is set, dump a grab of the window for a visual check
        if (qEnvironmentVariableIsSet("WEATHERDECK_SNAPSHOT")) {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (window) {
                // Optional size override to check cramped layouts (WEATHERDECK_WINDOW=WxH)
                if (qEnvironmentVariableIsSet("WEATHERDECK_WINDOW")) {
                    const QRegularExpression rx("^\\s*(\\d+)x(\\d+)\\s*$");
                    const auto m = rx.match(QString::fromLocal8Bit(qgetenv("WEATHERDECK_WINDOW")));
                    if (m.hasMatch() && m.capturedTexts().size() >= 3) {
                        window->setWidth(m.captured(1).toInt());
                        window->setHeight(m.captured(2).toInt());
                    }
                }
                QTest::qWait(300); // Let async image loads and layouts settle
                const QImage shot = window->grabWindow();
                QVERIFY(!shot.isNull());
                shot.save(QString::fromLocal8Bit(qgetenv("WEATHERDECK_SNAPSHOT")));
            }
        }
    }
};

QTEST_MAIN(WeatherdeckTests)
#include "weatherdeck_tests.moc"
