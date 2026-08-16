#pragma once

#include "data/weather_types.hpp"

namespace apogee::data {

class NwsClient;
class OpenMeteoClient;

// Combines NwsClient and OpenMeteoClient into the single weather picture a
// launch site needs: NWS's human-QC'd surface conditions when available
// (US sites), Open-Meteo as the surface fallback (non-US, or an NWS
// outage), and Open-Meteo always for the wind-aloft altitude profile since
// NWS has no equivalent.
class WeatherService {
public:
    WeatherService(NwsClient& nws, OpenMeteoClient& openMeteo);

    LaunchSiteWeather fetchLaunchSiteWeather(double latitude, double longitude);

private:
    NwsClient& nws_;
    OpenMeteoClient& openMeteo_;
};

}  // namespace apogee::data
