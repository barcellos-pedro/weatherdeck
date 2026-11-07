#include <QtTest>
#include "backend.h"
class WeatherdeckTests : public QObject { Q_OBJECT
private slots:
    void parsesForecast() { const QByteArray json=R"({"current":{"temperature_2m":21.4,"relative_humidity_2m":63,"apparent_temperature":20,"precipitation":0,"weather_code":1,"wind_speed_10m":12,"wind_direction_10m":120},"current_units":{"temperature_2m":"°C","wind_speed_10m":"km/h"},"hourly":{"time":["2026-01-01T00:00"],"temperature_2m":[21],"precipitation_probability":[5],"weather_code":[0],"relative_humidity_2m":[60],"uv_index":[0]},"daily":{"time":["2026-01-01"],"weather_code":[0],"temperature_2m_max":[25],"temperature_2m_min":[17],"precipitation_probability_max":[10],"uv_index_max":[4]}})"; QVariantMap weather; QVariantList hourly,daily; QString error; QVERIFY(Backend::parseForecast(json,&weather,&hourly,&daily,&error)); QCOMPARE(weather["temp"].toDouble(),21.4); QCOMPARE(hourly.size(),1); QCOMPARE(daily.size(),1); }
    void rejectsInvalidResponse() { QVariantMap weather; QVariantList hourly,daily; QString error; QVERIFY(!Backend::parseForecast("{}",&weather,&hourly,&daily,&error)); QVERIFY(!error.isEmpty()); }
};
QTEST_MAIN(WeatherdeckTests)
#include "weatherdeck_tests.moc"
