#include "backend.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

namespace { QString condition(int code) {
    if (code == 0) return QObject::tr("Clear");
    if (code <= 3) return QObject::tr("Cloudy");
    if (code <= 49) return QObject::tr("Fog");
    if (code <= 57) return QObject::tr("Drizzle");
    if (code <= 67) return QObject::tr("Rain");
    if (code <= 77) return QObject::tr("Snow");
    if (code <= 82) return QObject::tr("Showers");
    return QObject::tr("Thunderstorm"); }
QString glyph(int code) { if (code == 0) return "☀"; if (code <= 3) return "☁"; if (code <= 49) return "≋"; if (code <= 67) return "☂"; if (code <= 77) return "❄"; return "⚡"; }
QVariantMap placeFrom(const QJsonObject &o) { return {{"name", o["name"].toString()}, {"country", o["country"].toString()}, {"admin", o["admin1"].toString()}, {"lat", o["latitude"].toDouble()}, {"lon", o["longitude"].toDouble()}, {"timezone", o["timezone"].toString()}}; }
}

Backend::Backend(QObject *parent) : QObject(parent), m_settings("omarchy", "weatherdeck") {
    m_fahrenheit = m_settings.value("fahrenheit", false).toBool(); loadPins();
    m_refreshTimer.setInterval(10 * 60 * 1000); connect(&m_refreshTimer, &QTimer::timeout, this, &Backend::refresh); m_refreshTimer.start();
    m_searchTimer.setSingleShot(true); m_searchTimer.setInterval(180);
    connect(&m_searchTimer, &QTimer::timeout, this, [this] { const auto query = m_searchTimer.property("query").toString();
        if (query.size() < 2) { m_searchResults.clear(); emit searchResultsChanged(); return; }
        QUrl url("https://geocoding-api.open-meteo.com/v1/search"); QUrlQuery q; q.addQueryItem("name", query); q.addQueryItem("count", "8"); q.addQueryItem("language", QLocale().name().left(2)); url.setQuery(q);
        auto *reply = m_network.get(QNetworkRequest(url)); connect(reply, &QNetworkReply::finished, this, [this, reply] { QVariantList results; const auto root = QJsonDocument::fromJson(reply->readAll()).object(); for (const auto &v : root["results"].toArray()) results << placeFrom(v.toObject()); reply->deleteLater(); m_searchResults = results; emit searchResultsChanged(); }); });
    const auto saved = m_settings.value("current").toMap(); if (!saved.isEmpty()) fetchFor(saved, false); else locate();
}
bool Backend::pinned() const { for (const auto &p : m_pins) if (p.toMap()["lat"] == m_current["lat"] && p.toMap()["lon"] == m_current["lon"]) return true; return false; }
void Backend::loadPins() { m_pins = m_settings.value("pins").toList(); }
void Backend::saveCurrent() { m_settings.setValue("current", m_current); m_settings.setValue("pins", m_pins); }
void Backend::locate() { m_status = tr("Finding your approximate location…"); emit changed(); auto *reply = m_network.get(QNetworkRequest(QUrl("https://ipwho.is/"))); connect(reply, &QNetworkReply::finished, this, [this, reply] { const auto o = QJsonDocument::fromJson(reply->readAll()).object(); reply->deleteLater(); if (!o["success"].toBool()) { m_status = tr("Choose a city to get started."); emit changed(); return; } fetchFor({{"name", o["city"].toString()}, {"country", o["country"].toString()}, {"lat", o["latitude"].toDouble()}, {"lon", o["longitude"].toDouble()}, {"timezone", o["timezone"].toObject()["id"].toString()}}); }); }
void Backend::refresh() { if (!m_current.isEmpty()) fetchFor(m_current, false); }
void Backend::fetchFor(const QVariantMap &place, bool persist) { m_current = place; if (persist) saveCurrent(); m_locationName = place["name"].toString() + (place["country"].toString().isEmpty() ? "" : ", " + place["country"].toString()); m_loading = true; m_status.clear(); emit changed();
    QUrl url("https://api.open-meteo.com/v1/forecast"); QUrlQuery q; q.addQueryItem("latitude", place["lat"].toString()); q.addQueryItem("longitude", place["lon"].toString()); q.addQueryItem("timezone", "auto"); q.addQueryItem("forecast_days", "7"); q.addQueryItem("temperature_unit", m_fahrenheit ? "fahrenheit" : "celsius"); q.addQueryItem("wind_speed_unit", m_fahrenheit ? "mph" : "kmh"); q.addQueryItem("current", "temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,wind_direction_10m"); q.addQueryItem("hourly", "temperature_2m,precipitation_probability,weather_code,relative_humidity_2m,uv_index"); q.addQueryItem("daily", "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,uv_index_max"); url.setQuery(q);
    auto *reply = m_network.get(QNetworkRequest(url)); connect(reply, &QNetworkReply::finished, this, [this, reply] { QString error; QVariantMap weather; QVariantList hourly, daily; const bool ok = reply->error() == QNetworkReply::NoError && parseForecast(reply->readAll(), &weather, &hourly, &daily, &error); reply->deleteLater(); m_loading = false; if (!ok) { m_status = error.isEmpty() ? tr("Weather is unavailable. Press r to retry.") : error; emit changed(); return; } m_weather=weather; m_hourly=hourly; m_daily=daily; m_updated=QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat); m_status.clear(); emit changed(); fetchAir(); }); }
void Backend::fetchAir() { QUrl url("https://air-quality-api.open-meteo.com/v1/air-quality"); QUrlQuery q; q.addQueryItem("latitude", m_current["lat"].toString()); q.addQueryItem("longitude", m_current["lon"].toString()); q.addQueryItem("current", "european_aqi,pm2_5"); url.setQuery(q); auto *reply=m_network.get(QNetworkRequest(url)); connect(reply,&QNetworkReply::finished,this,[this,reply] { const auto c=QJsonDocument::fromJson(reply->readAll()).object()["current"].toObject(); reply->deleteLater(); if (!c.isEmpty()) { m_weather["aqi"]=c["european_aqi"].toDouble(); m_weather["pm25"]=c["pm2_5"].toDouble(); emit changed(); } }); }
void Backend::search(const QString &query) { m_searchTimer.setProperty("query", query); m_searchTimer.start(); }
void Backend::selectSearchResult(int index) { if (index >= 0 && index < m_searchResults.size()) { fetchFor(m_searchResults[index].toMap()); m_searchResults.clear(); emit searchResultsChanged(); } }
void Backend::toggleUnits() { m_fahrenheit=!m_fahrenheit; m_settings.setValue("fahrenheit",m_fahrenheit); refresh(); emit changed(); }
void Backend::togglePin() { if (m_current.isEmpty()) return; for(int i=0;i<m_pins.size();++i) if(m_pins[i].toMap()["lat"]==m_current["lat"] && m_pins[i].toMap()["lon"]==m_current["lon"]) { m_pins.removeAt(i); saveCurrent(); emit pinsChanged(); return; } m_pins << m_current; saveCurrent(); emit pinsChanged(); }
void Backend::cyclePin(int direction) { if(m_pins.isEmpty()) return; int index=0; for(int i=0;i<m_pins.size();++i) if(m_pins[i].toMap()["lat"]==m_current["lat"]&&m_pins[i].toMap()["lon"]==m_current["lon"]) {index=i;break;} index=(index+direction+m_pins.size())%m_pins.size(); fetchFor(m_pins[index].toMap()); }
bool Backend::parseForecast(const QByteArray &body, QVariantMap *weather, QVariantList *hourly, QVariantList *daily, QString *error) {
    const auto root = QJsonDocument::fromJson(body).object();
    if (root.isEmpty()) { *error = QObject::tr("Weather returned an invalid response."); return false; }
    if (root["error"].toBool()) { *error = root["reason"].toString(); return false; }
    const auto c = root["current"].toObject();
    if (c.isEmpty()) { *error = QObject::tr("Weather returned no current conditions."); return false; }
    const int code = c["weather_code"].toInt();
    *weather = {{"temp", c["temperature_2m"].toDouble()}, {"feels", c["apparent_temperature"].toDouble()}, {"humidity", c["relative_humidity_2m"].toDouble()}, {"rain", c["precipitation"].toDouble()}, {"wind", c["wind_speed_10m"].toDouble()}, {"direction", c["wind_direction_10m"].toDouble()}, {"code", code}, {"condition", condition(code)}, {"glyph", glyph(code)}, {"tempUnit", root["current_units"].toObject()["temperature_2m"].toString()}, {"windUnit", root["current_units"].toObject()["wind_speed_10m"].toString()}};
    const auto h = root["hourly"].toObject();
    const auto times = h["time"].toArray();
    for (int i = 0; i < qMin(24, times.size()); ++i) {
        const int x = h["weather_code"].toArray()[i].toInt();
        hourly->append(QVariantMap{{"time", times[i].toString().mid(11, 5)}, {"temp", h["temperature_2m"].toArray()[i].toDouble()}, {"rain", h["precipitation_probability"].toArray()[i].toDouble()}, {"humidity", h["relative_humidity_2m"].toArray()[i].toDouble()}, {"uv", h["uv_index"].toArray()[i].toDouble()}, {"glyph", glyph(x)}});
    }
    const auto d = root["daily"].toObject();
    const auto dates = d["time"].toArray();
    for (int i = 0; i < dates.size(); ++i) {
        const int x = d["weather_code"].toArray()[i].toInt();
        daily->append(QVariantMap{{"date", dates[i].toString()}, {"max", d["temperature_2m_max"].toArray()[i].toDouble()}, {"min", d["temperature_2m_min"].toArray()[i].toDouble()}, {"rain", d["precipitation_probability_max"].toArray()[i].toDouble()}, {"uv", d["uv_index_max"].toArray()[i].toDouble()}, {"glyph", glyph(x)}, {"condition", condition(x)}});
    }
    return true;
}
