#include "theme.h"
#include <QDir>
#include <QFile>
#include <QHash>
#include <QRegularExpression>

Theme::Theme(QObject *parent) : QObject(parent) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    reload();
}

void Theme::reload() {
    const QString base = QDir::homePath() + "/.local/state/omarchy/current";
    QHash<QString, QString> colors;
    QFile file(base + "/theme/colors.toml");
    if (file.open(QIODevice::ReadOnly)) {
        const auto text = QString::fromUtf8(file.readAll());
        const QRegularExpression line(R"(^\s*([a-z_]+)\s*=\s*["'](#[0-9a-fA-F]{6})["'])",
                                      QRegularExpression::MultilineOption);
        auto it = line.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            colors.insert(match.captured(1), match.captured(2));
        }
    }

    auto take = [&colors](const QString &key, const QColor &fallback) {
        const auto value = colors.value(key);
        return value.startsWith('#') ? QColor(value) : fallback;
    };

    // Remember the previous values to decide whether something changed
    const auto oldAccent = m_accent, oldGood = m_good, oldFair = m_fair, oldWarn = m_warn;
    const auto oldPoor = m_poor, oldBad = m_bad, oldExtreme = m_extreme;
    const auto oldBg = m_bg, oldSurface = m_surface, oldSurfaceAlt = m_surfaceAlt;
    const auto oldBorder = m_border, oldText = m_text, oldTextBright = m_textBright;
    const auto oldTextDim = m_textDim, oldTextFaint = m_textFaint;

    m_accent = take("accent", m_accent);
    m_good = take("green", m_good);
    m_fair = take("cyan", m_fair);
    m_warn = take("yellow", m_warn);
    m_poor = take("orange", m_poor);
    m_bad = take("red", m_bad);
    m_extreme = take("magenta", m_extreme);
    m_bg = take("darker_background", m_bg);
    m_surface = take("dark_background", m_surface);
    m_surfaceAlt = take("background", m_surfaceAlt);
    m_border = take("selection", m_border);
    m_text = take("foreground", m_text);
    m_textBright = take("bright_foreground", m_textBright);
    m_textDim = take("light_foreground", m_textDim);
    m_textFaint = take("dark_foreground", m_textFaint);

    const auto watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    if (QFile::exists(base + "/theme/colors.toml")) m_watcher.addPath(base + "/theme/colors.toml");
    if (QFile::exists(base + "/theme")) m_watcher.addPath(base + "/theme");

    // Emit a single changed() signal only when a visible token actually moved
    if (oldAccent != m_accent || oldGood != m_good || oldFair != m_fair ||
        oldWarn != m_warn || oldPoor != m_poor || oldBad != m_bad ||
        oldExtreme != m_extreme || oldBg != m_bg || oldSurface != m_surface ||
        oldSurfaceAlt != m_surfaceAlt || oldBorder != m_border || oldText != m_text ||
        oldTextBright != m_textBright || oldTextDim != m_textDim ||
        oldTextFaint != m_textFaint) {
        emit changed();
    }
}
