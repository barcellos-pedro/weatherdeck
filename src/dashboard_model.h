#pragma once
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

struct CurrentConditions {
    Q_GADGET
    Q_PROPERTY(double temp MEMBER temp)
    Q_PROPERTY(double feels MEMBER feels)
    Q_PROPERTY(double humidity MEMBER humidity)
    Q_PROPERTY(double rain MEMBER rain)
    Q_PROPERTY(double wind MEMBER wind)
    Q_PROPERTY(double direction MEMBER direction)
    Q_PROPERTY(int code MEMBER code)
    Q_PROPERTY(QString condition MEMBER condition)
    Q_PROPERTY(QString glyph MEMBER glyph)
    Q_PROPERTY(QString tempUnit MEMBER tempUnit)
    Q_PROPERTY(QString windUnit MEMBER windUnit)
    Q_PROPERTY(bool valid MEMBER valid)
public:
    double temp = 0.0;
    double feels = 0.0;
    double humidity = 0.0;
    double rain = 0.0;
    double wind = 0.0;
    double direction = 0.0;
    int code = 0;
    QString condition;
    QString glyph;
    QString tempUnit;
    QString windUnit;
    bool valid = false;
};
Q_DECLARE_METATYPE(CurrentConditions)

struct HourlyForecast {
    Q_GADGET
    Q_PROPERTY(QString time MEMBER time)
    Q_PROPERTY(double temp MEMBER temp)
    Q_PROPERTY(double rain MEMBER rain)
    Q_PROPERTY(double humidity MEMBER humidity)
    Q_PROPERTY(double uv MEMBER uv)
    Q_PROPERTY(QString glyph MEMBER glyph)
public:
    QString time;
    double temp = 0.0;
    double rain = 0.0;
    double humidity = 0.0;
    double uv = 0.0;
    QString glyph;
};
Q_DECLARE_METATYPE(HourlyForecast)

struct DailyForecast {
    Q_GADGET
    Q_PROPERTY(QString date MEMBER date)
    Q_PROPERTY(double max MEMBER max)
    Q_PROPERTY(double min MEMBER min)
    Q_PROPERTY(double rain MEMBER rain)
    Q_PROPERTY(double uv MEMBER uv)
    Q_PROPERTY(QString glyph MEMBER glyph)
    Q_PROPERTY(QString condition MEMBER condition)
public:
    QString date;
    double max = 0.0;
    double min = 0.0;
    double rain = 0.0;
    double uv = 0.0;
    QString glyph;
    QString condition;
};
Q_DECLARE_METATYPE(DailyForecast)

struct AirQuality {
    Q_GADGET
    Q_PROPERTY(bool available MEMBER available)
    Q_PROPERTY(double aqi MEMBER aqi)
    Q_PROPERTY(double pm25 MEMBER pm25)
public:
    bool available = false;
    double aqi = 0.0;
    double pm25 = 0.0;
};
Q_DECLARE_METATYPE(AirQuality)

namespace DashboardModel {
    QString conditionForCode(int code);
    QString glyphForCode(int code);

    bool parseForecastPayload(const QByteArray &body,
                              CurrentConditions *current,
                              QList<HourlyForecast> *hourly,
                              QList<DailyForecast> *daily,
                              QString *error);

    bool parseAirQualityPayload(const QByteArray &body,
                                AirQuality *airQuality,
                                QString *error);
}
