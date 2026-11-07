QT += core gui qml quick quickcontrols2 network dbus

CONFIG += c++17 release
TARGET = weatherdeck
TEMPLATE = app

HEADERS += src/backend.h src/theme.h src/translator.h \
           src/location.h src/dashboard_model.h src/dashboard_state.h \
           src/weather_acquisition.h src/dashboard_snapshot.h
SOURCES += src/main.cpp src/backend.cpp src/theme.cpp src/translator.cpp \
           src/dashboard_model.cpp src/dashboard_state.cpp \
           src/weather_acquisition.cpp src/dashboard_snapshot.cpp
RESOURCES += src/resources.qrc
