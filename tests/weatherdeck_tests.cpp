#include <QtTest>
#include "backend.h"
#include "dashboard_model.h"
#include "dashboard_state.h"
#include "location.h"

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
};

QTEST_MAIN(WeatherdeckTests)
#include "weatherdeck_tests.moc"
