#include "data/database.hpp"

#include <QSqlQuery>
#include <QUuid>

#include "data/schema.hpp"

namespace apogee::data {

Database::Database(QSqlDatabase db, QString connectionName)
    : db_(std::move(db)), connectionName_(std::move(connectionName)) {}

Database::~Database() {
    db_.close();
    const QString name = connectionName_;
    db_ = QSqlDatabase();  // drop this handle before removeDatabase (Qt requirement)
    QSqlDatabase::removeDatabase(name);
}

Database Database::open(const QString& path) {
    // Unique connection name per instance so tests can hold several
    // independent in-memory databases open at once.
    const QString connectionName = QUuid::createUuid().toString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    db.open();
    createSchema(db);
    return Database(std::move(db), connectionName);
}

}  // namespace apogee::data
