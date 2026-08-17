#pragma once

#include <QWidget>

#include "core/telemetry.hpp"

class QSqlDatabase;
class QDoubleSpinBox;
class QPushButton;
class QLabel;

namespace apogee::app {

class RocketDesign;
class ChartWidget;
class SimulationWorker;

// The Flight tab: launch-condition inputs (rail length/angle, ground wind,
// ejection delay -- Phase 6 will replace the wind fields with live
// weather), a "Fly" button that runs the current Design-tab rocket through
// core::Simulation on a background thread, three ChartWidgets (altitude,
// velocity, acceleration-G) with phase markers, and a summary-stats
// readout.
class FlightPanel : public QWidget {
    Q_OBJECT
public:
    FlightPanel(QSqlDatabase& db, RocketDesign& design, QWidget* parent = nullptr);
    ~FlightPanel() override;

private slots:
    void onFlyClicked();
    void onSimulationFinished();

private:
    void buildUi();
    void updateSummary(const core::SummaryStats& stats);

    QSqlDatabase& db_;
    RocketDesign& design_;
    SimulationWorker* worker_ = nullptr;

    QDoubleSpinBox* railLengthSpin_ = nullptr;
    QDoubleSpinBox* railAngleSpin_ = nullptr;
    QDoubleSpinBox* windSpeedSpin_ = nullptr;
    QDoubleSpinBox* windDirectionSpin_ = nullptr;
    QDoubleSpinBox* ejectionDelaySpin_ = nullptr;
    QPushButton* flyButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    ChartWidget* altitudeChart_ = nullptr;
    ChartWidget* velocityChart_ = nullptr;
    ChartWidget* accelChart_ = nullptr;

    QLabel* apogeeValue_ = nullptr;
    QLabel* apogeeTimeValue_ = nullptr;
    QLabel* maxVelocityValue_ = nullptr;
    QLabel* maxMachValue_ = nullptr;
    QLabel* maxGValue_ = nullptr;
    QLabel* burnoutValue_ = nullptr;
    QLabel* flightDurationValue_ = nullptr;
    QLabel* driftValue_ = nullptr;
    QLabel* minStabilityValue_ = nullptr;
    QLabel* railExitValue_ = nullptr;
};

}  // namespace apogee::app
