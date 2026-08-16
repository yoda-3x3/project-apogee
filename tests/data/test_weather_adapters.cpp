#include <catch_amalgamated.hpp>

#include <QFile>

#include "data/nws_client.hpp"
#include "data/open_meteo_client.hpp"
#include "data/weather_service.hpp"
#include "support/fixture_http_transport.hpp"

using namespace apogee::data;

namespace {
QByteArray readFixture(const QString& name) {
    QFile file(QString(APOGEE_TEST_FIXTURES_DIR) + "/" + name);
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}
}  // namespace

// All fixtures below were recorded from real, live API responses
// (2026-08-15) -- see tests/fixtures/*.json.

TEST_CASE("NwsClient parses real points+gridpoint responses", "[data][weather][nws]") {
    FixtureHttpTransport transport;
    transport.addFixture("/points/", readFixture("nws_points_sample.json"));
    transport.addFixture("/gridpoints/", readFixture("nws_gridpoint_sample.json"));
    NwsClient client(transport);

    const auto conditions = client.fetchSurfaceConditions(39.7456, -97.0892);
    REQUIRE(conditions.has_value());
    CHECK(conditions->valid);
    CHECK(conditions->source == "NWS");
    CHECK(conditions->temperatureC == Catch::Approx(28.333333).epsilon(0.001));
    CHECK(conditions->surfaceWindSpeedMs == Catch::Approx(1.852 / 3.6).epsilon(0.001));
    CHECK(conditions->surfaceWindDirectionDeg == Catch::Approx(280.0));
    CHECK(conditions->windGustMs == Catch::Approx(1.852 / 3.6).epsilon(0.001));
    // This grid office had no "pressure" values populated -- confirmed
    // directly against the live response, not assumed.
    CHECK(conditions->pressureHpa == Catch::Approx(0.0));
}

TEST_CASE("NwsClient returns nullopt for a non-US point (404)", "[data][weather][nws]") {
    FixtureHttpTransport transport;  // no /points/ fixture registered -> 404
    NwsClient client(transport);

    CHECK_FALSE(client.fetchSurfaceConditions(48.8566, 2.3522).has_value());  // Paris
}

TEST_CASE("OpenMeteoClient parses a real forecast response", "[data][weather][openmeteo]") {
    FixtureHttpTransport transport;
    transport.addFixture("open-meteo.com", readFixture("openmeteo_forecast_sample.json"));
    OpenMeteoClient client(transport);

    const auto weather = client.fetchForecast(39.7456, -97.0892);
    REQUIRE(weather.has_value());

    CHECK(weather->surface.valid);
    CHECK(weather->surface.source == "Open-Meteo");
    CHECK(weather->surface.temperatureC == Catch::Approx(26.2));
    CHECK(weather->surface.pressureHpa == Catch::Approx(963.4));
    CHECK(weather->surface.surfaceWindSpeedMs == Catch::Approx(18.6 / 3.6).epsilon(0.001));
    CHECK(weather->surface.surfaceWindDirectionDeg == Catch::Approx(136.0));

    REQUIRE(weather->windAloft.levels.size() == 4);
    CHECK(weather->windAloft.levels[0].altitudeM == Catch::Approx(10.0));
    CHECK(weather->windAloft.levels[0].speedMs == Catch::Approx(18.6 / 3.6).epsilon(0.001));
    CHECK(weather->windAloft.levels[1].altitudeM == Catch::Approx(80.0));
    CHECK(weather->windAloft.levels[1].speedMs == Catch::Approx(23.9 / 3.6).epsilon(0.001));
    CHECK(weather->windAloft.levels[2].altitudeM == Catch::Approx(120.0));
    CHECK(weather->windAloft.levels[2].speedMs == Catch::Approx(16.7 / 3.6).epsilon(0.001));
    CHECK(weather->windAloft.levels[3].altitudeM == Catch::Approx(180.0));
    CHECK(weather->windAloft.levels[3].speedMs == Catch::Approx(23.8 / 3.6).epsilon(0.001));
    CHECK(weather->windAloft.levels[3].directionDeg == Catch::Approx(94.0));
}

TEST_CASE("WeatherService prefers NWS surface conditions but patches pressure from Open-Meteo",
          "[data][weather][service]") {
    FixtureHttpTransport transport;
    transport.addFixture("/points/", readFixture("nws_points_sample.json"));
    transport.addFixture("/gridpoints/", readFixture("nws_gridpoint_sample.json"));
    transport.addFixture("open-meteo.com", readFixture("openmeteo_forecast_sample.json"));

    NwsClient nws(transport);
    OpenMeteoClient openMeteo(transport);
    WeatherService service(nws, openMeteo);

    const LaunchSiteWeather result = service.fetchLaunchSiteWeather(39.7456, -97.0892);

    CHECK(result.surface.source == "NWS");                       // NWS preferred when available
    CHECK(result.surface.temperatureC == Catch::Approx(28.333333).epsilon(0.001));  // from NWS
    CHECK(result.surface.pressureHpa == Catch::Approx(963.4));    // patched from Open-Meteo
    REQUIRE(result.windAloft.levels.size() == 4);                 // always from Open-Meteo
}

TEST_CASE("WeatherService falls back to Open-Meteo entirely when NWS has no coverage",
          "[data][weather][service]") {
    FixtureHttpTransport transport;
    transport.addFixture("open-meteo.com", readFixture("openmeteo_forecast_sample.json"));
    // no /points/ fixture -> NWS 404s for this (non-US) coordinate

    NwsClient nws(transport);
    OpenMeteoClient openMeteo(transport);
    WeatherService service(nws, openMeteo);

    const LaunchSiteWeather result = service.fetchLaunchSiteWeather(48.8566, 2.3522);

    CHECK(result.surface.source == "Open-Meteo");
    CHECK(result.surface.valid);
    REQUIRE(result.windAloft.levels.size() == 4);
}
