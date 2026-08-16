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

QVector<KitSummary> KitRepository::listAll() {
    QVector<KitSummary> result;
    QSqlQuery query(db_);
    query.exec("SELECT id, manufacturer, name, sku, description FROM kits ORDER BY manufacturer, name");
    while (query.next()) {
        KitSummary kit;
        kit.id = query.value(0).toLongLong();
        kit.manufacturer = query.value(1).toString();
        kit.name = query.value(2).toString();
        kit.sku = query.value(3).toString();
        kit.description = query.value(4).toString();
        result.push_back(kit);
    }
    return result;
}

QVector<KitComponentRef> KitRepository::listComponents(qint64 kitId) {
    QVector<KitComponentRef> result;
    QSqlQuery query(db_);
    query.prepare("SELECT component_id, quantity, role FROM kit_components WHERE kit_id = ?");
    query.addBindValue(kitId);
    query.exec();
    while (query.next()) {
        KitComponentRef ref;
        ref.componentId = query.value(0).toLongLong();
        ref.quantity = query.value(1).toInt();
        ref.role = query.value(2).toString();
        result.push_back(ref);
    }
    return result;
}

}  // namespace apogee::data
