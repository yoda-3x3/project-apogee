#include "data/nws_client.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "data/http_transport.hpp"

namespace apogee::data {

namespace {
constexpr const char* kBaseUrl = "https://api.weather.gov";

// Converts a gridpoint field's first (nearest-term) value to a known unit.
// NWS reports uom as "wmoUnit:<unit>"; unknown units pass through raw so a
// caller can at least see a number rather than silently getting 0.
double firstValueConverted(const QJsonObject& field) {
    const QString uom = field.value("uom").toString();
    const QJsonArray values = field.value("values").toArray();
    if (values.isEmpty()) return 0.0;

    const double raw = values.first().toObject().value("value").toDouble();
    if (uom == "wmoUnit:km_h-1") return raw / 3.6;  // -> m/s
    if (uom == "wmoUnit:Pa") return raw / 100.0;    // -> hPa
    return raw;  // degC, degree_(angle) etc. need no conversion
}

bool hasValues(const QJsonObject& field) {
    return !field.value("values").toArray().isEmpty();
}
}  // namespace

NwsClient::NwsClient(HttpTransport& transport) : transport_(transport) {}

std::optional<WeatherConditions> NwsClient::fetchSurfaceConditions(double latitude,
                                                                     double longitude) {
    const QUrl pointsUrl(QString("%1/points/%2,%3")
                              .arg(kBaseUrl)
                              .arg(latitude, 0, 'f', 4)
                              .arg(longitude, 0, 'f', 4));
    const HttpResponse pointsResponse = transport_.get(pointsUrl);
    if (pointsResponse.networkError || pointsResponse.statusCode == 404) return std::nullopt;

    const QJsonObject pointsProps =
        QJsonDocument::fromJson(pointsResponse.body).object().value("properties").toObject();
    const QString gridId = pointsProps.value("gridId").toString();
    const int gridX = pointsProps.value("gridX").toInt();
    const int gridY = pointsProps.value("gridY").toInt();
    if (gridId.isEmpty()) return std::nullopt;

    const QUrl gridUrl(
        QString("%1/gridpoints/%2/%3,%4").arg(kBaseUrl).arg(gridId).arg(gridX).arg(gridY));
    const HttpResponse gridResponse = transport_.get(gridUrl);
    if (gridResponse.networkError || gridResponse.statusCode == 404) return std::nullopt;

    const QJsonObject props =
        QJsonDocument::fromJson(gridResponse.body).object().value("properties").toObject();

    WeatherConditions conditions;
    conditions.temperatureC = firstValueConverted(props.value("temperature").toObject());
    conditions.surfaceWindSpeedMs = firstValueConverted(props.value("windSpeed").toObject());
    conditions.surfaceWindDirectionDeg =
        firstValueConverted(props.value("windDirection").toObject());
    conditions.windGustMs = firstValueConverted(props.value("windGust").toObject());
    // "pressure" is frequently unpopulated for a given forecast office --
    // leave at 0 when absent; WeatherService fills it from Open-Meteo.
    const QJsonObject pressureField = props.value("pressure").toObject();
    conditions.pressureHpa = hasValues(pressureField) ? firstValueConverted(pressureField) : 0.0;
    conditions.source = "NWS";
    conditions.valid = hasValues(props.value("temperature").toObject());
    return conditions;
}

}  // namespace apogee::data
