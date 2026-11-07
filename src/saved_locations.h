#pragma once
#include <QList>
#include <QObject>
#include <QSettings>
#include "location.h"

class LocationStorage {
public:
    virtual ~LocationStorage() = default;
    virtual Location loadCurrent() = 0;
    virtual void saveCurrent(const Location &location) = 0;
    virtual QList<Location> loadPins() = 0;
    virtual void savePins(const QList<Location> &pins) = 0;
    virtual bool loadFahrenheit(bool defaultValue = false) = 0;
    virtual void saveFahrenheit(bool fahrenheit) = 0;
};

class QSettingsLocationStorage : public LocationStorage {
public:
    explicit QSettingsLocationStorage(const QString &organization = "omarchy",
                                     const QString &application = "weatherdeck");
    ~QSettingsLocationStorage() override = default;

    Location loadCurrent() override;
    void saveCurrent(const Location &location) override;
    QList<Location> loadPins() override;
    void savePins(const QList<Location> &pins) override;
    bool loadFahrenheit(bool defaultValue = false) override;
    void saveFahrenheit(bool fahrenheit) override;

private:
    QSettings m_settings;
};

class InMemoryLocationStorage : public LocationStorage {
public:
    explicit InMemoryLocationStorage() = default;
    ~InMemoryLocationStorage() override = default;

    Location loadCurrent() override { return m_current; }
    void saveCurrent(const Location &location) override { m_current = location; }
    QList<Location> loadPins() override { return m_pins; }
    void savePins(const QList<Location> &pins) override { m_pins = pins; }
    bool loadFahrenheit(bool defaultValue = false) override { return m_hasFahrenheit ? m_fahrenheit : defaultValue; }
    void saveFahrenheit(bool fahrenheit) override { m_fahrenheit = fahrenheit; m_hasFahrenheit = true; }

private:
    Location m_current;
    QList<Location> m_pins;
    bool m_fahrenheit = false;
    bool m_hasFahrenheit = false;
};

class SavedLocations : public QObject {
    Q_OBJECT
public:
    explicit SavedLocations(LocationStorage *storage, QObject *parent = nullptr);
    ~SavedLocations() override = default;

    void restore();

    Location current() const { return m_current; }
    void setCurrent(const Location &location, bool persist = true);

    QList<Location> pins() const { return m_pins; }
    bool isPinned(const Location &location) const;
    bool isCurrentPinned() const { return isPinned(m_current); }

    void addPin(const Location &location);
    void removePin(const Location &location);
    void togglePin(const Location &location);
    void toggleCurrentPin();

    void cyclePin(int direction);

    bool fahrenheit() const { return m_fahrenheit; }
    void setFahrenheit(bool fahrenheit);

signals:
    void currentChanged(const Location &location);
    void pinsChanged();
    void fahrenheitChanged(bool fahrenheit);

private:
    void deduplicatePins();

    LocationStorage *m_storage = nullptr;
    Location m_current;
    QList<Location> m_pins;
    bool m_fahrenheit = false;
};
