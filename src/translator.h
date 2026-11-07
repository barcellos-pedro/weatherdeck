#pragma once
#include <QObject>
#include <QTranslator>
class Translator : public QTranslator {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY changed)
public:
    explicit Translator(QObject *parent = nullptr);
    QString language() const { return m_language; }
signals: void changed();
private: QString m_language = "en";
};
