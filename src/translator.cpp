#include "translator.h"
#include <QDir>
#include <QFile>
#include <QLocale>
Translator::Translator(QObject *parent) : QTranslator(parent) {
    QFile language(QDir::homePath() + "/.config/omarchy/language");
    if (language.open(QIODevice::ReadOnly)) {
        const QString candidate = QString::fromUtf8(language.readLine()).trimmed();
        if (!candidate.isEmpty()) m_language = candidate;
    }
    QLocale::setDefault(QLocale(m_language));
}
