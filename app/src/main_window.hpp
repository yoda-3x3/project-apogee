#pragma once

#include <QMainWindow>

#include "data/database.hpp"
#include "theme_manager.hpp"

class QComboBox;
class QMenu;
class QTabWidget;

namespace apogee::app {

class RocketBuilderPanel;
class PartsBrowserPanel;

// Top-level layout is mode tabs (Design / Launch / Flight), not a flat pile
// of dock widgets -- each phase adds panels within the tab for the mode
// they belong to, so the window stays navigable as more panels arrive
// (Launch gets the map + weather panel in Phase 6, Flight gets telemetry
// charts in Phase 5 and the 3D trajectory view in Phase 7).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onThemeSelected(int index);
    void onImportTheme();

private:
    void buildMenusAndToolbar();
    void refreshThemeCombo();
    void buildTabs();
    QWidget* buildPlaceholderTab(const QString& message);

    ThemeManager themeManager_;
    QComboBox* themeCombo_ = nullptr;
    QMenu* viewMenu_ = nullptr;

    data::Database db_;
    QTabWidget* tabs_ = nullptr;
    RocketBuilderPanel* rocketBuilderPanel_ = nullptr;
    PartsBrowserPanel* partsBrowserPanel_ = nullptr;
};

}  // namespace apogee::app
