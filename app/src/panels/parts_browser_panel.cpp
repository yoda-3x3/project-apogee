#include "panels/parts_browser_panel.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "data/component_repository.hpp"
#include "data/motor_repository.hpp"
#include "data/network_http_transport.hpp"
#include "data/thrustcurve_client.hpp"

namespace apogee::app {

PartsBrowserPanel::PartsBrowserPanel(QSqlDatabase& db, QWidget* parent) : QWidget(parent), db_(db) {
    buildUi();
    reloadComponentsTable();
}

void PartsBrowserPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* catalogGroup = new QGroupBox("Seeded Parts Catalog", this);
    auto* catalogLayout = new QVBoxLayout(catalogGroup);
    componentsModel_ = new QStandardItemModel(0, 5, this);
    componentsModel_->setHorizontalHeaderLabels({"Type", "Manufacturer", "Name", "SKU", "Mass (g)"});
    componentsTable_ = new QTableView(catalogGroup);
    componentsTable_->setModel(componentsModel_);
    componentsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    componentsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    componentsTable_->horizontalHeader()->setStretchLastSection(true);
    componentsTable_->setSortingEnabled(true);
    catalogLayout->addWidget(componentsTable_);
    layout->addWidget(catalogGroup, 1);

    auto* searchGroup = new QGroupBox("ThrustCurve.org Motor Search", this);
    auto* searchLayout = new QVBoxLayout(searchGroup);

    auto* searchRow = new QHBoxLayout();
    manufacturerEdit_ = new QLineEdit(searchGroup);
    manufacturerEdit_->setPlaceholderText("Manufacturer (e.g. Estes)");
    designationEdit_ = new QLineEdit(searchGroup);
    designationEdit_->setPlaceholderText("Designation (e.g. C6)");
    searchButton_ = new QPushButton("Search", searchGroup);
    searchRow->addWidget(manufacturerEdit_);
    searchRow->addWidget(designationEdit_);
    searchRow->addWidget(searchButton_);
    searchLayout->addLayout(searchRow);
    connect(searchButton_, &QPushButton::clicked, this, &PartsBrowserPanel::onSearchClicked);

    motorResultsModel_ = new QStandardItemModel(0, 5, this);
    motorResultsModel_->setHorizontalHeaderLabels(
        {"Manufacturer", "Designation", "Impulse Class", "Avg Thrust (N)", "Total Impulse (Ns)"});
    motorResultsTable_ = new QTableView(searchGroup);
    motorResultsTable_->setModel(motorResultsModel_);
    motorResultsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    motorResultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    motorResultsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    motorResultsTable_->horizontalHeader()->setStretchLastSection(true);
    searchLayout->addWidget(motorResultsTable_);

    auto* cacheRow = new QHBoxLayout();
    cacheMotorButton_ = new QPushButton("Cache Selected Motor", searchGroup);
    cacheMotorButton_->setEnabled(false);
    connect(cacheMotorButton_, &QPushButton::clicked, this, &PartsBrowserPanel::onCacheMotorClicked);
    cacheRow->addWidget(cacheMotorButton_);
    statusLabel_ = new QLabel(searchGroup);
    cacheRow->addWidget(statusLabel_, 1);
    searchLayout->addLayout(cacheRow);

    connect(motorResultsTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() { cacheMotorButton_->setEnabled(motorResultsTable_->currentIndex().isValid()); });

    layout->addWidget(searchGroup, 1);
}

void PartsBrowserPanel::reloadComponentsTable() {
    data::ComponentRepository components(db_);
    const QVector<data::ComponentWithDetail> all = components.listAll();

    componentsModel_->removeRows(0, componentsModel_->rowCount());
    componentsModel_->setRowCount(static_cast<int>(all.size()));
    for (int row = 0; row < all.size(); ++row) {
        const data::ComponentSummary& s = all[row].summary;
        componentsModel_->setItem(row, 0, new QStandardItem(s.type));
        componentsModel_->setItem(row, 1, new QStandardItem(s.manufacturer));
        componentsModel_->setItem(row, 2, new QStandardItem(s.name));
        componentsModel_->setItem(row, 3, new QStandardItem(s.sku));
        componentsModel_->setItem(row, 4, new QStandardItem(QString::number(s.massG, 'f', 1)));
    }
}

void PartsBrowserPanel::onSearchClicked() {
    statusLabel_->setText("Searching...");

    data::NetworkHttpTransport transport;
    data::ThrustCurveClient client(transport);
    data::MotorSearchCriteria criteria;
    criteria.manufacturer = manufacturerEdit_->text().trimmed();
    criteria.designation = designationEdit_->text().trimmed();
    criteria.maxResults = 25;

    lastSearchResults_ = client.searchMotors(criteria);

    motorResultsModel_->removeRows(0, motorResultsModel_->rowCount());
    motorResultsModel_->setRowCount(static_cast<int>(lastSearchResults_.size()));
    for (int row = 0; row < lastSearchResults_.size(); ++row) {
        const data::MotorSummary& m = lastSearchResults_[row];
        motorResultsModel_->setItem(row, 0, new QStandardItem(m.manufacturer));
        motorResultsModel_->setItem(row, 1, new QStandardItem(m.designation));
        motorResultsModel_->setItem(row, 2, new QStandardItem(m.impulseClass));
        motorResultsModel_->setItem(row, 3, new QStandardItem(QString::number(m.avgThrustN, 'f', 2)));
        motorResultsModel_->setItem(row, 4, new QStandardItem(QString::number(m.totImpulseNs, 'f', 2)));
    }

    statusLabel_->setText(lastSearchResults_.isEmpty()
                               ? "No results (check network connection or search terms)"
                               : QString("%1 result(s)").arg(lastSearchResults_.size()));
}

void PartsBrowserPanel::onCacheMotorClicked() {
    const int row = motorResultsTable_->currentIndex().row();
    if (row < 0 || row >= lastSearchResults_.size()) return;

    const data::MotorSummary& motor = lastSearchResults_[row];
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
