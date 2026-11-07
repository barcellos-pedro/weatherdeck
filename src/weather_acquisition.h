#pragma once
#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <functional>
#include "location.h"

class WeatherAcquisition : public QObject {
    Q_OBJECT
public:
    using ForecastCallback = std::function<void(bool success, const QByteArray &payload, const QString &error)>;
    using AirQualityCallback = std::function<void(bool success, const QByteArray &payload, const QString &error)>;
    using LocateCallback = std::function<void(bool success, const Location &location, const QString &error)>;
    using SearchCallback = std::function<void(bool success, const QList<Location> &results, const QString &error)>;

    explicit WeatherAcquisition(QObject *parent = nullptr) : QObject(parent) {}
    ~WeatherAcquisition() override = default;

    virtual void fetchForecast(const Location &location, bool fahrenheit, ForecastCallback callback) = 0;
    virtual void fetchAirQuality(const Location &location, AirQualityCallback callback) = 0;
    virtual void locate(LocateCallback callback) = 0;
    virtual void search(const QString &query, SearchCallback callback) = 0;
};

class QtWeatherAcquisition : public WeatherAcquisition {
    Q_OBJECT
public:
    explicit QtWeatherAcquisition(QObject *parent = nullptr);
    ~QtWeatherAcquisition() override = default;

    void fetchForecast(const Location &location, bool fahrenheit, ForecastCallback callback) override;
    void fetchAirQuality(const Location &location, AirQualityCallback callback) override;
    void locate(LocateCallback callback) override;
    void search(const QString &query, SearchCallback callback) override;

private:
    QNetworkAccessManager m_network;
};

class FakeWeatherAcquisition : public WeatherAcquisition {
    Q_OBJECT
public:
    explicit FakeWeatherAcquisition(QObject *parent = nullptr);
    ~FakeWeatherAcquisition() override = default;

    void setForecastResponse(bool success, const QByteArray &payload, const QString &error = QString());
    void setAirQualityResponse(bool success, const QByteArray &payload, const QString &error = QString());
    void setLocateResponse(bool success, const Location &loc, const QString &error = QString());
    void setSearchResponse(bool success, const QList<Location> &results, const QString &error = QString());

    void setDeferCallbacks(bool defer);
    void triggerPendingForecast();
    void triggerNextForecast();
    void triggerPendingAirQuality();
    void triggerNextAirQuality();

    void fetchForecast(const Location &location, bool fahrenheit, ForecastCallback callback) override;
    void fetchAirQuality(const Location &location, AirQualityCallback callback) override;
    void locate(LocateCallback callback) override;
    void search(const QString &query, SearchCallback callback) override;

    int forecastCallCount() const { return m_forecastCallCount; }
    int airCallCount() const { return m_airCallCount; }
    int searchCallCount() const { return m_searchCallCount; }

private:
    bool m_forecastSuccess = true;
    QByteArray m_forecastPayload;
    QString m_forecastError;

    bool m_airSuccess = true;
    QByteArray m_airPayload;
    QString m_airError;

    bool m_locateSuccess = true;
    Location m_locateResult;
    QString m_locateError;

    bool m_searchSuccess = true;
    QList<Location> m_searchResults;
    QString m_searchError;

    bool m_deferCallbacks = false;
    QList<std::function<void()>> m_pendingForecastCallbacks;
    QList<std::function<void()>> m_pendingAirCallbacks;

    int m_forecastCallCount = 0;
    int m_airCallCount = 0;
    int m_searchCallCount = 0;
};
