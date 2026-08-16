#pragma once

#include <optional>

#include "data/weather_types.hpp"

namespace apogee::data {

class HttpTransport;

// Client for api.open-meteo.com's free forecast API (no API key). One call
// covers both surface conditions and the wind-aloft altitude profile
// (10/80/120/180m) the 6DOF physics engine needs -- NWS has no equivalent
// profile, so this is used unconditionally, not just as a fallback.
class OpenMeteoClient {
public:
    explicit OpenMeteoClient(HttpTransport& transport);

    std::optional<LaunchSiteWeather> fetchForecast(double latitude, double longitude);

private:
    HttpTransport& transport_;
};

}  // namespace apogee::data
