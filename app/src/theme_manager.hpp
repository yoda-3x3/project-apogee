#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace apogee::app {

struct ThemeInfo {
    QString id;           // stable key, e.g. "dark" or a user theme's filename stem
    QString displayName;  // shown in the theme combo box
    QString path;         // ":/themes/dark.qss" or a user AppData path
    bool builtIn = false;
};

// Discovers, applies, and imports .qss theme files. Bundled themes live as
// Qt resources (:/themes/*.qss); user-imported themes are copied into
// QStandardPaths::AppDataLocation/themes so a new theme can be added by
// dropping in a .qss file, no rebuild required.
class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QObject* parent = nullptr);

    QVector<ThemeInfo> discoverThemes() const;

    // Returns the theme id that was actually applied ("" on failure).
    QString applyTheme(const QString& id);

    // Copies sourceQssPath into the user themes folder. Returns the new
    // theme's id ("" on failure, e.g. unreadable file).
    QString importTheme(const QString& sourceQssPath);

    QString currentThemeId() const { return currentThemeId_; }

    // Restores the last-applied theme from QSettings, falling back to "dark".
    void restoreLastTheme();

private:
    QString userThemesDir() const;
    QString currentThemeId_;
};

}  // namespace apogee::app
