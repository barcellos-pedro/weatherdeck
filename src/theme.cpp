#include "theme.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>

Theme::Theme(QObject *parent) : QObject(parent) {
    m_debounce.setSingleShot(true); m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    reload();
}
void Theme::reload() {
    const QString base = QDir::homePath() + "/.local/state/omarchy/current";
    const QString colors = base + "/theme/colors.toml";
    const auto old = m_accent;
    QFile file(colors);
    if (file.open(QIODevice::ReadOnly)) {
        const auto match = QRegularExpression(R"(^\s*accent\s*=\s*[\"'](#[0-9a-fA-F]{6})[\"'])", QRegularExpression::MultilineOption)
            .match(QString::fromUtf8(file.readAll()));
        if (match.hasMatch()) m_accent = match.captured(1);
    }
    const auto watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    if (QFile::exists(colors)) m_watcher.addPath(colors);
    if (QFile::exists(base + "/theme")) m_watcher.addPath(base + "/theme");
    if (old != m_accent) emit changed();
}
