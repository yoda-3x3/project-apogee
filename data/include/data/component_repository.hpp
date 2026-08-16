#pragma once

#include <QVariantMap>
#include <QtGlobal>

#include "data/component_types.hpp"

class QSqlDatabase;

namespace apogee::data {

class ComponentRepository {
public:
    explicit ComponentRepository(QSqlDatabase& db);

    qint64 insertComponent(const ComponentSummary& component);

    // Inserts the type-specific detail row (e.g. component_nose_cones) for
    // an already-inserted component. `fields` keys must match that table's
    // column names (see schema.cpp) -- kept generic instead of one method
    // per component type since the seven detail tables only differ in
    // column names, not in insertion logic.
    void insertDetail(qint64 componentId, const QString& type, const QVariantMap& fields);

    int componentCount();

private:
    QSqlDatabase& db_;
};

}  // namespace apogee::data
