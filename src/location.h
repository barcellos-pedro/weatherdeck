#pragma once
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

struct Location {
    QString name;
    QString country;
    QString admin;
    double lat = 0.0;
    double lon = 0.0;
    QString timezone;

    QString normalizedKey() const {
        return QString::asprintf("%.4f,%.4f", lat, lon);
    }

    bool isSameLocation(const Location &other) const {
        return normalizedKey() == other.normalizedKey();
    }

    bool isValid() const {
        return !name.trimmed().isEmpty() && (lat != 0.0 || lon != 0.0);
    }

    QString displayName() const {
        if (country.trimmed().isEmpty()) return name;
        return name + ", " + country;
    }

    QJsonObject toJson() const {
        return {
            {"name", name},
            {"country", country},
            {"admin", admin},
            {"lat", lat},
            {"lon", lon},
            {"timezone", timezone}
        };
    }

    static Location fromJson(const QJsonObject &obj) {
        Location loc;
        loc.name = obj["name"].toString();
        loc.country = obj["country"].toString();
        loc.admin = obj["admin"].toString();
        if (loc.admin.isEmpty() && obj.contains("admin1")) {
            loc.admin = obj["admin1"].toString();
        }
        loc.lat = obj["lat"].toDouble(obj["latitude"].toDouble());
        loc.lon = obj["lon"].toDouble(obj["longitude"].toDouble());
        loc.timezone = obj["timezone"].toString();
        return loc;
    }

    QVariantMap toVariantMap() const {
        return {
            {"name", name},
            {"country", country},
            {"admin", admin},
            {"lat", lat},
            {"lon", lon},
            {"timezone", timezone}
        };
    }

    static Location fromVariantMap(const QVariantMap &map) {
        Location loc;
        loc.name = map["name"].toString();
        loc.country = map["country"].toString();
        loc.admin = map["admin"].toString();
        loc.lat = map["lat"].toDouble();
        loc.lon = map["lon"].toDouble();
        loc.timezone = map["timezone"].toString();
        return loc;
    }
};

inline bool operator==(const Location &a, const Location &b) {
    return a.isSameLocation(b);
}
inline bool operator!=(const Location &a, const Location &b) {
    return !(a == b);
}
