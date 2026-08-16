#include "data/open_meteo_client.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include "data/http_transport.hpp"

namespace apogee::data {

namespace {
constexpr const char* kBaseUrl = "https://api.open-meteo.com/v1/forecast";

// All hourly series share one "time" axis; index 0 is the nearest hour to
// now (forecast_days=1, timezone=auto keeps that aligned to the request).
double firstOf(const QJsonObject& hourly, const QString& key) {
    const QJsonArray values = hourly.value(key).toArray();
    return values.isEmpty() ? 0.0 : values.first().toDouble();
}

double kmhToMs(double kmh) { return kmh / 3.6; }
}  // namespace

OpenMeteoClient::OpenMeteoClient(HttpTransport& transport) : transport_(transport) {}

std::optional<LaunchSiteWeather> OpenMeteoClient::fetchForecast(double latitude,
                                                                  double longitude) {
    QUrl url(kBaseUrl);
    QUrlQuery query;
    query.addQueryItem("latitude", QString::number(latitude, 'f', 4));
    query.addQueryItem("longitude", QString::number(longitude, 'f', 4));
    query.addQueryItem("hourly",
                        "wind_speed_10m,wind_direction_10m,"
                        "wind_speed_80m,wind_direction_80m,"
                        "wind_speed_120m,wind_direction_120m,"
                        "wind_speed_180m,wind_direction_180m,"
                        "wind_gusts_10m,temperature_2m,surface_pressure");
    query.addQueryItem("timezone", "auto");
    query.addQueryItem("forecast_days", "1");
    url.setQuery(query);

    const HttpResponse response = transport_.get(url);
    if (response.networkError || response.body.isEmpty()) return std::nullopt;

    const QJsonObject hourly =
        QJsonDocument::fromJson(response.body).object().value("hourly").toObject();
    if (hourly.isEmpty()) return std::nullopt;

    LaunchSiteWeather result;

    result.surface.temperatureC = firstOf(hourly, "temperature_2m");
    result.surface.pressureHpa = firstOf(hourly, "surface_pressure");
    result.surface.surfaceWindSpeedMs = kmhToMs(firstOf(hourly, "wind_speed_10m"));
    result.surface.surfaceWindDirectionDeg = firstOf(hourly, "wind_direction_10m");
    result.surface.windGustMs = kmhToMs(firstOf(hourly, "wind_gusts_10m"));
    result.surface.source = "Open-Meteo";
    result.surface.valid = true;

    result.windAloft.levels = {
        {10.0, kmhToMs(firstOf(hourly, "wind_speed_10m")), firstOf(hourly, "wind_direction_10m")},
        {80.0, kmhToMs(firstOf(hourly, "wind_speed_80m")), firstOf(hourly, "wind_direction_80m")},
        {120.0, kmhToMs(firstOf(hourly, "wind_speed_120m")),
         firstOf(hourly, "wind_direction_120m")},
        {180.0, kmhToMs(firstOf(hourly, "wind_speed_180m")),
         firstOf(hourly, "wind_direction_180m")},
    };

    return result;
}

}  // namespace apogee::data
