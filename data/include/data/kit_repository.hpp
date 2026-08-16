#pragma once

#include <QString>
#include <QtGlobal>

#include "data/component_types.hpp"

class QSqlDatabase;

namespace apogee::data {

class KitRepository {
public:
    explicit KitRepository(QSqlDatabase& db);

    qint64 insertKit(const KitSummary& kit);
    void addKitComponent(qint64 kitId, qint64 componentId, int quantity, const QString& role);

    int kitCount();

private:
    QSqlDatabase& db_;
};

}  // namespace apogee::data
