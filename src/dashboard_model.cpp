#include "dashboard_model.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace DashboardModel {

QString conditionForCode(int code) {
    if (code == 0) return QObject::tr("Clear");
    if (code <= 3) return QObject::tr("Cloudy");
    if (code <= 49) return QObject::tr("Fog");
    if (code <= 57) return QObject::tr("Drizzle");
    if (code <= 67) return QObject::tr("Rain");
    if (code <= 77) return QObject::tr("Snow");
    if (code <= 82) return QObject::tr("Showers");
    return QObject::tr("Thunderstorm");
}

QString glyphForCode(int code) {
    if (code == 0) return "☀";
    if (code <= 3) return "☁";
    if (code <= 49) return "≋";
    if (code <= 67) return "☂";
    if (code <= 77) return "❄";
    return "⚡";
}

bool parseForecastPayload(const QByteArray &body,
                          CurrentConditions *current,
                          QList<HourlyForecast> *hourly,
                          QList<DailyForecast> *daily,
                          QString *error) {
    QJsonParseError parseErr;
    const auto doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = QObject::tr("Weather returned an invalid response.");
        return false;
    }
    const auto root = doc.object();
    if (root.isEmpty()) {
        if (error) *error = QObject::tr("Weather returned an invalid response.");
        return false;
    }
    if (root["error"].toBool()) {
        if (error) *error = root["reason"].toString();
        return false;
    }

    const auto c = root["current"].toObject();
    if (c.isEmpty()) {
        if (error) *error = QObject::tr("Weather returned no current conditions.");
        return false;
    }

    if (current) {
        const int code = c["weather_code"].toInt();
        current->code = code;
        current->condition = conditionForCode(code);
        current->glyph = glyphForCode(code);
        current->temp = c["temperature_2m"].toDouble();
        current->feels = c["apparent_temperature"].toDouble();
        current->humidity = c["relative_humidity_2m"].toDouble();
        current->rain = c["precipitation"].toDouble();
        current->wind = c["wind_speed_10m"].toDouble();
        current->direction = c["wind_direction_10m"].toDouble();
        current->tempUnit = root["current_units"].toObject()["temperature_2m"].toString("°C");
        current->windUnit = root["current_units"].toObject()["wind_speed_10m"].toString("km/h");
        current->valid = true;
    }

    if (hourly) {
        hourly->clear();
        const auto h = root["hourly"].toObject();
        const auto times = h["time"].toArray();
        const auto codes = h["weather_code"].toArray();
        const auto temps = h["temperature_2m"].toArray();
        const auto rains = h["precipitation_probability"].toArray();
        const auto hums = h["relative_humidity_2m"].toArray();
        const auto uvs = h["uv_index"].toArray();

        // Validate series alignment across parallel hourly arrays
        qsizetype count = times.size();
        count = std::min({count, codes.size(), temps.size(), rains.size(), hums.size(), uvs.size()});
        count = std::min<qsizetype>(count, 24);

        for (qsizetype i = 0; i < count; ++i) {
            HourlyForecast item;
            item.time = times[i].toString().mid(11, 5);
            item.temp = temps[i].toDouble();
            item.rain = rains[i].toDouble();
            item.humidity = hums[i].toDouble();
            item.uv = uvs[i].toDouble();
            item.glyph = glyphForCode(codes[i].toInt());
            hourly->append(item);
        }
    }

    if (daily) {
        daily->clear();
        const auto d = root["daily"].toObject();
        const auto dates = d["time"].toArray();
        const auto codes = d["weather_code"].toArray();
        const auto maxs = d["temperature_2m_max"].toArray();
        const auto mins = d["temperature_2m_min"].toArray();
        const auto rains = d["precipitation_probability_max"].toArray();
        const auto uvs = d["uv_index_max"].toArray();

        // Validate series alignment across parallel daily arrays
        qsizetype count = dates.size();
        count = std::min({count, codes.size(), maxs.size(), mins.size(), rains.size(), uvs.size()});

        for (qsizetype i = 0; i < count; ++i) {
            DailyForecast item;
            item.date = dates[i].toString();
            item.max = maxs[i].toDouble();
            item.min = mins[i].toDouble();
            item.rain = rains[i].toDouble();
            item.uv = uvs[i].toDouble();
            const int code = codes[i].toInt();
            item.glyph = glyphForCode(code);
            item.condition = conditionForCode(code);
            daily->append(item);
        }
    }

    return true;
}

bool parseAirQualityPayload(const QByteArray &body,
                            AirQuality *airQuality,
                            QString *error) {
    if (!airQuality) return false;
    airQuality->available = false;

    QJsonParseError parseErr;
    const auto doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = QObject::tr("Air quality returned invalid data.");
        return false;
    }
    const auto root = doc.object();
    if (root["error"].toBool()) {
        if (error) *error = root["reason"].toString();
        return false;
    }
    const auto current = root["current"].toObject();
    if (current.isEmpty()) {
        if (error) *error = QObject::tr("Air quality returned no current data.");
        return false;
    }

    airQuality->aqi = current["european_aqi"].toDouble();
    airQuality->pm25 = current["pm2_5"].toDouble();
    airQuality->available = true;
    return true;
}

} // namespace DashboardModel
