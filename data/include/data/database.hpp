#pragma once

#include <QSqlDatabase>
#include <QString>

namespace apogee::data {

// Owns one SQLite connection (a unique Qt connection name per instance, so
// tests can open several independent in-memory databases side by side).
// Applies the schema on open.
class Database {
public:
    // path == ":memory:" opens a private in-memory database (used by tests).
    static Database open(const QString& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    QSqlDatabase& handle() { return db_; }

private:
    Database(QSqlDatabase db, QString connectionName);

    QSqlDatabase db_;
    QString connectionName_;
};

}  // namespace apogee::data
