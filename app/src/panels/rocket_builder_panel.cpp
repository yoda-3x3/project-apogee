#include "panels/rocket_builder_panel.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QVBoxLayout>

#include "data/component_repository.hpp"
#include "data/kit_repository.hpp"
#include "data/motor_repository.hpp"
#include "widgets/rocket_diagram_widget.hpp"

namespace apogee::app {

namespace {
constexpr qint64 kNoneItemData = -1;

QString formatComponentLabel(const data::ComponentSummary& s) {
    return QString("%1 %2").arg(s.manufacturer, s.name);
}

QString formatMotorLabel(const data::MotorSummary& m) {
    return QString("%1 %2 (%3 Ns)").arg(m.manufacturer, m.designation).arg(m.totImpulseNs, 0, 'f', 1);
}
}  // namespace

RocketBuilderPanel::RocketBuilderPanel(QSqlDatabase& db, QWidget* parent) : QWidget(parent), db_(db) {
    buildUi();
    reloadFromDatabase();
    refreshReadout();

    connect(&design_, &RocketDesign::changed, this, &RocketBuilderPanel::refreshReadout);
}

void RocketBuilderPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* kitRow = new QHBoxLayout();
    kitRow->addWidget(new QLabel("Load Kit:", this));
    kitCombo_ = new QComboBox(this);
    kitRow->addWidget(kitCombo_, 1);
    auto* loadKitButton = new QPushButton("Load", this);
    connect(loadKitButton, &QPushButton::clicked, this, &RocketBuilderPanel::onLoadKitClicked);
    kitRow->addWidget(loadKitButton);
    layout->addLayout(kitRow);

    auto* slotsGroup = new QGroupBox("Components", this);
    auto* slotsForm = new QFormLayout(slotsGroup);
    noseCombo_ = new QComboBox(slotsGroup);
    bodyTubeCombo_ = new QComboBox(slotsGroup);
    finSetCombo_ = new QComboBox(slotsGroup);
    motorMountCombo_ = new QComboBox(slotsGroup);
    recoveryCombo_ = new QComboBox(slotsGroup);
    motorCombo_ = new QComboBox(slotsGroup);
    slotsForm->addRow("Nose Cone:", noseCombo_);
    slotsForm->addRow("Body Tube:", bodyTubeCombo_);
    slotsForm->addRow("Fin Set:", finSetCombo_);
    slotsForm->addRow("Motor Mount:", motorMountCombo_);
    slotsForm->addRow("Recovery:", recoveryCombo_);
    slotsForm->addRow("Motor:", motorCombo_);
    layout->addWidget(slotsGroup);

    for (QComboBox* combo :
         {noseCombo_, bodyTubeCombo_, finSetCombo_, motorMountCombo_, recoveryCombo_, motorCombo_}) {
        connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                &RocketBuilderPanel::onSlotComboChanged);
    }

    auto* statsGroup = new QGroupBox("Stability", this);
    auto* statsForm = new QFormLayout(statsGroup);
    massLabel_ = new QLabel("-", statsGroup);
    cgLabel_ = new QLabel("-", statsGroup);
    cpLabel_ = new QLabel("-", statsGroup);
    marginLabel_ = new QLabel("-", statsGroup);
    statsForm->addRow("Loaded mass:", massLabel_);
    statsForm->addRow("CG from nose:", cgLabel_);
    statsForm->addRow("CP from nose:", cpLabel_);
    statsForm->addRow("Stability margin:", marginLabel_);
    layout->addWidget(statsGroup);

    diagram_ = new RocketDiagramWidget(this);
    layout->addWidget(diagram_, 1);
}

void RocketBuilderPanel::reloadFromDatabase() {
    data::ComponentRepository components(db_);
    data::KitRepository kits(db_);
    data::MotorRepository motors(db_);

    noseCones_ = components.listByType("nose_cone");
    bodyTubes_ = components.listByType("body_tube");
    finSets_ = components.listByType("fin_set");
    motorMounts_ = components.listByType("motor_mount");
    recoveries_ = components.listByType("parachute") + components.listByType("streamer");
    kits_ = kits.listAll();
    motors_ = motors.listAll();

    populateComponentCombo(noseCombo_, noseCones_);
    populateComponentCombo(bodyTubeCombo_, bodyTubes_);
    populateComponentCombo(finSetCombo_, finSets_);
    populateComponentCombo(motorMountCombo_, motorMounts_);
    populateComponentCombo(recoveryCombo_, recoveries_);

    const QSignalBlocker kitBlocker(kitCombo_);
    kitCombo_->clear();
    for (const data::KitSummary& kit : kits_) {
        kitCombo_->addItem(QString("%1 %2").arg(kit.manufacturer, kit.name), kit.id);
    }

    const QSignalBlocker motorBlocker(motorCombo_);
    motorCombo_->clear();
    motorCombo_->addItem("(none)", kNoneItemData);
    for (const data::MotorSummary& motor : motors_) {
        motorCombo_->addItem(formatMotorLabel(motor), motor.id);
    }
}

void RocketBuilderPanel::populateComponentCombo(QComboBox* combo,
                                                 const QVector<data::ComponentWithDetail>& items) {
    const QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem("(none)", kNoneItemData);
    for (const data::ComponentWithDetail& item : items) {
        combo->addItem(formatComponentLabel(item.summary), item.summary.id);
    }
}

void RocketBuilderPanel::onSlotComboChanged() {
    auto findById = [](const QVector<data::ComponentWithDetail>& items,
                        qint64 id) -> std::optional<data::ComponentWithDetail> {
        for (const data::ComponentWithDetail& item : items) {
            if (item.summary.id == id) return item;
        }
        return std::nullopt;
    };

    design_.setNoseCone(findById(noseCones_, noseCombo_->currentData().toLongLong()));
    design_.setBodyTube(findById(bodyTubes_, bodyTubeCombo_->currentData().toLongLong()));
    design_.setFinSet(findById(finSets_, finSetCombo_->currentData().toLongLong()));
    design_.setMotorMount(findById(motorMounts_, motorMountCombo_->currentData().toLongLong()));
    design_.setRecovery(findById(recoveries_, recoveryCombo_->currentData().toLongLong()));

    const qint64 motorId = motorCombo_->currentData().toLongLong();
    std::optional<data::MotorSummary> selectedMotor;
    for (const data::MotorSummary& motor : motors_) {
        if (motor.id == motorId) {
            selectedMotor = motor;
            break;
        }
    }
    design_.setMotor(selectedMotor);
}

void RocketBuilderPanel::onLoadKitClicked() {
    const qint64 kitId = kitCombo_->currentData().toLongLong();
    if (kitId <= 0) return;

    data::KitRepository kitRepo(db_);
    data::ComponentRepository componentRepo(db_);

    auto selectInCombo = [](QComboBox* combo, qint64 componentId) {
        const int index = combo->findData(componentId);
        if (index >= 0) combo->setCurrentIndex(index);
    };

    for (const data::KitComponentRef& ref : kitRepo.listComponents(kitId)) {
        const auto component = componentRepo.getById(ref.componentId);
        if (!component) continue;

        if (component->summary.type == "nose_cone") {
            selectInCombo(noseCombo_, ref.componentId);
        } else if (component->summary.type == "body_tube") {
            selectInCombo(bodyTubeCombo_, ref.componentId);
        } else if (component->summary.type == "fin_set") {
            selectInCombo(finSetCombo_, ref.componentId);
        } else if (component->summary.type == "motor_mount") {
            selectInCombo(motorMountCombo_, ref.componentId);
        } else if (component->summary.type == "parachute" || component->summary.type == "streamer") {
            selectInCombo(recoveryCombo_, ref.componentId);
        }
    }
}

void RocketBuilderPanel::refreshReadout() {
    const StabilityInfo info = design_.computeStability();
    diagram_->setStabilityInfo(info);

    if (!info.hasMinimumParts) {
        massLabel_->setText("-");
        cgLabel_->setText("-");
        cpLabel_->setText("-");
        marginLabel_->setText("-");
        return;
    }

    massLabel_->setText(QString("%1 g").arg(info.totalMassKg * 1000.0, 0, 'f', 1));
    cgLabel_->setText(QString("%1 mm").arg(info.cgFromNoseM * 1000.0, 0, 'f', 1));
    cpLabel_->setText(QString("%1 mm").arg(info.cpFromNoseM * 1000.0, 0, 'f', 1));

    const QString marginText = QString("%1 calibers").arg(info.marginCalibers, 0, 'f', 2);
    marginLabel_->setText(marginText);
    QString color = "black";
    if (info.marginCalibers < 0.0) {
        color = "#c0392b";  // unstable
    } else if (info.marginCalibers < 1.0) {
        color = "#d68910";  // marginal
    } else {
        color = "#1e8449";  // stable
    }
    marginLabel_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
}

}  // namespace apogee::app
