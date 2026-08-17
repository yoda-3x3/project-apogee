#pragma once

#include <QVector>
#include <QWidget>

#include "data/thrustcurve_types.hpp"

class QComboBox;
class QLabel;
class QPushButton;
class QSqlDatabase;
class QTableView;
class QStandardItemModel;

namespace apogee::app {

// Two read-only/reference tables: every seeded component (browse-only --
// components come from the embedded seed catalog, nothing to fetch), and a
// live ThrustCurve.org motor search. The motor search is a cascading pair
// of dropdowns -- Manufacturer (from the real ThrustCurve.org metadata
// list), then Model (from a search filtered to that manufacturer) -- rather
// than free-text fields or an always-visible results table, since there
// are only ever a couple hundred motors per manufacturer at most.
class PartsBrowserPanel : public QWidget {
    Q_OBJECT
public:
    explicit PartsBrowserPanel(QSqlDatabase& db, QWidget* parent = nullptr);

signals:
    void motorsCached();

private slots:
    void onManufacturerChanged();
    void onModelChanged();
    void onCacheMotorClicked();

private:
    void buildUi();
    void reloadComponentsTable();
    void loadManufacturers();

    QSqlDatabase& db_;

    QTableView* componentsTable_ = nullptr;
    QStandardItemModel* componentsModel_ = nullptr;

    QComboBox* manufacturerCombo_ = nullptr;
    QComboBox* modelCombo_ = nullptr;
    QPushButton* cacheMotorButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* selectedMotorDetailLabel_ = nullptr;

    QVector<data::MotorSummary> currentModels_;
};

}  // namespace apogee::app
