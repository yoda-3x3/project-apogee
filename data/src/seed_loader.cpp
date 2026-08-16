#include "data/seed_loader.hpp"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QVariantMap>

#include "data/component_repository.hpp"
#include "data/kit_repository.hpp"

namespace apogee::data {

namespace {

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

}  // namespace

void seedIfEmpty(QSqlDatabase& db) {
    ComponentRepository components(db);
    if (components.componentCount() > 0) return;  // already seeded

    const QHash<QString, qint64> keyToId = loadComponents(components);

    KitRepository kits(db);
    loadKits(kits, keyToId);
}

}  // namespace apogee::data
