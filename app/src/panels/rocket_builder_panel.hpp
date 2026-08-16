#pragma once

#include <QVector>
#include <QWidget>

#include "data/component_types.hpp"
#include "data/thrustcurve_types.hpp"
#include "models/rocket_design.hpp"

class QComboBox;
class QLabel;
class QPushButton;
class QSqlDatabase;

namespace apogee::app {

class RocketDiagramWidget;

// Lets the user assemble a rocket from the seeded parts catalog: one combo
// box per slot (nose cone / body tube / fin set / motor mount / recovery /
// motor), a "Load Kit" convenience action that fills all five component
// slots from a whole seeded kit in one click, and a live CG/CP/stability
// readout + 2D diagram that recompute on every change -- no simulation
// involved, so this updates instantly.
class RocketBuilderPanel : public QWidget {
    Q_OBJECT
public:
    explicit RocketBuilderPanel(QSqlDatabase& db, QWidget* parent = nullptr);

    RocketDesign& design() { return design_; }

public slots:
    // Re-queries the DB for components/kits/motors and repopulates every
    // combo box -- call after PartsBrowserPanel caches a new motor.
    void reloadFromDatabase();

private slots:
    void onLoadKitClicked();
    void onSlotComboChanged();
    void refreshReadout();

private:
    void buildUi();
    void populateComponentCombo(QComboBox* combo, const QVector<data::ComponentWithDetail>& items);

    QSqlDatabase& db_;
    RocketDesign design_;

    QComboBox* kitCombo_ = nullptr;
    QComboBox* noseCombo_ = nullptr;
    QComboBox* bodyTubeCombo_ = nullptr;
    QComboBox* finSetCombo_ = nullptr;
    QComboBox* motorMountCombo_ = nullptr;
    QComboBox* recoveryCombo_ = nullptr;
    QComboBox* motorCombo_ = nullptr;

    QLabel* massLabel_ = nullptr;
    QLabel* cgLabel_ = nullptr;
    QLabel* cpLabel_ = nullptr;
    QLabel* marginLabel_ = nullptr;

    RocketDiagramWidget* diagram_ = nullptr;

    QVector<data::ComponentWithDetail> noseCones_;
    QVector<data::ComponentWithDetail> bodyTubes_;
    QVector<data::ComponentWithDetail> finSets_;
    QVector<data::ComponentWithDetail> motorMounts_;
    QVector<data::ComponentWithDetail> recoveries_;  // parachutes + streamers
    QVector<data::KitSummary> kits_;
    QVector<data::MotorSummary> motors_;
};

}  // namespace apogee::app
