#pragma once

#include <QVector>
#include <QWidget>

#include "data/thrustcurve_types.hpp"

class QLabel;
class QLineEdit;
class QPushButton;
class QSqlDatabase;
class QTableView;
class QStandardItemModel;

namespace apogee::app {

// Two read-only/reference tables: every seeded component (browse-only --
// components come from the embedded seed catalog, nothing to fetch), and a
// live ThrustCurve.org motor search with a "Cache Motor" action that
// downloads the motor's thrust samples and stores them locally via
// MotorRepository, so RocketBuilderPanel's motor combo can pick it up.
class PartsBrowserPanel : public QWidget {
    Q_OBJECT
public:
    explicit PartsBrowserPanel(QSqlDatabase& db, QWidget* parent = nullptr);

signals:
    void motorsCached();

private slots:
    void onSearchClicked();
    void onCacheMotorClicked();

private:
    void buildUi();
    void reloadComponentsTable();

    QSqlDatabase& db_;

    QTableView* componentsTable_ = nullptr;
    QStandardItemModel* componentsModel_ = nullptr;

    QLineEdit* manufacturerEdit_ = nullptr;
    QLineEdit* designationEdit_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QTableView* motorResultsTable_ = nullptr;
    QStandardItemModel* motorResultsModel_ = nullptr;
    QPushButton* cacheMotorButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QVector<data::MotorSummary> lastSearchResults_;
};

}  // namespace apogee::app
