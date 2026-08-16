#include "data/kit_repository.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>

namespace apogee::data {

KitRepository::KitRepository(QSqlDatabase& db) : db_(db) {}

qint64 KitRepository::insertKit(const KitSummary& kit) {
    QSqlQuery query(db_);
    query.prepare("INSERT INTO kits (manufacturer, name, sku, description) VALUES (?,?,?,?)");
    query.addBindValue(kit.manufacturer);
    query.addBindValue(kit.name);
    query.addBindValue(kit.sku);
    query.addBindValue(kit.description);
    query.exec();
    return query.lastInsertId().toLongLong();
}

void KitRepository::addKitComponent(qint64 kitId, qint64 componentId, int quantity,
                                     const QString& role) {
    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO kit_components (kit_id, component_id, quantity, role) VALUES (?,?,?,?)");
    query.addBindValue(kitId);
    query.addBindValue(componentId);
    query.addBindValue(quantity);
    query.addBindValue(role);
    query.exec();
}

int KitRepository::kitCount() {
    QSqlQuery query(db_);
    query.exec("SELECT COUNT(*) FROM kits");
    query.next();
    return query.value(0).toInt();
}

}  // namespace apogee::data
