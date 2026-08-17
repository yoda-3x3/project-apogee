#include "data/seed_loader.hpp"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantMap>

#include "data/component_repository.hpp"
#include "data/kit_repository.hpp"

namespace apogee::data {

namespace {

// Bump whenever components.json/kits.json changes in a way that should
// force existing databases to reseed -- see seed_loader.hpp's comment on
// seedIfNeeded() for why a version check is needed at all.
constexpr int kSeedVersion = 2;

QJsonArray readJsonArray(const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).array();
}

QVariantMap detailToVariantMap(const QJsonObject& detail) {
    QVariantMap map;
    for (auto it = detail.constBegin(); it != detail.constEnd(); ++it) {
        map.insert(it.key(), it.value().toVariant());
    }
    return map;
}

// Loads components.json, inserting each as a component + its type-specific
// detail row. Returns a map from the seed file's local "key" to the row id
// the component actually got, so kits.json can reference components.
QHash<QString, qint64> loadComponents(ComponentRepository& components) {
    QHash<QString, qint64> keyToId;
    for (const QJsonValue& v : readJsonArray(":/seed/components.json")) {
        const QJsonObject obj = v.toObject();

        ComponentSummary summary;
        summary.type = obj.value("type").toString();
        summary.manufacturer = obj.value("manufacturer").toString();
        summary.name = obj.value("name").toString();
        summary.sku = obj.value("sku").toString();
        summary.massG = obj.value("mass_g").toDouble();
        summary.priceUsd = obj.value("price_usd").toDouble();
        summary.notes = obj.value("notes").toString();

        const qint64 id = components.insertComponent(summary);
        components.insertDetail(id, summary.type, detailToVariantMap(obj.value("detail").toObject()));

        keyToId.insert(obj.value("key").toString(), id);
    }
    return keyToId;
}

void loadKits(KitRepository& kits, const QHash<QString, qint64>& componentKeyToId) {
    for (const QJsonValue& v : readJsonArray(":/seed/kits.json")) {
        const QJsonObject obj = v.toObject();

        KitSummary summary;
        summary.manufacturer = obj.value("manufacturer").toString();
        summary.name = obj.value("name").toString();
        summary.sku = obj.value("sku").toString();
        summary.description = obj.value("description").toString();

        const qint64 kitId = kits.insertKit(summary);

        for (const QJsonValue& componentRef : obj.value("components").toArray()) {
            const QJsonObject ref = componentRef.toObject();
            const QString key = ref.value("key").toString();
            if (!componentKeyToId.contains(key)) continue;  // seed data typo -- skip, don't crash
            kits.addKitComponent(kitId, componentKeyToId.value(key), ref.value("quantity").toInt(1),
                                  ref.value("role").toString());
        }
    }
}

int storedSeedVersion(QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare("SELECT value FROM seed_meta WHERE key = 'seed_version'");
    if (!query.exec() || !query.next()) return 0;
    return query.value(0).toInt();
}

void setStoredSeedVersion(QSqlDatabase& db, int version) {
    QSqlQuery deleteQuery(db);
    deleteQuery.exec("DELETE FROM seed_meta WHERE key = 'seed_version'");

    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO seed_meta (key, value) VALUES ('seed_version', :v)");
    insertQuery.bindValue(":v", version);
    insertQuery.exec();
}

// Clears out a previous seeding's components/kits so loadComponents()/
// loadKits() can repopulate from scratch. Deliberately leaves the motors/
// motor_simfiles/motor_thrust_samples tables alone -- those hold live
// ThrustCurve.org data the user fetched themselves, unrelated to the
// bundled seed JSON.
void wipeSeedTables(QSqlDatabase& db) {
    static const QStringList kSeedTables = {
        "kit_components",       "kits",
        "component_nose_cones", "component_body_tubes",   "component_transitions",
        "component_fin_sets",   "component_parachutes",   "component_streamers",
        "component_motor_mounts", "components",
    };
    for (const QString& table : kSeedTables) {
        QSqlQuery query(db);
        query.exec("DELETE FROM " + table);
    }
}

}  // namespace

void seedIfNeeded(QSqlDatabase& db) {
    ComponentRepository components(db);
    const bool alreadySeeded = components.componentCount() > 0;
    if (alreadySeeded && storedSeedVersion(db) == kSeedVersion) return;  // nothing changed

    if (alreadySeeded) wipeSeedTables(db);  // stale data from an older seed version

    const QHash<QString, qint64> keyToId = loadComponents(components);

    KitRepository kits(db);
    loadKits(kits, keyToId);

    setStoredSeedVersion(db, kSeedVersion);
}

}  // namespace apogee::data
