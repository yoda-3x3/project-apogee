#pragma once

#include <QMainWindow>

#include "theme_manager.hpp"

class QComboBox;

namespace apogee::app {

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

    ThemeManager themeManager_;
    QComboBox* themeCombo_ = nullptr;
};

}  // namespace apogee::app
