#pragma once

#include <QString>
#include <QVector>

namespace apogee::data {

// Mirrors the ThrustCurve.org /api/v1/search.json result shape, field for
// field (verified live 2026-08-15 against a real "Estes C6" query).
struct MotorSummary {
    QString motorId;
    QString manufacturer;
    QString manufacturerAbbrev;
    QString designation;
    QString commonName;
    QString impulseClass;
    double diameterMm = 0;
    double lengthMm = 0;
    QString motorType;  // "SU" | "reload" | "hybrid"
    QString certOrg;
    double avgThrustN = 0;
    double maxThrustN = 0;
    double totImpulseNs = 0;
    double burnTimeS = 0;
    double totalWeightG = 0;
    double propWeightG = 0;
    QString delays;  // "0,3,5,7"
    bool delayAdjustable = false;
    QString propInfo;
    bool sparky = false;
    QString availability;
    QString infoUrl;
    QString updatedOn;
};

struct ThrustSample {
    double timeS = 0;
    double thrustN = 0;
};

// Mirrors /api/v1/download.json?data=samples -- ThrustCurve.org pre-parses
// RASP/RockSim files server-side into plain {time,thrust} pairs, so no
// hand-written RASP/RSE parser is needed.
struct MotorSimfile {
    QString motorId;
    QString simfileId;
    QString format;  // "RASP" | "RockSim"
    QString source;  // "cert" | "user"
    QString infoUrl;
    QString dataUrl;
    QVector<ThrustSample> samples;
};

struct MotorSearchCriteria {
    QString manufacturer;
    QString designation;
    int maxResults = 20;
};

}  // namespace apogee::data
