#include "panels/flight_panel.hpp"

#include <utility>
#include <vector>

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QSqlDatabase>
#include <QVBoxLayout>

#include "core/motor_model.hpp"
#include "core/simulation.hpp"
#include "core/wind_field.hpp"
#include "data/motor_repository.hpp"
#include "models/rocket_design.hpp"
#include "widgets/chart_widget.hpp"
#include "workers/simulation_worker.hpp"

namespace apogee::app {

namespace {
QString formatBool(bool value) { return value ? "Yes" : "No"; }
}  // namespace

FlightPanel::FlightPanel(QSqlDatabase& db, RocketDesign& design, QWidget* parent)
    : QWidget(parent), db_(db), design_(design) {
    buildUi();

    worker_ = new SimulationWorker(this);
    connect(worker_, &SimulationWorker::finished, this, &FlightPanel::onSimulationFinished);
}

FlightPanel::~FlightPanel() {
    if (worker_) worker_->wait();
}

void FlightPanel::buildUi() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* leftColumn = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftColumn);

    auto* launchGroup = new QGroupBox("Launch Conditions", leftColumn);
    auto* launchForm = new QFormLayout(launchGroup);

    railLengthSpin_ = new QDoubleSpinBox(launchGroup);
    railLengthSpin_->setRange(0.3, 10.0);
    railLengthSpin_->setSingleStep(0.1);
    railLengthSpin_->setSuffix(" m");
    railLengthSpin_->setValue(1.0);
    launchForm->addRow("Rail length:", railLengthSpin_);

    railAngleSpin_ = new QDoubleSpinBox(launchGroup);
    railAngleSpin_->setRange(0.0, 45.0);
    railAngleSpin_->setSingleStep(1.0);
    railAngleSpin_->setSuffix(" deg from vertical");
    launchForm->addRow("Rail angle:", railAngleSpin_);

    windSpeedSpin_ = new QDoubleSpinBox(launchGroup);
    windSpeedSpin_->setRange(0.0, 30.0);
    windSpeedSpin_->setSingleStep(0.5);
    windSpeedSpin_->setSuffix(" m/s");
    launchForm->addRow("Ground wind speed:", windSpeedSpin_);

    windDirectionSpin_ = new QDoubleSpinBox(launchGroup);
    windDirectionSpin_->setRange(0.0, 359.0);
    windDirectionSpin_->setSingleStep(15.0);
    windDirectionSpin_->setSuffix(" deg (from north)");
    launchForm->addRow("Wind direction:", windDirectionSpin_);

    ejectionDelaySpin_ = new QDoubleSpinBox(launchGroup);
    ejectionDelaySpin_->setRange(0.0, 15.0);
    ejectionDelaySpin_->setSingleStep(0.5);
    ejectionDelaySpin_->setSuffix(" s");
    ejectionDelaySpin_->setValue(3.0);
    launchForm->addRow("Ejection delay:", ejectionDelaySpin_);

    leftLayout->addWidget(launchGroup);

    flyButton_ = new QPushButton("Fly", leftColumn);
    connect(flyButton_, &QPushButton::clicked, this, &FlightPanel::onFlyClicked);
    leftLayout->addWidget(flyButton_);

    statusLabel_ = new QLabel(leftColumn);
    statusLabel_->setWordWrap(true);
    leftLayout->addWidget(statusLabel_);

    auto* summaryGroup = new QGroupBox("Summary", leftColumn);
    auto* summaryForm = new QFormLayout(summaryGroup);
    apogeeValue_ = new QLabel("-", summaryGroup);
    apogeeTimeValue_ = new QLabel("-", summaryGroup);
    maxVelocityValue_ = new QLabel("-", summaryGroup);
    maxMachValue_ = new QLabel("-", summaryGroup);
    maxGValue_ = new QLabel("-", summaryGroup);
    burnoutValue_ = new QLabel("-", summaryGroup);
    flightDurationValue_ = new QLabel("-", summaryGroup);
    driftValue_ = new QLabel("-", summaryGroup);
    minStabilityValue_ = new QLabel("-", summaryGroup);
    railExitValue_ = new QLabel("-", summaryGroup);
    summaryForm->addRow("Apogee:", apogeeValue_);
    summaryForm->addRow("Apogee time:", apogeeTimeValue_);
    summaryForm->addRow("Max velocity:", maxVelocityValue_);
    summaryForm->addRow("Max Mach:", maxMachValue_);
    summaryForm->addRow("Max acceleration:", maxGValue_);
    summaryForm->addRow("Burnout (alt / vel):", burnoutValue_);
    summaryForm->addRow("Flight duration:", flightDurationValue_);
    summaryForm->addRow("Drift distance:", driftValue_);
    summaryForm->addRow("Min stability margin:", minStabilityValue_);
    summaryForm->addRow("Stable off rail:", railExitValue_);
    leftLayout->addWidget(summaryGroup);

    leftLayout->addStretch(1);

    auto* rightColumn = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightColumn);
    altitudeChart_ = new ChartWidget("Altitude AGL", "m", rightColumn, "No flight data yet -- click Fly");
    velocityChart_ = new ChartWidget("Velocity", "m/s", rightColumn, "No flight data yet -- click Fly");
    accelChart_ = new ChartWidget("Acceleration", "G", rightColumn, "No flight data yet -- click Fly");
    rightLayout->addWidget(altitudeChart_, 1);
    rightLayout->addWidget(velocityChart_, 1);
    rightLayout->addWidget(accelChart_, 1);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftColumn);
    splitter->addWidget(rightColumn);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);
}

void FlightPanel::onFlyClicked() {
    if (worker_->isRunning()) return;

    const StabilityInfo info = design_.computeStability();
    if (!info.hasMinimumParts) {
        statusLabel_->setText("Select at least a nose cone, body tube, and fin set in the Design tab first.");
        return;
    }
    if (!design_.motor()) {
        statusLabel_->setText("Select a motor in the Design tab first.");
        return;
    }

    data::MotorRepository motorRepo(db_);
    const QVector<data::ThrustSample> cachedSamples = motorRepo.getCachedSamples(design_.motor()->id);
    if (cachedSamples.isEmpty()) {
        statusLabel_->setText(
            "No cached thrust curve for this motor -- cache it in the Parts Browser tab first.");
        return;
    }

    std::vector<core::ThrustSample> samples;
    samples.reserve(static_cast<std::size_t>(cachedSamples.size()));
    for (const data::ThrustSample& s : cachedSamples) samples.push_back({s.timeS, s.thrustN});

    const double propellantMassKg = design_.motor()->propWeightG / 1000.0;
    const double casingMassKg =
        (design_.motor()->totalWeightG - design_.motor()->propWeightG) / 1000.0;
    core::MotorModel motor(std::move(samples), propellantMassKg, casingMassKg);

    core::LaunchConditions launch;
    launch.railLengthM = railLengthSpin_->value();
    launch.railAngleFromVerticalDeg = railAngleSpin_->value();
    launch.wind = core::WindField::powerLawShear(windSpeedSpin_->value(), windDirectionSpin_->value());

    core::SimulationConfig config;
    config.ejectionDelayS = ejectionDelaySpin_->value();

    flyButton_->setEnabled(false);
    statusLabel_->setText("Simulating...");

    worker_->setInputs(info.definition, std::move(motor), launch, config);
    worker_->start();
}

void FlightPanel::onSimulationFinished() {
    const core::Telemetry& telemetry = worker_->result();

    flyButton_->setEnabled(true);
    statusLabel_->setText(QString("Flight complete: %1 samples recorded.").arg(telemetry.samples.size()));

    QVector<QPointF> altitudePoints, velocityPoints, gPoints;
    QVector<PhaseMarker> markers;
    altitudePoints.reserve(static_cast<int>(telemetry.samples.size()));
    velocityPoints.reserve(static_cast<int>(telemetry.samples.size()));
    gPoints.reserve(static_cast<int>(telemetry.samples.size()));

    core::FlightPhase lastPhase = core::FlightPhase::OnRail;
    bool first = true;
    for (const core::TelemetrySample& s : telemetry.samples) {
        altitudePoints.push_back(QPointF(s.timeS, s.position.z));
        velocityPoints.push_back(QPointF(s.timeS, s.velocity.norm()));
        gPoints.push_back(QPointF(s.timeS, s.gForce));
        if (first || s.phase != lastPhase) {
            markers.push_back({s.timeS, s.phase});
            lastPhase = s.phase;
            first = false;
        }
    }

    altitudeChart_->setData(altitudePoints, markers);
    velocityChart_->setData(velocityPoints, markers);
    accelChart_->setData(gPoints, markers);

    updateSummary(telemetry.summary);
}

void FlightPanel::updateSummary(const core::SummaryStats& stats) {
    apogeeValue_->setText(QString("%1 m").arg(stats.apogeeM, 0, 'f', 1));
    apogeeTimeValue_->setText(QString("%1 s").arg(stats.apogeeTimeS, 0, 'f', 2));
    maxVelocityValue_->setText(QString("%1 m/s").arg(stats.maxVelocityMs, 0, 'f', 1));
    maxMachValue_->setText(QString::number(stats.maxMachNumber, 'f', 2));
    maxGValue_->setText(QString("%1 G").arg(stats.maxAccelerationG, 0, 'f', 1));
    burnoutValue_->setText(QString("%1 m / %2 m/s")
                                .arg(stats.burnoutAltitudeM, 0, 'f', 1)
                                .arg(stats.burnoutVelocityMs, 0, 'f', 1));
    flightDurationValue_->setText(QString("%1 s").arg(stats.flightDurationS, 0, 'f', 1));
    driftValue_->setText(QString("%1 m").arg(stats.driftDistanceM, 0, 'f', 1));

    minStabilityValue_->setText(QString("%1 calibers").arg(stats.minStabilityMarginCalibers, 0, 'f', 2));
    QString marginColor = "#1e8449";
    if (stats.minStabilityMarginCalibers < 0.0) {
        marginColor = "#c0392b";
    } else if (stats.minStabilityMarginCalibers < 1.0) {
        marginColor = "#d68910";
    }
    minStabilityValue_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(marginColor));

    railExitValue_->setText(formatBool(stats.railExitStable));
    railExitValue_->setStyleSheet(QString("color: %1; font-weight: bold;")
                                       .arg(stats.railExitStable ? "#1e8449" : "#c0392b"));
}

}  // namespace apogee::app
