#include <QQmlApplicationEngine>
#include <QQmlContext>
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
        QCOMPARE(current.condition, QString("Cloudy"));
        QCOMPARE(current.glyph, QString("☁"));
        QCOMPARE(current.tempUnit, QString("°C"));
        QCOMPARE(current.windUnit, QString("km/h"));

        QCOMPARE(hourly.size(), 2);
        QCOMPARE(hourly[0].time, QString("00:00"));
        QCOMPARE(hourly[0].temp, 21.0);
        QCOMPARE(hourly[0].glyph, QString("☀"));
        QCOMPARE(hourly[1].time, QString("01:00"));
        QCOMPARE(hourly[1].glyph, QString("☁"));

        QCOMPARE(daily.size(), 1);
        QCOMPARE(daily[0].date, QString("2026-01-01"));
        QCOMPARE(daily[0].max, 25.0);
        QCOMPARE(daily[0].min, 17.0);
        QCOMPARE(daily[0].rain, 10.0);
        QCOMPARE(daily[0].uv, 4.0);
        QCOMPARE(daily[0].glyph, QString("☀"));
        QCOMPARE(daily[0].condition, QString("Clear"));
    }

    void mapsWeatherCodesAndGlyphs() {
        QCOMPARE(DashboardModel::conditionForCode(0), QString("Clear"));
        QCOMPARE(DashboardModel::glyphForCode(0), QString("☀"));

        QCOMPARE(DashboardModel::conditionForCode(2), QString("Cloudy"));
        QCOMPARE(DashboardModel::glyphForCode(2), QString("☁"));

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

    void offscreenQmlLoad() {
        auto *fake = new FakeWeatherAcquisition();
        auto *storage = new InMemoryLocationStorage();

        const QByteArray forecastJson = R"({
            "current": {"temperature_2m": 21.0, "apparent_temperature": 20.0, "relative_humidity_2m": 60, "precipitation": 0.0, "wind_speed_10m": 10.0, "weather_code": 0},
            "current_units": {"temperature_2m": "°C", "wind_speed_10m": "km/h"},
            "hourly": {"time": ["2026-01-01T00:00"], "weather_code": [0], "temperature_2m": [21.0], "precipitation_probability": [0], "relative_humidity_2m": [60], "uv_index": [0]},
            "daily": {"time": ["2026-01-01"], "weather_code": [0], "temperature_2m_max": [25.0], "temperature_2m_min": [15.0], "precipitation_probability_max": [0], "uv_index_max": [2]}
        })";
        const QByteArray airJson = R"({"current": {"european_aqi": 30.0, "pm2_5": 7.5}})";
        fake->setForecastResponse(true, forecastJson);
        fake->setAirQualityResponse(true, airJson);

        Location loc;
        loc.name = "Stockholm";
        loc.country = "Sweden";
        loc.lat = 59.32;
        loc.lon = 18.06;
        storage->saveCurrent(loc);

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
    }
};

QTEST_MAIN(WeatherdeckTests)
#include "weatherdeck_tests.moc"
