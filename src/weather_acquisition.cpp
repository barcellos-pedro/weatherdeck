#include "weather_acquisition.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

// QtWeatherAcquisition
QtWeatherAcquisition::QtWeatherAcquisition(QObject *parent)
    : WeatherAcquisition(parent) {
}

void QtWeatherAcquisition::fetchForecast(const Location &location, bool fahrenheit, ForecastCallback callback) {
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery q;
    q.addQueryItem("latitude", QString::number(location.lat, 'f', 6));
    q.addQueryItem("longitude", QString::number(location.lon, 'f', 6));
    q.addQueryItem("timezone", "auto");
    q.addQueryItem("forecast_days", "7");
    q.addQueryItem("temperature_unit", fahrenheit ? "fahrenheit" : "celsius");
    q.addQueryItem("wind_speed_unit", fahrenheit ? "mph" : "kmh");
    q.addQueryItem("current", "temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,wind_direction_10m");
    q.addQueryItem("hourly", "temperature_2m,precipitation_probability,weather_code,relative_humidity_2m,uv_index");
    q.addQueryItem("daily", "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,uv_index_max");
    url.setQuery(q);

    auto *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const bool ok = reply->error() == QNetworkReply::NoError;
        const auto body = reply->readAll();
        const auto err = reply->errorString();
        reply->deleteLater();
        if (callback) callback(ok, body, ok ? QString() : err);
    });
}

void QtWeatherAcquisition::fetchAirQuality(const Location &location, AirQualityCallback callback) {
    QUrl url("https://air-quality-api.open-meteo.com/v1/air-quality");
    QUrlQuery q;
    q.addQueryItem("latitude", QString::number(location.lat, 'f', 6));
    q.addQueryItem("longitude", QString::number(location.lon, 'f', 6));
    q.addQueryItem("current", "european_aqi,pm2_5");
    url.setQuery(q);

    auto *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const bool ok = reply->error() == QNetworkReply::NoError;
        const auto body = reply->readAll();
        const auto err = reply->errorString();
        reply->deleteLater();
        if (callback) callback(ok, body, ok ? QString() : err);
    });
}

void QtWeatherAcquisition::locate(LocateCallback callback) {
    auto *reply = m_network.get(QNetworkRequest(QUrl("https://ipwho.is/")));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const auto body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!ok) {
            if (callback) callback(false, Location{}, tr("Unable to locate approximate position."));
            return;
        }
        const auto o = QJsonDocument::fromJson(body).object();
        if (!o["success"].toBool()) {
            if (callback) callback(false, Location{}, tr("Choose a city to get started."));
            return;
        }
        Location loc;
        loc.name = o["city"].toString();
        loc.country = o["country"].toString();
        loc.lat = o["latitude"].toDouble();
        loc.lon = o["longitude"].toDouble();
        loc.timezone = o["timezone"].toObject()["id"].toString();
        if (callback) callback(true, loc, QString());
    });
}

void QtWeatherAcquisition::search(const QString &query, SearchCallback callback) {
    if (query.size() < 2) {
        if (callback) callback(true, {}, QString());
        return;
    }
    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    QUrlQuery q;
    q.addQueryItem("name", query);
    q.addQueryItem("count", "8");
    q.addQueryItem("language", QLocale().name().left(2));
    url.setQuery(q);

    auto *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const auto body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!ok) {
            if (callback) callback(false, {}, tr("Search request failed."));
            return;
        }
        QList<Location> results;
        const auto root = QJsonDocument::fromJson(body).object();
        for (const auto &v : root["results"].toArray()) {
            results.append(Location::fromJson(v.toObject()));
        }
        if (callback) callback(true, results, QString());
    });
}

// FakeWeatherAcquisition
FakeWeatherAcquisition::FakeWeatherAcquisition(QObject *parent)
    : WeatherAcquisition(parent) {
}

void FakeWeatherAcquisition::setForecastResponse(bool success, const QByteArray &payload, const QString &error) {
    m_forecastSuccess = success;
    m_forecastPayload = payload;
    m_forecastError = error;
}

void FakeWeatherAcquisition::setAirQualityResponse(bool success, const QByteArray &payload, const QString &error) {
    m_airSuccess = success;
    m_airPayload = payload;
    m_airError = error;
}

void FakeWeatherAcquisition::setLocateResponse(bool success, const Location &loc, const QString &error) {
    m_locateSuccess = success;
    m_locateResult = loc;
    m_locateError = error;
}

void FakeWeatherAcquisition::setSearchResponse(bool success, const QList<Location> &results, const QString &error) {
    m_searchSuccess = success;
    m_searchResults = results;
    m_searchError = error;
}

void FakeWeatherAcquisition::setDeferCallbacks(bool defer) {
    m_deferCallbacks = defer;
}

void FakeWeatherAcquisition::triggerPendingForecast() {
    auto pending = m_pendingForecastCallbacks;
    m_pendingForecastCallbacks.clear();
    for (auto &cb : pending) cb();
}

void FakeWeatherAcquisition::triggerNextForecast() {
    if (!m_pendingForecastCallbacks.isEmpty()) {
        auto cb = m_pendingForecastCallbacks.takeFirst();
        cb();
    }
}

void FakeWeatherAcquisition::triggerPendingAirQuality() {
    auto pending = m_pendingAirCallbacks;
    m_pendingAirCallbacks.clear();
    for (auto &cb : pending) cb();
}

void FakeWeatherAcquisition::triggerNextAirQuality() {
    if (!m_pendingAirCallbacks.isEmpty()) {
        auto cb = m_pendingAirCallbacks.takeFirst();
        cb();
    }
}

void FakeWeatherAcquisition::fetchForecast(const Location &, bool, ForecastCallback callback) {
    ++m_forecastCallCount;
    auto execute = [this, callback]() {
        if (callback) callback(m_forecastSuccess, m_forecastPayload, m_forecastError);
    };
    if (m_deferCallbacks) {
        m_pendingForecastCallbacks.append(execute);
    } else {
        execute();
    }
}

void FakeWeatherAcquisition::fetchAirQuality(const Location &, AirQualityCallback callback) {
    ++m_airCallCount;
    auto execute = [this, callback]() {
        if (callback) callback(m_airSuccess, m_airPayload, m_airError);
    };
    if (m_deferCallbacks) {
        m_pendingAirCallbacks.append(execute);
    } else {
        execute();
    }
}

void FakeWeatherAcquisition::locate(LocateCallback callback) {
    if (callback) callback(m_locateSuccess, m_locateResult, m_locateError);
}

void FakeWeatherAcquisition::search(const QString &, SearchCallback callback) {
    ++m_searchCallCount;
    if (callback) callback(m_searchSuccess, m_searchResults, m_searchError);
}
