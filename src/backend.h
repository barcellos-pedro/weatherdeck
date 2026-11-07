#pragma once
#include <QList>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include "dashboard_snapshot.h"
#include "dashboard_state.h"
#include "location.h"
#include "saved_locations.h"
#include "weather_acquisition.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(DashboardState* dashboard READ dashboard CONSTANT)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    Backend(WeatherAcquisition *acquisition, LocationStorage *storage, QObject *parent = nullptr);
    ~Backend() override;

    DashboardState* dashboard() { return &m_dashboardState; }
    QVariantList searchResults() const { return m_searchResults; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void selectSearchResult(int index);
    Q_INVOKABLE void toggleUnits();
    Q_INVOKABLE void togglePin();
    Q_INVOKABLE void cyclePin(int direction);

signals:
    void searchResultsChanged();

private:
    void setupConnections();
    void locate();

    bool m_ownsAcquisition = false;
    bool m_ownsStorage = false;
    WeatherAcquisition *m_acquisition = nullptr;
    LocationStorage *m_storage = nullptr;

    DashboardState m_dashboardState;
    SavedLocations m_savedLocations;
    DashboardSnapshotCoordinator m_snapshotCoordinator;

    QTimer m_refreshTimer;
    QTimer m_searchTimer;

    QList<Location> m_rawSearchResults;
    QVariantList m_searchResults;
};
