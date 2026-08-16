#include "main_window.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "core/version.hpp"
#include "data/version.hpp"

namespace apogee::app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Project Apogee");
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    auto* placeholder = new QLabel(
        "Project Apogee -- model rocket flight simulator\n\n"
        "Phase 1 scaffold: rocket builder, parts browser, launch-site map,\n"
        "and telemetry panels arrive in later phases.",
        central);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
    setCentralWidget(central);

    buildMenusAndToolbar();

    statusBar()->showMessage(QString("core %1 | data %2")
                                  .arg(core::version())
                                  .arg(data::version()));

    themeManager_.restoreLastTheme();
    refreshThemeCombo();
}

void MainWindow::buildMenusAndToolbar() {
    auto* viewMenu = menuBar()->addMenu("&View");
    auto* themeMenu = viewMenu->addMenu("&Theme");

    auto* importAction = themeMenu->addAction("&Import Theme...");
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportTheme);

    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->addWidget(new QLabel("Theme:", toolbar));

    themeCombo_ = new QComboBox(toolbar);
    toolbar->addWidget(themeCombo_);
    connect(themeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::onThemeSelected);
}

void MainWindow::refreshThemeCombo() {
    const QSignalBlocker blocker(themeCombo_);
    themeCombo_->clear();

    const auto themes = themeManager_.discoverThemes();
    int selectedIndex = 0;
    for (int i = 0; i < themes.size(); ++i) {
        themeCombo_->addItem(themes[i].displayName, themes[i].id);
        if (themes[i].id == themeManager_.currentThemeId()) {
            selectedIndex = i;
        }
    }
    themeCombo_->setCurrentIndex(selectedIndex);
}

void MainWindow::onThemeSelected(int index) {
    if (index < 0) return;
    const QString id = themeCombo_->itemData(index).toString();
    themeManager_.applyTheme(id);
}

void MainWindow::onImportTheme() {
    const QString path = QFileDialog::getOpenFileName(this, "Import Theme", QString(),
                                                        "Qt Stylesheets (*.qss)");
    if (path.isEmpty()) return;

    const QString newId = themeManager_.importTheme(path);
    if (newId.isEmpty()) {
        QMessageBox::warning(this, "Import Theme", "Could not import that stylesheet.");
        return;
    }

    refreshThemeCombo();
    for (int i = 0; i < themeCombo_->count(); ++i) {
        if (themeCombo_->itemData(i).toString() == newId) {
            themeCombo_->setCurrentIndex(i);
            break;
        }
    }
}

}  // namespace apogee::app
