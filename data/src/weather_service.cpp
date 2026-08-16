#include "data/weather_service.hpp"

#include "data/nws_client.hpp"
#include "data/open_meteo_client.hpp"

namespace apogee::data {

WeatherService::WeatherService(NwsClient& nws, OpenMeteoClient& openMeteo)
    : nws_(nws), openMeteo_(openMeteo) {}

LaunchSiteWeather WeatherService::fetchLaunchSiteWeather(double latitude, double longitude) {
    // Open-Meteo is always fetched: it's the only source of the wind-aloft
    // profile, and it's the surface-conditions fallback too.
    const std::optional<LaunchSiteWeather> openMeteoResult =
        openMeteo_.fetchForecast(latitude, longitude);

    LaunchSiteWeather result;
    if (openMeteoResult) {
        result = *openMeteoResult;
    }

    const std::optional<WeatherConditions> nwsSurface =
        nws_.fetchSurfaceConditions(latitude, longitude);
    if (nwsSurface && nwsSurface->valid) {
        WeatherConditions surface = *nwsSurface;
        // NWS frequently has no pressure reading for a given office; patch
        // it from Open-Meteo rather than reporting a bogus 0.
        if (surface.pressureHpa == 0.0 && result.surface.valid) {
            surface.pressureHpa = result.surface.pressureHpa;
        }
        result.surface = surface;
    }

    return result;
}

}  // namespace apogee::data
