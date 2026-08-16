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

ComponentSummary summaryFromQuery(const QSqlQuery& q) {
    ComponentSummary s;
    s.id = q.value("id").toLongLong();
    s.type = q.value("type").toString();
    s.manufacturer = q.value("manufacturer").toString();
    s.name = q.value("name").toString();
    s.sku = q.value("sku").toString();
    s.massG = q.value("mass_g").toDouble();
    s.priceUsd = q.value("price_usd").toDouble();
    s.notes = q.value("notes").toString();
    return s;
}

void fillDetail(QSqlDatabase& db, ComponentWithDetail& c) {
    const QString table = tableNameForType(c.summary.type);
    if (table.isEmpty()) return;

    QSqlQuery q(db);
    q.prepare(QString("SELECT * FROM %1 WHERE component_id = ?").arg(table));
    q.addBindValue(c.summary.id);
    if (!q.exec() || !q.next()) return;

    if (c.summary.type == "nose_cone") {
        c.noseCone.shape = q.value("shape").toString();
        c.noseCone.lengthMm = q.value("length_mm").toDouble();
        c.noseCone.baseDiameterMm = q.value("base_diameter_mm").toDouble();
        c.noseCone.shoulderLengthMm = q.value("shoulder_length_mm").toDouble();
        c.noseCone.shoulderDiameterMm = q.value("shoulder_diameter_mm").toDouble();
        c.noseCone.material = q.value("material").toString();
    } else if (c.summary.type == "body_tube") {
        c.bodyTube.outerDiameterMm = q.value("outer_diameter_mm").toDouble();
        c.bodyTube.innerDiameterMm = q.value("inner_diameter_mm").toDouble();
        c.bodyTube.lengthMm = q.value("length_mm").toDouble();
        c.bodyTube.material = q.value("material").toString();
    } else if (c.summary.type == "transition") {
        c.transition.foreDiameterMm = q.value("fore_diameter_mm").toDouble();
        c.transition.aftDiameterMm = q.value("aft_diameter_mm").toDouble();
        c.transition.lengthMm = q.value("length_mm").toDouble();
        c.transition.foreShoulderLengthMm = q.value("fore_shoulder_length_mm").toDouble();
        c.transition.aftShoulderLengthMm = q.value("aft_shoulder_length_mm").toDouble();
        c.transition.material = q.value("material").toString();
    } else if (c.summary.type == "fin_set") {
        c.finSet.finCount = q.value("fin_count").toInt();
        c.finSet.rootChordMm = q.value("root_chord_mm").toDouble();
        c.finSet.tipChordMm = q.value("tip_chord_mm").toDouble();
        c.finSet.semiSpanMm = q.value("semi_span_mm").toDouble();
        c.finSet.sweepLengthMm = q.value("sweep_length_mm").toDouble();
        c.finSet.thicknessMm = q.value("thickness_mm").toDouble();
        c.finSet.mountingDiameterMm = q.value("mounting_diameter_mm").toDouble();
        c.finSet.crossSection = q.value("cross_section").toString();
        c.finSet.material = q.value("material").toString();
    } else if (c.summary.type == "parachute") {
        c.parachute.diameterMm = q.value("diameter_mm").toDouble();
        c.parachute.cd = q.value("cd").toDouble();
        c.parachute.shroudLines = q.value("shroud_lines").toInt();
    } else if (c.summary.type == "streamer") {
        c.streamer.lengthMm = q.value("length_mm").toDouble();
        c.streamer.widthMm = q.value("width_mm").toDouble();
        c.streamer.cd = q.value("cd").toDouble();
    } else if (c.summary.type == "motor_mount") {
        c.motorMount.motorDiameterMm = q.value("motor_diameter_mm").toDouble();
        c.motorMount.mountLengthMm = q.value("mount_length_mm").toDouble();
        c.motorMount.centeringRingCount = q.value("centering_ring_count").toInt();
    }
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

QVector<ComponentWithDetail> ComponentRepository::listAll() {
    QVector<ComponentWithDetail> result;
    QSqlQuery query(db_);
    query.exec("SELECT * FROM components ORDER BY type, name");
    while (query.next()) {
        ComponentWithDetail c;
        c.summary = summaryFromQuery(query);
        fillDetail(db_, c);
        result.push_back(c);
    }
    return result;
}

QVector<ComponentWithDetail> ComponentRepository::listByType(const QString& type) {
    QVector<ComponentWithDetail> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM components WHERE type = ? ORDER BY name");
    query.addBindValue(type);
    query.exec();
    while (query.next()) {
        ComponentWithDetail c;
        c.summary = summaryFromQuery(query);
        fillDetail(db_, c);
        result.push_back(c);
    }
    return result;
}

std::optional<ComponentWithDetail> ComponentRepository::getById(qint64 id) {
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM components WHERE id = ?");
    query.addBindValue(id);
    query.exec();
    if (!query.next()) return std::nullopt;

    ComponentWithDetail c;
    c.summary = summaryFromQuery(query);
    fillDetail(db_, c);
    return c;
}

}  // namespace apogee::data
