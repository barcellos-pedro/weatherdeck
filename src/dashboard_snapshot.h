#pragma once
#include <QDateTime>
#include <QLocale>
#include <QObject>
#include <QString>
#include "dashboard_model.h"
#include "dashboard_state.h"
#include "location.h"
#include "weather_acquisition.h"

class DashboardSnapshotCoordinator : public QObject {
    Q_OBJECT
public:
    explicit DashboardSnapshotCoordinator(WeatherAcquisition *acquisition, QObject *parent = nullptr);
    ~DashboardSnapshotCoordinator() override = default;

    void requestSnapshot(const Location &location, bool fahrenheit);
    void refresh(const Location &location, bool fahrenheit);
    quint64 generation() const { return m_generation; }

signals:
    void snapshotReady(const DashboardSnapshotData &snapshot);
    void snapshotFailed(const QString &error);
    void loadingChanged(bool loading);

private:
    WeatherAcquisition *m_acquisition = nullptr;
    quint64 m_generation = 0;
};
