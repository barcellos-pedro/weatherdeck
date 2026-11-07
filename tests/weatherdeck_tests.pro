QT += core gui quick quickcontrols2 network testlib
CONFIG += c++17 testcase
TARGET = weatherdeck_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/backend.h
SOURCES += weatherdeck_tests.cpp ../src/backend.cpp
