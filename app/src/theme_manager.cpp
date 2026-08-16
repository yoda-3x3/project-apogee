#include "theme_manager.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace apogee::app {

namespace {
constexpr const char* kSettingsOrg = "ProjectApogee";
constexpr const char* kSettingsApp = "ApogeeStudio";
constexpr const char* kThemeSettingsKey = "ui/themeId";
constexpr const char* kDefaultThemeId = "classic";
}  // namespace

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {}

QString ThemeManager::userThemesDir() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + "/themes";
}

QVector<ThemeInfo> ThemeManager::discoverThemes() const {
    QVector<ThemeInfo> themes;

    // Bundled themes: fixed, known set embedded via themes.qrc.
    themes.push_back({"dark", "Dark", ":/themes/dark.qss", true});
    themes.push_back({"light", "Light", ":/themes/light.qss", true});
    themes.push_back({"high_contrast", "High Contrast", ":/themes/high_contrast.qss", true});
    themes.push_back({"classic", "Classic", ":/themes/classic.qss", true});

    // User-imported themes: any *.qss dropped into the AppData themes folder.
    QDir dir(userThemesDir());
    if (dir.exists()) {
        const auto entries = dir.entryInfoList({"*.qss"}, QDir::Files, QDir::Name);
        for (const QFileInfo& fi : entries) {
            const QString id = fi.baseName();
            themes.push_back({id, fi.baseName(), fi.absoluteFilePath(), false});
        }
    }

    return themes;
}

QString ThemeManager::applyTheme(const QString& id) {
    const auto themes = discoverThemes();
    for (const ThemeInfo& theme : themes) {
        if (theme.id != id) continue;

        QFile file(theme.path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString styleSheet = QString::fromUtf8(file.readAll());
        if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
            app->setStyleSheet(styleSheet);
        }
        currentThemeId_ = id;

        QSettings settings(kSettingsOrg, kSettingsApp);
        settings.setValue(kThemeSettingsKey, id);
        return id;
    }
    return {};
}

QString ThemeManager::importTheme(const QString& sourceQssPath) {
    QFileInfo src(sourceQssPath);
    if (!src.exists() || src.suffix().toLower() != "qss") {
        return {};
    }

    QDir dir(userThemesDir());
    if (!dir.exists() && !dir.mkpath(".")) {
        return {};
    }

    const QString destPath = dir.filePath(src.fileName());
    QFile::remove(destPath);  // re-importing the same name should overwrite, not fail
    if (!QFile::copy(src.absoluteFilePath(), destPath)) {
        return {};
    }

    return QFileInfo(destPath).baseName();
}

void ThemeManager::restoreLastTheme() {
    QSettings settings(kSettingsOrg, kSettingsApp);
    const QString savedId = settings.value(kThemeSettingsKey, kDefaultThemeId).toString();
    if (applyTheme(savedId).isEmpty()) {
        applyTheme(kDefaultThemeId);
    }
}

}  // namespace apogee::app
