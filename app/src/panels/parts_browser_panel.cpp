#include "panels/parts_browser_panel.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QSqlDatabase>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

#include "data/component_repository.hpp"
#include "data/motor_repository.hpp"
#include "data/network_http_transport.hpp"
#include "data/thrustcurve_client.hpp"

namespace apogee::app {

namespace {
constexpr int kUnselectedItemData = -1;

QString formatModelLabel(const data::MotorSummary& m) {
    return QString("%1 (%2, %3 Ns)").arg(m.designation, m.impulseClass).arg(m.totImpulseNs, 0, 'f', 1);
}

QString formatMotorDetail(const data::MotorSummary& m) {
    return QString("%1 %2 -- avg %3 N, max %4 N, total impulse %5 Ns, burn %6 s")
        .arg(m.manufacturer, m.designation)
        .arg(m.avgThrustN, 0, 'f', 2)
        .arg(m.maxThrustN, 0, 'f', 2)
        .arg(m.totImpulseNs, 0, 'f', 2)
        .arg(m.burnTimeS, 0, 'f', 2);
}
}  // namespace

PartsBrowserPanel::PartsBrowserPanel(QSqlDatabase& db, QWidget* parent) : QWidget(parent), db_(db) {
    buildUi();
    reloadComponentsTable();
    loadManufacturers();
}

void PartsBrowserPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* catalogGroup = new QGroupBox("Seeded Parts Catalog", this);
    auto* catalogLayout = new QVBoxLayout(catalogGroup);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Manufacturer:", catalogGroup));
    componentManufacturerFilterCombo_ = new QComboBox(catalogGroup);
    connect(componentManufacturerFilterCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &PartsBrowserPanel::onComponentManufacturerFilterChanged);
    filterRow->addWidget(componentManufacturerFilterCombo_, 1);
    catalogLayout->addLayout(filterRow);

    componentsModel_ = new QStandardItemModel(0, 5, this);
    componentsModel_->setHorizontalHeaderLabels({"Type", "Manufacturer", "Name", "SKU", "Mass (g)"});
    componentsProxy_ = new QSortFilterProxyModel(this);
    componentsProxy_->setSourceModel(componentsModel_);
    componentsProxy_->setFilterKeyColumn(1);  // Manufacturer column

    componentsTable_ = new QTableView(catalogGroup);
    componentsTable_->setModel(componentsProxy_);
    componentsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    componentsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    componentsTable_->horizontalHeader()->setStretchLastSection(true);
    componentsTable_->setSortingEnabled(true);
    catalogLayout->addWidget(componentsTable_);
    layout->addWidget(catalogGroup, 1);

    auto* searchGroup = new QGroupBox("ThrustCurve.org Motor Search", this);
    auto* searchForm = new QFormLayout(searchGroup);

    manufacturerCombo_ = new QComboBox(searchGroup);
    searchForm->addRow("Manufacturer:", manufacturerCombo_);
    connect(manufacturerCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &PartsBrowserPanel::onManufacturerChanged);

    modelCombo_ = new QComboBox(searchGroup);
    modelCombo_->setEnabled(false);
    searchForm->addRow("Model:", modelCombo_);
    connect(modelCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &PartsBrowserPanel::onModelChanged);

    selectedMotorDetailLabel_ = new QLabel(searchGroup);
    selectedMotorDetailLabel_->setWordWrap(true);
    searchForm->addRow(selectedMotorDetailLabel_);

    auto* cacheRow = new QHBoxLayout();
    cacheMotorButton_ = new QPushButton("Cache Selected Motor", searchGroup);
    cacheMotorButton_->setEnabled(false);
    connect(cacheMotorButton_, &QPushButton::clicked, this, &PartsBrowserPanel::onCacheMotorClicked);
    cacheRow->addWidget(cacheMotorButton_);
    statusLabel_ = new QLabel(searchGroup);
    cacheRow->addWidget(statusLabel_, 1);
    searchForm->addRow(cacheRow);

    layout->addWidget(searchGroup);
}

void PartsBrowserPanel::reloadComponentsTable() {
    data::ComponentRepository components(db_);
    const QVector<data::ComponentWithDetail> all = components.listAll();

    componentsModel_->removeRows(0, componentsModel_->rowCount());
    componentsModel_->setRowCount(static_cast<int>(all.size()));
    QSet<QString> manufacturers;
    for (int row = 0; row < all.size(); ++row) {
        const data::ComponentSummary& s = all[row].summary;
        componentsModel_->setItem(row, 0, new QStandardItem(s.type));
        componentsModel_->setItem(row, 1, new QStandardItem(s.manufacturer));
        componentsModel_->setItem(row, 2, new QStandardItem(s.name));
        componentsModel_->setItem(row, 3, new QStandardItem(s.sku));
        componentsModel_->setItem(row, 4, new QStandardItem(QString::number(s.massG, 'f', 1)));
        manufacturers.insert(s.manufacturer);
    }

    QStringList sortedManufacturers(manufacturers.begin(), manufacturers.end());
    sortedManufacturers.sort(Qt::CaseInsensitive);

    const QString previousSelection = componentManufacturerFilterCombo_->currentData().toString();
    const QSignalBlocker blocker(componentManufacturerFilterCombo_);
    componentManufacturerFilterCombo_->clear();
    componentManufacturerFilterCombo_->addItem(QString("(all manufacturers -- %1 parts)").arg(all.size()),
                                                QString());
    for (const QString& manufacturer : sortedManufacturers) {
        componentManufacturerFilterCombo_->addItem(manufacturer, manufacturer);
    }
    const int restoredIndex = componentManufacturerFilterCombo_->findData(previousSelection);
    componentManufacturerFilterCombo_->setCurrentIndex(restoredIndex >= 0 ? restoredIndex : 0);

    onComponentManufacturerFilterChanged();
}

void PartsBrowserPanel::onComponentManufacturerFilterChanged() {
    const QString manufacturer = componentManufacturerFilterCombo_->currentData().toString();
    if (manufacturer.isEmpty()) {
        componentsProxy_->setFilterRegularExpression(QRegularExpression());
    } else {
        componentsProxy_->setFilterRegularExpression(
            QRegularExpression(QRegularExpression::anchoredPattern(QRegularExpression::escape(manufacturer))));
    }
}

void PartsBrowserPanel::loadManufacturers() {
    statusLabel_->setText("Loading manufacturers...");

    data::NetworkHttpTransport transport;
    data::ThrustCurveClient client(transport);
    const data::MotorMetadata metadata = client.fetchMetadata();

    manufacturerCombo_->clear();
    manufacturerCombo_->addItem("(select manufacturer)", kUnselectedItemData);
    for (const data::ManufacturerInfo& m : metadata.manufacturers) {
        manufacturerCombo_->addItem(m.name, m.abbrev);
    }

    statusLabel_->setText(metadata.manufacturers.isEmpty()
                               ? "Could not load manufacturers (check network connection)"
                               : QString("%1 manufacturers available").arg(metadata.manufacturers.size()));
}

void PartsBrowserPanel::onManufacturerChanged() {
    modelCombo_->clear();
    modelCombo_->setEnabled(false);
    selectedMotorDetailLabel_->clear();
    cacheMotorButton_->setEnabled(false);
    currentModels_.clear();

    const QString abbrev = manufacturerCombo_->currentData().toString();
    if (abbrev.isEmpty()) return;

    statusLabel_->setText(QString("Searching %1 motors...").arg(manufacturerCombo_->currentText()));

    data::NetworkHttpTransport transport;
    data::ThrustCurveClient client(transport);
    data::MotorSearchCriteria criteria;
    criteria.manufacturer = abbrev;
    criteria.maxResults = 250;
    currentModels_ = client.searchMotors(criteria);

    modelCombo_->addItem("(select model)", kUnselectedItemData);
    for (int i = 0; i < currentModels_.size(); ++i) {
        modelCombo_->addItem(formatModelLabel(currentModels_[i]), i);
    }
    modelCombo_->setEnabled(!currentModels_.isEmpty());

    statusLabel_->setText(currentModels_.isEmpty()
                               ? "No motors found for that manufacturer"
                               : QString("%1 model(s)").arg(currentModels_.size()));
}

void PartsBrowserPanel::onModelChanged() {
    const int index = modelCombo_->currentData().toInt();
    if (index < 0 || index >= currentModels_.size()) {
        selectedMotorDetailLabel_->clear();
        cacheMotorButton_->setEnabled(false);
        return;
    }

    selectedMotorDetailLabel_->setText(formatMotorDetail(currentModels_[index]));
    cacheMotorButton_->setEnabled(true);
}

void PartsBrowserPanel::onCacheMotorClicked() {
    const int index = modelCombo_->currentData().toInt();
    if (index < 0 || index >= currentModels_.size()) return;

    const data::MotorSummary& motor = currentModels_[index];
    statusLabel_->setText(QString("Caching %1 %2...").arg(motor.manufacturer, motor.designation));

    data::NetworkHttpTransport transport;
    data::ThrustCurveClient client(transport);
    const auto simfile = client.downloadSamples(motor.motorId);

    data::MotorRepository motorRepo(db_);
    const qint64 motorRowId = motorRepo.upsertMotor(motor);
    if (simfile) {
        motorRepo.upsertSimfile(motorRowId, *simfile);
        statusLabel_->setText(QString("Cached %1 %2 (%3 samples)")
                                   .arg(motor.manufacturer, motor.designation)
                                   .arg(simfile->samples.size()));
    } else {
        statusLabel_->setText(QString("Cached %1 %2 spec (no thrust samples available)")
                                   .arg(motor.manufacturer, motor.designation));
    }

    emit motorsCached();
}

}  // namespace apogee::app
