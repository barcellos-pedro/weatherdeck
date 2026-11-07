#include "backend.h"

Backend::Backend(QObject *parent)
    : Backend(new QtWeatherAcquisition(), new QSettingsLocationStorage(), parent) {
    m_ownsAcquisition = true;
    m_ownsStorage = true;
}

Backend::Backend(WeatherAcquisition *acquisition, LocationStorage *storage, QObject *parent)
    : QObject(parent),
      m_acquisition(acquisition),
      m_storage(storage),
      m_dashboardState(this),
      m_savedLocations(storage, this),
      m_snapshotCoordinator(acquisition, this) {
    m_savedLocations.restore();
    m_dashboardState.setFahrenheit(m_savedLocations.fahrenheit());

    setupConnections();

    if (m_savedLocations.current().isValid()) {
        m_snapshotCoordinator.requestSnapshot(m_savedLocations.current(), m_savedLocations.fahrenheit());
    } else {
        locate();
    }
}

Backend::~Backend() {
    if (m_ownsAcquisition) delete m_acquisition;
    if (m_ownsStorage) delete m_storage;
}

void Backend::setupConnections() {
    // Snapshot Coordinator -> Dashboard State
    connect(&m_snapshotCoordinator, &DashboardSnapshotCoordinator::snapshotReady, this, [this](const DashboardSnapshotData &data) {
        m_dashboardState.applySnapshot(data);
        m_dashboardState.setPinned(m_savedLocations.isPinned(data.location));
    });

    connect(&m_snapshotCoordinator, &DashboardSnapshotCoordinator::snapshotFailed, this, [this](const QString &err) {
        m_dashboardState.setStatus(err);
    });

    connect(&m_snapshotCoordinator, &DashboardSnapshotCoordinator::loadingChanged, this, [this](bool loading) {
        m_dashboardState.setLoading(loading);
    });

    // Saved Locations -> Snapshot Coordinator & Dashboard State
    connect(&m_savedLocations, &SavedLocations::currentChanged, this, [this](const Location &loc) {
        m_dashboardState.setPinned(m_savedLocations.isCurrentPinned());
        m_snapshotCoordinator.requestSnapshot(loc, m_savedLocations.fahrenheit());
    });

    connect(&m_savedLocations, &SavedLocations::pinsChanged, this, [this]() {
        m_dashboardState.setPinned(m_savedLocations.isCurrentPinned());
    });

    connect(&m_savedLocations, &SavedLocations::fahrenheitChanged, this, [this](bool fahrenheit) {
        m_dashboardState.setFahrenheit(fahrenheit);
        if (m_savedLocations.current().isValid()) {
            m_snapshotCoordinator.requestSnapshot(m_savedLocations.current(), fahrenheit);
        }
    });

    // Refresh timer: every 10 minutes
    m_refreshTimer.setInterval(10 * 60 * 1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Backend::refresh);
    m_refreshTimer.start();

    // Debounced search timer: 180ms
    m_searchTimer.setSingleShot(true);
    m_searchTimer.setInterval(180);
    connect(&m_searchTimer, &QTimer::timeout, this, [this]() {
        const QString query = m_searchTimer.property("query").toString();
        if (query.size() < 2) {
            m_rawSearchResults.clear();
            m_searchResults.clear();
            emit searchResultsChanged();
            return;
        }

        m_acquisition->search(query, [this](bool ok, const QList<Location> &results, const QString &) {
            if (!ok) return;
            m_rawSearchResults = results;
            m_searchResults.clear();
            for (const auto &loc : results) {
                m_searchResults.append(loc.toVariantMap());
            }
            emit searchResultsChanged();
        });
    });
}

void Backend::locate() {
    m_dashboardState.setStatus(tr("Finding your approximate location…"));
    m_acquisition->locate([this](bool ok, const Location &location, const QString &err) {
        if (!ok || !location.isValid()) {
            m_dashboardState.setStatus(err.isEmpty() ? tr("Choose a city to get started.") : err);
            return;
        }
        m_savedLocations.setCurrent(location, false);
    });
}

void Backend::refresh() {
    if (m_savedLocations.current().isValid()) {
        m_snapshotCoordinator.refresh(m_savedLocations.current(), m_savedLocations.fahrenheit());
    }
}

void Backend::search(const QString &query) {
    m_searchTimer.setProperty("query", query);
    m_searchTimer.start();
}

void Backend::selectSearchResult(int index) {
    if (index >= 0 && index < m_rawSearchResults.size()) {
        m_savedLocations.setCurrent(m_rawSearchResults[index], true);
        m_rawSearchResults.clear();
        m_searchResults.clear();
        emit searchResultsChanged();
    }
}

void Backend::toggleUnits() {
    m_savedLocations.setFahrenheit(!m_savedLocations.fahrenheit());
}

void Backend::togglePin() {
    m_savedLocations.toggleCurrentPin();
}

void Backend::cyclePin(int direction) {
    m_savedLocations.cyclePin(direction);
}
