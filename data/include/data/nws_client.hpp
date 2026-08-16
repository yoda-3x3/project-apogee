#pragma once

#include <optional>

#include "data/weather_types.hpp"

namespace apogee::data {

class HttpTransport;

// Client for api.weather.gov (no API key; requires a descriptive User-Agent,
// sent by NetworkHttpTransport on every request). NWS gridpoint data gives
// human-QC'd SURFACE conditions only -- no altitude-band wind profile (see
// WeatherService for how the wind-aloft profile is sourced instead).
class NwsClient {
public:
    explicit NwsClient(HttpTransport& transport);

    // Resolves (lat, lon) to a forecast office grid via /points, then reads
    // current surface conditions from /gridpoints. Returns nullopt for
    // non-US coordinates (the /points lookup 404s there) or on network
    // failure -- both are WeatherService's cue to fall back to Open-Meteo.
    std::optional<WeatherConditions> fetchSurfaceConditions(double latitude, double longitude);

private:
    HttpTransport& transport_;
};

}  // namespace apogee::data
