QT += core gui quick quickcontrols2 network testlib
CONFIG += c++17 testcase
TARGET = weatherdeck_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/backend.h \
           ../src/location.h ../src/dashboard_model.h ../src/dashboard_state.h \
           ../src/weather_acquisition.h ../src/dashboard_snapshot.h \
           ../src/saved_locations.h
SOURCES += weatherdeck_tests.cpp ../src/backend.cpp \
           ../src/dashboard_model.cpp ../src/dashboard_state.cpp \
           ../src/weather_acquisition.cpp ../src/dashboard_snapshot.cpp \
           ../src/saved_locations.cpp
