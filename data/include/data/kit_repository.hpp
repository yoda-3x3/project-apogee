#pragma once

#include <QString>
#include <QVector>
#include <QtGlobal>

#include "data/component_types.hpp"

class QSqlDatabase;

namespace apogee::data {

struct KitComponentRef {
    qint64 componentId = 0;
    int quantity = 1;
    QString role;
};

class KitRepository {
public:
    explicit KitRepository(QSqlDatabase& db);

    qint64 insertKit(const KitSummary& kit);
    void addKitComponent(qint64 kitId, qint64 componentId, int quantity, const QString& role);

    int kitCount();

    QVector<KitSummary> listAll();
    QVector<KitComponentRef> listComponents(qint64 kitId);

private:
    QSqlDatabase& db_;
};

}  // namespace apogee::data
