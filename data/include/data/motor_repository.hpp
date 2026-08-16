#pragma once

#include <QtGlobal>

#include "data/thrustcurve_types.hpp"

class QSqlDatabase;

namespace apogee::data {

// Caches ThrustCurve.org motor data locally. No TTL -- motor specs rarely
// change, so re-fetching only happens via an explicit "Refresh" action
// (Phase 4+), which re-runs upsertMotor/upsertSimfile.
class MotorRepository {
public:
    explicit MotorRepository(QSqlDatabase& db);

    // Inserts or updates by thrustcurve_motor_id (the natural key). Returns
    // the local row id.
    qint64 upsertMotor(const MotorSummary& motor);

    // Replaces any previously cached simfile/samples for this motor with
    // the given one. Returns the new motor_simfiles row id.
    qint64 upsertSimfile(qint64 motorId, const MotorSimfile& simfile);

    int motorCount();

private:
    QSqlDatabase& db_;
};

}  // namespace apogee::data
