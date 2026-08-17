#pragma once

class QSqlDatabase;

namespace apogee::data {

// Populates components/kits from the embedded seed JSON (:/seed/*.json).
// Safe to call every startup: a no-op once the database already has the
// current seed data, but wipes and reloads components/kits (motors and
// their cached thrust curves are untouched -- those come from ThrustCurve.
// org, not the seed files) if the embedded seed version has moved on since
// this database was last seeded. A plain "components table has any rows"
// check isn't enough for that: it would leave every existing install
// permanently stuck on whichever seed data it happened to start with, which
// is exactly what happened to the original hand-curated ~25-part catalog
// after it was replaced by the 2,383-part OpenRocket import.
void seedIfNeeded(QSqlDatabase& db);

}  // namespace apogee::data
