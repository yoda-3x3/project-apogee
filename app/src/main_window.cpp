#include "main_window.hpp"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "core/version.hpp"
#include "data/seed_loader.hpp"
#include "data/version.hpp"
#include "panels/parts_browser_panel.hpp"
#include "panels/rocket_builder_panel.hpp"

namespace apogee::app {

namespace {
QString databasePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/apogee.sqlite";
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), db_(data::Database::open(databasePath())) {
    setWindowTitle("Project Apogee");
    resize(1280, 800);

    data::seedIfEmpty(db_.handle());

    buildMenusAndToolbar();
    buildTabs();

    statusBar()->showMessage(QString("core %1 | data %2")
                                  .arg(core::version())
                                  .arg(data::version()));

    themeManager_.restoreLastTheme();
    refreshThemeCombo();
}

void MainWindow::buildMenusAndToolbar() {
    viewMenu_ = menuBar()->addMenu("&View");
    auto* themeMenu = viewMenu_->addMenu("&Theme");

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

QWidget* MainWindow::buildPlaceholderTab(const QString& message) {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    auto* label = new QLabel(message, widget);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return widget;
}

void MainWindow::buildTabs() {
    tabs_ = new QTabWidget(this);
    setCentralWidget(tabs_);

    rocketBuilderPanel_ = new RocketBuilderPanel(db_.handle(), this);
    partsBrowserPanel_ = new PartsBrowserPanel(db_.handle(), this);

    auto* designSplitter = new QSplitter(Qt::Horizontal, tabs_);
    designSplitter->addWidget(rocketBuilderPanel_);
    designSplitter->addWidget(partsBrowserPanel_);
    designSplitter->setStretchFactor(0, 1);
    designSplitter->setStretchFactor(1, 1);
    tabs_->addTab(designSplitter, "Design");

    tabs_->addTab(buildPlaceholderTab("Launch site, satellite map, and live weather arrive in Phase 6."),
                  "Launch");
    tabs_->addTab(buildPlaceholderTab("Flight simulation and telemetry charts arrive in Phase 5."),
                  "Flight");

    connect(partsBrowserPanel_, &PartsBrowserPanel::motorsCached, rocketBuilderPanel_,
            &RocketBuilderPanel::reloadFromDatabase);
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
