#pragma once

#include <optional>

#include <QVariantMap>
#include <QVector>
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

    // Every component, each joined with its type-specific detail row.
    QVector<ComponentWithDetail> listAll();

    // Every component of one type (e.g. "nose_cone"), joined with detail.
    QVector<ComponentWithDetail> listByType(const QString& type);

    std::optional<ComponentWithDetail> getById(qint64 id);

private:
    QSqlDatabase& db_;
};

}  // namespace apogee::data
