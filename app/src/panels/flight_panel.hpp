#pragma once

#include <QWidget>

#include "core/telemetry.hpp"

class QSqlDatabase;
class QDoubleSpinBox;
class QPushButton;
class QLabel;

namespace apogee::app {

class RocketDesign;
class LaunchSite;
class ChartWidget;
class SimulationWorker;

// The Flight tab: an ejection-delay input, a "Fly" button that runs the
// current Design-tab rocket through core::Simulation on a background
// thread using the current Launch-tab site/rail/wind (LaunchSite, shared
// with LaunchSitePanel), three ChartWidgets (altitude, velocity,
// acceleration-G) with phase markers, and a summary-stats readout.
class FlightPanel : public QWidget {
    Q_OBJECT
public:
    FlightPanel(QSqlDatabase& db, RocketDesign& design, LaunchSite& launchSite, QWidget* parent = nullptr);
    ~FlightPanel() override;

signals:
    // Emitted after a flight, so LaunchSitePanel can drop a landing marker
    // on the map. ENU meters, matching core::SummaryStats::landingOffsetM.
    void flightCompleted(double landingOffsetEastM, double landingOffsetNorthM);

private slots:
    void onFlyClicked();
    void onSimulationFinished();
    void refreshLaunchSiteSummary();

private:
    void buildUi();
    void updateSummary(const core::SummaryStats& stats);

    QSqlDatabase& db_;
    RocketDesign& design_;
    LaunchSite& launchSite_;
    SimulationWorker* worker_ = nullptr;

    QLabel* launchSiteSummaryLabel_ = nullptr;
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
