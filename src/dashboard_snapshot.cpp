#include "dashboard_snapshot.h"
#include <memory>

DashboardSnapshotCoordinator::DashboardSnapshotCoordinator(WeatherAcquisition *acquisition, QObject *parent)
    : QObject(parent), m_acquisition(acquisition) {
}

void DashboardSnapshotCoordinator::refresh(const Location &location, bool fahrenheit) {
    requestSnapshot(location, fahrenheit);
}

void DashboardSnapshotCoordinator::requestSnapshot(const Location &location, bool fahrenheit) {
    if (!m_acquisition) return;
    if (!location.isValid()) return;

    const quint64 gen = ++m_generation;
    emit loadingChanged(true);

    struct GenerationContext {
        bool forecastReceived = false;
        bool forecastSuccess = false;
        CurrentConditions current;
        QList<HourlyForecast> hourly;
        QList<DailyForecast> daily;

        bool airReceived = false;
        AirQuality airQuality;
    };

    auto ctx = std::make_shared<GenerationContext>();

    // 1. Fetch forecast
    m_acquisition->fetchForecast(location, fahrenheit, [this, gen, location, fahrenheit, ctx](bool success, const QByteArray &payload, const QString &netErr) {
        if (gen != m_generation) return; // Discard stale generation

        ctx->forecastReceived = true;
        ctx->forecastSuccess = success;

        if (!success) {
            const QString err = netErr.isEmpty() ? tr("Weather is unavailable. Press r to retry.") : netErr;
            emit snapshotFailed(err);
            emit loadingChanged(false);
            return;
        }

        QString parseErr;
        if (!DashboardModel::parseForecastPayload(payload, &ctx->current, &ctx->hourly, &ctx->daily, &parseErr)) {
            emit snapshotFailed(parseErr.isEmpty() ? tr("Weather returned an invalid response.") : parseErr);
            emit loadingChanged(false);
            return;
        }

        DashboardSnapshotData data;
        data.location = location;
        data.locationName = location.displayName();
        data.updated = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
        data.updatedAt = QDateTime::currentSecsSinceEpoch();
        data.generation = gen;
        data.loading = false;
        data.fahrenheit = fahrenheit;
        data.current = ctx->current;
        data.hourly = ctx->hourly;
        data.daily = ctx->daily;
        data.airQuality = ctx->airQuality; // might be available if air arrived first, or unavailable

        emit snapshotReady(data);
        emit loadingChanged(false);
    });

    // 2. Fetch air quality
    m_acquisition->fetchAirQuality(location, [this, gen, location, fahrenheit, ctx](bool success, const QByteArray &payload, const QString &) {
        if (gen != m_generation) return; // Discard stale generation

        ctx->airReceived = true;
        if (success) {
            QString parseErr;
            DashboardModel::parseAirQualityPayload(payload, &ctx->airQuality, &parseErr);
        } else {
            ctx->airQuality.available = false;
        }

        // If forecast was already successfully received, emit updated snapshot
        if (ctx->forecastReceived && ctx->forecastSuccess) {
            DashboardSnapshotData data;
            data.location = location;
            data.locationName = location.displayName();
            data.updated = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
            data.updatedAt = QDateTime::currentSecsSinceEpoch();
            data.generation = gen;
            data.loading = false;
            data.fahrenheit = fahrenheit;
            data.current = ctx->current;
            data.hourly = ctx->hourly;
            data.daily = ctx->daily;
            data.airQuality = ctx->airQuality;

            emit snapshotReady(data);
        }
    });
}
