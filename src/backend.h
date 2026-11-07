#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap weather READ weather NOTIFY changed)
    Q_PROPERTY(QVariantList hourly READ hourly NOTIFY changed)
    Q_PROPERTY(QVariantList daily READ daily NOTIFY changed)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantList pins READ pins NOTIFY pinsChanged)
    Q_PROPERTY(QString locationName READ locationName NOTIFY changed)
    Q_PROPERTY(QString updated READ updated NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool fahrenheit READ fahrenheit NOTIFY changed)
    Q_PROPERTY(bool pinned READ pinned NOTIFY pinsChanged)
public:
    explicit Backend(QObject *parent = nullptr);
    QVariantMap weather() const { return m_weather; }
    QVariantList hourly() const { return m_hourly; }
    QVariantList daily() const { return m_daily; }
    QVariantList searchResults() const { return m_searchResults; }
    QVariantList pins() const { return m_pins; }
    QString locationName() const { return m_locationName; }
    QString updated() const { return m_updated; }
    QString status() const { return m_status; }
    bool loading() const { return m_loading; }
    bool fahrenheit() const { return m_fahrenheit; }
    bool pinned() const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void selectSearchResult(int index);
    Q_INVOKABLE void toggleUnits();
    Q_INVOKABLE void togglePin();
    Q_INVOKABLE void cyclePin(int direction);
    static bool parseForecast(const QByteArray &body, QVariantMap *weather, QVariantList *hourly, QVariantList *daily, QString *error);
signals:
    void changed(); void searchResultsChanged(); void pinsChanged();
private:
    void fetchFor(const QVariantMap &place, bool persist = true);
    void fetchAir(); void locate(); void saveCurrent(); void loadPins();
    QNetworkAccessManager m_network;
    QSettings m_settings;
    QTimer m_refreshTimer, m_searchTimer;
    QVariantMap m_current, m_weather;
    QVariantList m_hourly, m_daily, m_searchResults, m_pins;
    QString m_locationName, m_updated, m_status;
    bool m_loading = false, m_fahrenheit = false;
};
