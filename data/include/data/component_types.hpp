#pragma once

#include <QString>
#include <QtGlobal>

namespace apogee::data {

struct ComponentSummary {
    qint64 id = 0;
    QString type;  // "nose_cone"|"body_tube"|"transition"|"fin_set"|"parachute"|"streamer"|"motor_mount"
    QString manufacturer;
    QString name;
    QString sku;
    double massG = 0;
    double priceUsd = 0;
    QString notes;
};

struct KitSummary {
    qint64 id = 0;
    QString manufacturer;
    QString name;
    QString sku;
    QString description;
};

// One struct per component_* detail table (schema.cpp), field-for-field.
struct NoseConeDetail {
    QString shape;
    double lengthMm = 0;
    double baseDiameterMm = 0;
    double shoulderLengthMm = 0;
    double shoulderDiameterMm = 0;
    QString material;
};

struct BodyTubeDetail {
    double outerDiameterMm = 0;
    double innerDiameterMm = 0;
    double lengthMm = 0;
    QString material;
};

struct TransitionDetail {
    double foreDiameterMm = 0;
    double aftDiameterMm = 0;
    double lengthMm = 0;
    double foreShoulderLengthMm = 0;
    double aftShoulderLengthMm = 0;
    QString material;
};

struct FinSetDetail {
    int finCount = 0;
    double rootChordMm = 0;
    double tipChordMm = 0;
    double semiSpanMm = 0;
    double sweepLengthMm = 0;
    double thicknessMm = 0;
    double mountingDiameterMm = 0;
    QString crossSection;
    QString material;
};

struct ParachuteDetail {
    double diameterMm = 0;
    double cd = 0.75;
    int shroudLines = 0;
};

struct StreamerDetail {
    double lengthMm = 0;
    double widthMm = 0;
    double cd = 1.0;
};

struct MotorMountDetail {
    double motorDiameterMm = 0;
    double mountLengthMm = 0;
    int centeringRingCount = 0;
};

// A component plus whichever one of the type-specific detail structs above
// applies (selected by summary.type) -- the others are left default. A
// simple flat struct rather than a variant: there are exactly 7 fixed
// component kinds, not an open-ended set, so the extra unused fields cost
// nothing meaningful at this scale.
struct ComponentWithDetail {
    ComponentSummary summary;
    NoseConeDetail noseCone;
    BodyTubeDetail bodyTube;
    TransitionDetail transition;
    FinSetDetail finSet;
    ParachuteDetail parachute;
    StreamerDetail streamer;
    MotorMountDetail motorMount;
};

}  // namespace apogee::data
