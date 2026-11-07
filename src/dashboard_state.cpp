#include "dashboard_state.h"

DashboardState::DashboardState(QObject *parent) : QObject(parent) {
}

void DashboardState::setLocationName(const QString &name) {
    if (m_locationName != name) {
        m_locationName = name;
        emit changed();
    }
}

void DashboardState::setUpdated(const QString &updated) {
    if (m_updated != updated) {
        m_updated = updated;
        emit changed();
    }
}

void DashboardState::setStatus(const QString &status) {
    if (m_status != status) {
        m_status = status;
        emit changed();
    }
}

void DashboardState::setLoading(bool loading) {
    if (m_loading != loading) {
        m_loading = loading;
        emit changed();
    }
}

void DashboardState::setFahrenheit(bool fahrenheit) {
    if (m_fahrenheit != fahrenheit) {
        m_fahrenheit = fahrenheit;
        emit changed();
    }
}

void DashboardState::setPinned(bool pinned) {
    if (m_pinned != pinned) {
        m_pinned = pinned;
        emit changed();
    }
}

void DashboardState::setCurrent(const CurrentConditions &current) {
    m_current = current;
    emit changed();
}

void DashboardState::setAirQuality(const AirQuality &airQuality) {
    m_airQuality = airQuality;
    emit changed();
}

void DashboardState::setHourly(const QList<HourlyForecast> &hourly) {
    m_hourlyList.clear();
    for (const auto &item : hourly) {
        m_hourlyList.append(QVariant::fromValue(item));
    }
    emit changed();
}

void DashboardState::setDaily(const QList<DailyForecast> &daily) {
    m_dailyList.clear();
    for (const auto &item : daily) {
        m_dailyList.append(QVariant::fromValue(item));
    }
    emit changed();
}

void DashboardState::applySnapshot(const DashboardSnapshotData &snapshot) {
    m_locationName = snapshot.locationName;
    m_updated = snapshot.updated;
    m_status = snapshot.status;
    m_loading = snapshot.loading;
    m_fahrenheit = snapshot.fahrenheit;
    m_pinned = snapshot.pinned;
    m_current = snapshot.current;
    m_airQuality = snapshot.airQuality;

    m_hourlyList.clear();
    for (const auto &item : snapshot.hourly) {
        m_hourlyList.append(QVariant::fromValue(item));
    }
    m_dailyList.clear();
    for (const auto &item : snapshot.daily) {
        m_dailyList.append(QVariant::fromValue(item));
    }
    emit changed();
}
