#pragma once

#include <QString>
#include <QVector>

namespace apogee::data {

struct WeatherConditions {
    double temperatureC = 0;
    double pressureHpa = 0;
    double surfaceWindSpeedMs = 0;
    double surfaceWindDirectionDeg = 0;
    double windGustMs = 0;
    QString source;  // "NWS" or "Open-Meteo"
    bool valid = false;
};

// One altitude band's wind reading, e.g. Open-Meteo's 10/80/120/180m levels.
struct WindLevel {
    double altitudeM = 0;
    double speedMs = 0;
    double directionDeg = 0;
};

struct WindProfile {
    QVector<WindLevel> levels;  // ascending by altitudeM
};

struct LaunchSiteWeather {
    WeatherConditions surface;
    WindProfile windAloft;
};

}  // namespace apogee::data
