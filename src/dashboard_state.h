#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include "dashboard_model.h"
#include "location.h"

struct DashboardSnapshotData {
    Location location;
    QString locationName;
    QString updated;
    QString status;
    bool loading = false;
    bool fahrenheit = false;
    bool pinned = false;
    CurrentConditions current;
    AirQuality airQuality;
    QList<HourlyForecast> hourly;
    QList<DailyForecast> daily;
};

class DashboardState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString locationName READ locationName NOTIFY changed)
    Q_PROPERTY(QString updated READ updated NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool fahrenheit READ fahrenheit NOTIFY changed)
    Q_PROPERTY(bool pinned READ pinned NOTIFY changed)
    Q_PROPERTY(CurrentConditions current READ current NOTIFY changed)
    Q_PROPERTY(AirQuality airQuality READ airQuality NOTIFY changed)
    Q_PROPERTY(QVariantList hourly READ hourly NOTIFY changed)
    Q_PROPERTY(QVariantList daily READ daily NOTIFY changed)

public:
    explicit DashboardState(QObject *parent = nullptr);

    QString locationName() const { return m_locationName; }
    QString updated() const { return m_updated; }
    QString status() const { return m_status; }
    bool loading() const { return m_loading; }
    bool fahrenheit() const { return m_fahrenheit; }
    bool pinned() const { return m_pinned; }
    CurrentConditions current() const { return m_current; }
    AirQuality airQuality() const { return m_airQuality; }
    QVariantList hourly() const { return m_hourlyList; }
    QVariantList daily() const { return m_dailyList; }

    void setLocationName(const QString &name);
    void setUpdated(const QString &updated);
    void setStatus(const QString &status);
    void setLoading(bool loading);
    void setFahrenheit(bool fahrenheit);
    void setPinned(bool pinned);
    void setCurrent(const CurrentConditions &current);
    void setAirQuality(const AirQuality &airQuality);
    void setHourly(const QList<HourlyForecast> &hourly);
    void setDaily(const QList<DailyForecast> &daily);

    void applySnapshot(const DashboardSnapshotData &snapshot);

signals:
    void changed();

private:
    QString m_locationName;
    QString m_updated;
    QString m_status;
    bool m_loading = false;
    bool m_fahrenheit = false;
    bool m_pinned = false;
    CurrentConditions m_current;
    AirQuality m_airQuality;
    QVariantList m_hourlyList;
    QVariantList m_dailyList;
};
