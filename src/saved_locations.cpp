#include "saved_locations.h"
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

// QSettingsLocationStorage
QSettingsLocationStorage::QSettingsLocationStorage(const QString &organization, const QString &application)
    : m_settings(organization, application) {
}

Location QSettingsLocationStorage::loadCurrent() {
    const auto val = m_settings.value("current");
    if (!val.isValid()) return Location{};
    return Location::fromVariantMap(val.toMap());
}

void QSettingsLocationStorage::saveCurrent(const Location &location) {
    m_settings.setValue("current", location.toVariantMap());
}

QList<Location> QSettingsLocationStorage::loadPins() {
    QList<Location> pins;
    const auto list = m_settings.value("pins").toList();
    for (const auto &v : list) {
        const auto loc = Location::fromVariantMap(v.toMap());
        if (loc.isValid()) pins.append(loc);
    }
    return pins;
}

void QSettingsLocationStorage::savePins(const QList<Location> &pins) {
    QVariantList list;
    for (const auto &loc : pins) {
        list.append(loc.toVariantMap());
    }
    m_settings.setValue("pins", list);
}

bool QSettingsLocationStorage::loadFahrenheit(bool defaultValue) {
    return m_settings.value("fahrenheit", defaultValue).toBool();
}

void QSettingsLocationStorage::saveFahrenheit(bool fahrenheit) {
    m_settings.setValue("fahrenheit", fahrenheit);
}

// SavedLocations
SavedLocations::SavedLocations(LocationStorage *storage, QObject *parent)
    : QObject(parent), m_storage(storage) {
}

void SavedLocations::restore() {
    if (!m_storage) return;

    m_fahrenheit = m_storage->loadFahrenheit(false);
    emit fahrenheitChanged(m_fahrenheit);

    m_pins = m_storage->loadPins();
    deduplicatePins();
    emit pinsChanged();

    const auto saved = m_storage->loadCurrent();
    if (saved.isValid()) {
        m_current = saved;
        emit currentChanged(m_current);
    }
}

void SavedLocations::setCurrent(const Location &location, bool persist) {
    m_current = location;
    if (persist && m_storage) {
        m_storage->saveCurrent(m_current);
    }
    emit currentChanged(m_current);
}

bool SavedLocations::isPinned(const Location &location) const {
    if (!location.isValid()) return false;
    for (const auto &pin : m_pins) {
        if (pin.isSameLocation(location)) return true;
    }
    return false;
}

void SavedLocations::addPin(const Location &location) {
    if (!location.isValid()) return;
    if (isPinned(location)) return; // Deduplicated
    m_pins.append(location);
    if (m_storage) m_storage->savePins(m_pins);
    emit pinsChanged();
}

void SavedLocations::removePin(const Location &location) {
    if (!location.isValid()) return;
    for (int i = 0; i < m_pins.size(); ++i) {
        if (m_pins[i].isSameLocation(location)) {
            m_pins.removeAt(i);
            if (m_storage) m_storage->savePins(m_pins);
            emit pinsChanged();
            return;
        }
    }
}

void SavedLocations::togglePin(const Location &location) {
    if (!location.isValid()) return;
    if (isPinned(location)) {
        removePin(location);
    } else {
        addPin(location);
    }
}

void SavedLocations::toggleCurrentPin() {
    if (m_current.isValid()) {
        togglePin(m_current);
    }
}

void SavedLocations::cyclePin(int direction) {
    if (m_pins.isEmpty()) return;

    int index = -1;
    for (int i = 0; i < m_pins.size(); ++i) {
        if (m_pins[i].isSameLocation(m_current)) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        index = (direction >= 0) ? 0 : m_pins.size() - 1;
    } else {
        index = (index + direction + m_pins.size()) % m_pins.size();
    }

    setCurrent(m_pins[index], true);
}

void SavedLocations::setFahrenheit(bool fahrenheit) {
    if (m_fahrenheit != fahrenheit) {
        m_fahrenheit = fahrenheit;
        if (m_storage) m_storage->saveFahrenheit(m_fahrenheit);
        emit fahrenheitChanged(m_fahrenheit);
    }
}

void SavedLocations::deduplicatePins() {
    QList<Location> unique;
    QSet<QString> seenKeys;
    for (const auto &loc : m_pins) {
        const auto key = loc.normalizedKey();
        if (!seenKeys.contains(key)) {
            seenKeys.insert(key);
            unique.append(loc);
        }
    }
    m_pins = unique;
}
