#include "models/launch_site.hpp"

namespace apogee::app {

LaunchSite::LaunchSite(QObject* parent) : QObject(parent) {}

void LaunchSite::setCoordinates(double latitude, double longitude) {
    hasCoordinates_ = true;
    latitude_ = latitude;
    longitude_ = longitude;
    emit changed();
}

void LaunchSite::setElevationM(double elevationM) {
    elevationM_ = elevationM;
    emit changed();
}

void LaunchSite::setRailLengthM(double railLengthM) {
    railLengthM_ = railLengthM;
    emit changed();
}

void LaunchSite::setRailAngleDeg(double railAngleDeg) {
    railAngleDeg_ = railAngleDeg;
    emit changed();
}

void LaunchSite::setManualWind(double speedMs, double directionDeg) {
    manualWindSpeedMs_ = speedMs;
    manualWindDirectionDeg_ = directionDeg;
    emit changed();
}

void LaunchSite::setUseLiveWind(bool useLiveWind) {
    useLiveWind_ = useLiveWind;
    emit changed();
}

void LaunchSite::setWeather(const std::optional<data::LaunchSiteWeather>& weather) {
    weather_ = weather;
    emit changed();
}

}  // namespace apogee::app
