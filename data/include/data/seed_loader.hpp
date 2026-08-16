#pragma once

class QSqlDatabase;

namespace apogee::data {

// Populates components/kits from the embedded seed JSON (:/seed/*.json) the
// first time the database is empty. Safe to call every startup -- it's a
// no-op once the components table has any rows.
void seedIfEmpty(QSqlDatabase& db);

}  // namespace apogee::data
