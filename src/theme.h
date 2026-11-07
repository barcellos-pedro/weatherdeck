#pragma once
#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

// Presentation tokens derived from the Omarchy system theme
// (~/.local/state/omarchy/current/theme/colors.toml), hot-reloaded.
class Theme : public QObject {
    Q_OBJECT
    // Accents
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor good READ good NOTIFY changed)
    Q_PROPERTY(QColor fair READ fair NOTIFY changed)
    Q_PROPERTY(QColor warn READ warn NOTIFY changed)
    Q_PROPERTY(QColor poor READ poor NOTIFY changed)
    Q_PROPERTY(QColor bad READ bad NOTIFY changed)
    Q_PROPERTY(QColor extreme READ extreme NOTIFY changed)
    // Surfaces
    Q_PROPERTY(QColor bg READ bg NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor surfaceAlt READ surfaceAlt NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    // Text tones
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor textBright READ textBright NOTIFY changed)
    Q_PROPERTY(QColor textDim READ textDim NOTIFY changed)
    Q_PROPERTY(QColor textFaint READ textFaint NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
public:
    explicit Theme(QObject *parent = nullptr);

    QColor accent() const { return m_accent; }
    QColor good() const { return m_good; }
    QColor fair() const { return m_fair; }
    QColor warn() const { return m_warn; }
    QColor poor() const { return m_poor; }
    QColor bad() const { return m_bad; }
    QColor extreme() const { return m_extreme; }
    QColor bg() const { return m_bg; }
    QColor surface() const { return m_surface; }
    QColor surfaceAlt() const { return m_surfaceAlt; }
    QColor border() const { return m_border; }
    QColor text() const { return m_text; }
    QColor textBright() const { return m_textBright; }
    QColor textDim() const { return m_textDim; }
    QColor textFaint() const { return m_textFaint; }
    int radius() const { return m_radius; }

signals:
    void changed();

private:
    void reload();

    QColor m_accent = QColor("#d7827e");
    QColor m_good = QColor("#a9b665");
    QColor m_fair = QColor("#89b482");
    QColor m_warn = QColor("#d8a657");
    QColor m_poor = QColor("#e1875c");
    QColor m_bad = QColor("#ea6962");
    QColor m_extreme = QColor("#d3869b");
    QColor m_bg = QColor("#161616");
    QColor m_surface = QColor("#1e1e1e");
    QColor m_surfaceAlt = QColor("#282828");
    QColor m_border = QColor("#504945");
    QColor m_text = QColor("#d4be98");
    QColor m_textBright = QColor("#e4dcc3");
    QColor m_textDim = QColor("#bdae93");
    QColor m_textFaint = QColor("#7c6f64");
    int m_radius = 4;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
