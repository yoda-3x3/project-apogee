#pragma once

#include <optional>

#include "data/thrustcurve_types.hpp"

namespace apogee::data {

class HttpTransport;

// Client for thrustcurve.org's public JSON API (no API key required). All
// JSON-key knowledge is isolated here so only this file needs edits if the
// real schema drifts from what was verified live during planning.
class ThrustCurveClient {
public:
    explicit ThrustCurveClient(HttpTransport& transport);

    QVector<MotorSummary> searchMotors(const MotorSearchCriteria& criteria);

    // Downloads pre-parsed thrust samples for a motor, preferring a
    // cert-sourced simfile over a user-submitted one. Returns nullopt if the
    // motor has no simfiles or the request fails.
    std::optional<MotorSimfile> downloadSamples(const QString& motorId);

private:
    HttpTransport& transport_;
};

}  // namespace apogee::data
