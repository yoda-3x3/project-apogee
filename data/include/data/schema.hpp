#pragma once

class QSqlDatabase;

namespace apogee::data {

// Creates every table (CREATE TABLE IF NOT EXISTS) on an already-open
// QSqlDatabase. Safe to call every startup. Returns false and leaves the
// database's lastError() set if any statement fails.
bool createSchema(QSqlDatabase& db);

}  // namespace apogee::data
