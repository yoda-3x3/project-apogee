#include "data/component_repository.hpp"

#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

namespace apogee::data {

namespace {
QString tableNameForType(const QString& type) {
    static const QMap<QString, QString> kTypeToTable = {
        {"nose_cone", "component_nose_cones"},   {"body_tube", "component_body_tubes"},
        {"transition", "component_transitions"}, {"fin_set", "component_fin_sets"},
        {"parachute", "component_parachutes"},   {"streamer", "component_streamers"},
        {"motor_mount", "component_motor_mounts"},
    };
    return kTypeToTable.value(type);
}
}  // namespace

ComponentRepository::ComponentRepository(QSqlDatabase& db) : db_(db) {}

qint64 ComponentRepository::insertComponent(const ComponentSummary& component) {
    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO components (type, manufacturer, name, sku, mass_g, price_usd, notes) "
        "VALUES (?,?,?,?,?,?,?)");
    query.addBindValue(component.type);
    query.addBindValue(component.manufacturer);
    query.addBindValue(component.name);
    query.addBindValue(component.sku);
    query.addBindValue(component.massG);
    query.addBindValue(component.priceUsd);
    query.addBindValue(component.notes);
    query.exec();
    return query.lastInsertId().toLongLong();
}

void ComponentRepository::insertDetail(qint64 componentId, const QString& type,
                                        const QVariantMap& fields) {
    const QString table = tableNameForType(type);
    if (table.isEmpty()) return;

    QStringList columns = {"component_id"};
    QStringList placeholders = {"?"};
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        columns << it.key();
        placeholders << "?";
    }

    QSqlQuery query(db_);
    query.prepare(
        QString("INSERT INTO %1 (%2) VALUES (%3)").arg(table, columns.join(","), placeholders.join(",")));
    query.addBindValue(componentId);
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        query.addBindValue(it.value());
    }
    query.exec();
}

int ComponentRepository::componentCount() {
    QSqlQuery query(db_);
    query.exec("SELECT COUNT(*) FROM components");
    query.next();
    return query.value(0).toInt();
}

}  // namespace apogee::data
