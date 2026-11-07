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
    qint64 updatedAt = 0;       // Epoch seconds for relative "N m ago" labels
    quint64 generation = 0;
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
    Q_PROPERTY(QString coordinates READ coordinates NOTIFY changed)
    Q_PROPERTY(QString updated READ updated NOTIFY changed)
    Q_PROPERTY(qint64 updatedAt READ updatedAt NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool fahrenheit READ fahrenheit NOTIFY changed)
    Q_PROPERTY(bool pinned READ pinned NOTIFY changed)
    Q_PROPERTY(quint64 generation READ generation NOTIFY changed)
    Q_PROPERTY(QVariantList pins READ pins NOTIFY changed)
    Q_PROPERTY(int activePinIndex READ activePinIndex NOTIFY changed)
    Q_PROPERTY(CurrentConditions current READ current NOTIFY changed)
    Q_PROPERTY(AirQuality airQuality READ airQuality NOTIFY changed)
    Q_PROPERTY(QVariantList hourly READ hourly NOTIFY changed)
    Q_PROPERTY(QVariantList daily READ daily NOTIFY changed)

public:
    explicit DashboardState(QObject *parent = nullptr);

    QString locationName() const { return m_locationName; }
    QString coordinates() const { return m_coordinates; }
    QString updated() const { return m_updated; }
    qint64 updatedAt() const { return m_updatedAt; }
    QString status() const { return m_status; }
    bool loading() const { return m_loading; }
    bool fahrenheit() const { return m_fahrenheit; }
    bool pinned() const { return m_pinned; }
    quint64 generation() const { return m_generation; }
    QVariantList pins() const { return m_pins; }
    int activePinIndex() const { return m_activePinIndex; }
    CurrentConditions current() const { return m_current; }
    AirQuality airQuality() const { return m_airQuality; }
    QVariantList hourly() const { return m_hourlyList; }
    QVariantList daily() const { return m_dailyList; }

    void setLocationName(const QString &name);
    void setCoordinates(const QString &coordinates);
    void setUpdated(const QString &updated);
    void setUpdatedAt(qint64 epochSeconds);
    void setStatus(const QString &status);
    void setLoading(bool loading);
    void setFahrenheit(bool fahrenheit);
    void setPinned(bool pinned);
    void setGeneration(quint64 generation);
    void setPins(const QVariantList &pins);
    void setActivePinIndex(int index);
    void setCurrent(const CurrentConditions &current);
    void setAirQuality(const AirQuality &airQuality);
    void setHourly(const QList<HourlyForecast> &hourly);
    void setDaily(const QList<DailyForecast> &daily);

    void applySnapshot(const DashboardSnapshotData &snapshot);

signals:
    void changed();

private:
    QString m_locationName;
    QString m_coordinates;
    QString m_updated;
    qint64 m_updatedAt = 0;
    QString m_status;
    bool m_loading = false;
    bool m_fahrenheit = false;
    bool m_pinned = false;
    quint64 m_generation = 0;
    QVariantList m_pins;
    int m_activePinIndex = -1;
    CurrentConditions m_current;
    AirQuality m_airQuality;
    QVariantList m_hourlyList;
    QVariantList m_dailyList;
};
