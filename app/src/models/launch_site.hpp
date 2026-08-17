#pragma once

#include <optional>

#include <QObject>

#include "data/weather_types.hpp"

namespace apogee::app {

// The launch site currently configured on the Launch tab: coordinates,
// elevation, rail geometry, manual/live wind choice, and (once fetched)
// live weather -- shared between LaunchSitePanel, which sets it via the
// map/spin boxes and the "Fetch Weather" action, and FlightPanel, which
// reads it to build a core::LaunchConditions when flying. Mirrors the
// RocketDesign/RocketBuilderPanel pattern from Phase 4.
class LaunchSite : public QObject {
    Q_OBJECT
public:
    explicit LaunchSite(QObject* parent = nullptr);

    void setCoordinates(double latitude, double longitude);
    void setElevationM(double elevationM);
    void setRailLengthM(double railLengthM);
    void setRailAngleDeg(double railAngleDeg);
    void setManualWind(double speedMs, double directionDeg);
    void setUseLiveWind(bool useLiveWind);
    void setWeather(const std::optional<data::LaunchSiteWeather>& weather);

    bool hasCoordinates() const { return hasCoordinates_; }
    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }
    double elevationM() const { return elevationM_; }
    double railLengthM() const { return railLengthM_; }
    double railAngleDeg() const { return railAngleDeg_; }
    double manualWindSpeedMs() const { return manualWindSpeedMs_; }
    double manualWindDirectionDeg() const { return manualWindDirectionDeg_; }
    bool useLiveWind() const { return useLiveWind_; }
    const std::optional<data::LaunchSiteWeather>& weather() const { return weather_; }

signals:
    void changed();

private:
    bool hasCoordinates_ = false;
    double latitude_ = 0;
    double longitude_ = 0;
    double elevationM_ = 0;
    double railLengthM_ = 1.0;
    double railAngleDeg_ = 0;
    double manualWindSpeedMs_ = 0;
    double manualWindDirectionDeg_ = 0;
    bool useLiveWind_ = false;
    std::optional<data::LaunchSiteWeather> weather_;
};

}  // namespace apogee::app
