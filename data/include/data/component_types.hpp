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

}  // namespace apogee::data
