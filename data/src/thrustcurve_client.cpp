#include "data/thrustcurve_client.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrlQuery>

#include "data/http_transport.hpp"

namespace apogee::data {

namespace {
constexpr const char* kBaseUrl = "https://www.thrustcurve.org/api/v1";

MotorSummary parseMotorSummary(const QJsonObject& obj) {
    MotorSummary m;
    m.motorId = obj.value("motorId").toString();
    m.manufacturer = obj.value("manufacturer").toString();
    m.manufacturerAbbrev = obj.value("manufacturerAbbrev").toString();
    m.designation = obj.value("designation").toString();
    m.commonName = obj.value("commonName").toString();
    m.impulseClass = obj.value("impulseClass").toString();
    m.diameterMm = obj.value("diameter").toDouble();
    m.lengthMm = obj.value("length").toDouble();
    m.motorType = obj.value("type").toString();
    m.certOrg = obj.value("certOrg").toString();
    m.avgThrustN = obj.value("avgThrustN").toDouble();
    m.maxThrustN = obj.value("maxThrustN").toDouble();
    m.totImpulseNs = obj.value("totImpulseNs").toDouble();
    m.burnTimeS = obj.value("burnTimeS").toDouble();
    m.totalWeightG = obj.value("totalWeightG").toDouble();
    m.propWeightG = obj.value("propWeightG").toDouble();
    m.delays = obj.value("delays").toString();
    m.delayAdjustable = obj.value("delayAdjustable").toBool();
    m.propInfo = obj.value("propInfo").toString();
    m.sparky = obj.value("sparky").toBool();
    m.availability = obj.value("availability").toString();
    m.infoUrl = obj.value("infoUrl").toString();
    m.updatedOn = obj.value("updatedOn").toString();
    return m;
}

MotorSimfile parseSimfile(const QJsonObject& obj) {
    MotorSimfile sim;
    sim.motorId = obj.value("motorId").toString();
    sim.simfileId = obj.value("simfileId").toString();
    sim.format = obj.value("format").toString();
    sim.source = obj.value("source").toString();
    sim.infoUrl = obj.value("infoUrl").toString();
    sim.dataUrl = obj.value("dataUrl").toString();

    const QJsonArray samples = obj.value("samples").toArray();
    sim.samples.reserve(samples.size());
    for (const QJsonValue& v : samples) {
        const QJsonObject s = v.toObject();
        sim.samples.push_back({s.value("time").toDouble(), s.value("thrust").toDouble()});
    }
    return sim;
}
}  // namespace

ThrustCurveClient::ThrustCurveClient(HttpTransport& transport) : transport_(transport) {}

QVector<MotorSummary> ThrustCurveClient::searchMotors(const MotorSearchCriteria& criteria) {
    QUrl url(QString("%1/search.json").arg(kBaseUrl));
    QUrlQuery query;
    if (!criteria.manufacturer.isEmpty()) query.addQueryItem("manufacturer", criteria.manufacturer);
    if (!criteria.designation.isEmpty()) query.addQueryItem("designation", criteria.designation);
    query.addQueryItem("maxResults", QString::number(criteria.maxResults));
    url.setQuery(query);

    const HttpResponse response = transport_.get(url);
    QVector<MotorSummary> results;
    if (response.networkError || response.body.isEmpty()) return results;

    const QJsonDocument doc = QJsonDocument::fromJson(response.body);
    const QJsonArray resultsArray = doc.object().value("results").toArray();
    results.reserve(resultsArray.size());
    for (const QJsonValue& v : resultsArray) {
        results.push_back(parseMotorSummary(v.toObject()));
    }
    return results;
}

MotorMetadata ThrustCurveClient::fetchMetadata() {
    const QUrl url(QString("%1/metadata.json").arg(kBaseUrl));
    const HttpResponse response = transport_.get(url);

    MotorMetadata metadata;
    if (response.networkError || response.body.isEmpty()) return metadata;

    const QJsonDocument doc = QJsonDocument::fromJson(response.body);
    const QJsonArray manufacturers = doc.object().value("manufacturers").toArray();
    metadata.manufacturers.reserve(manufacturers.size());
    for (const QJsonValue& v : manufacturers) {
        const QJsonObject obj = v.toObject();
        metadata.manufacturers.push_back(
            {obj.value("name").toString(), obj.value("abbrev").toString()});
    }
    return metadata;
}

std::optional<MotorSimfile> ThrustCurveClient::downloadSamples(const QString& motorId) {
    QUrl url(QString("%1/download.json").arg(kBaseUrl));
    QUrlQuery query;
    query.addQueryItem("motorIds", motorId);
    query.addQueryItem("data", "samples");
    url.setQuery(query);

    const HttpResponse response = transport_.get(url);
    if (response.networkError || response.body.isEmpty()) return std::nullopt;

    const QJsonDocument doc = QJsonDocument::fromJson(response.body);
    const QJsonArray resultsArray = doc.object().value("results").toArray();
    if (resultsArray.isEmpty()) return std::nullopt;

    // Prefer a cert-sourced simfile (NAR/TRA certification data) over a
    // user-submitted one when multiple are available.
    std::optional<MotorSimfile> best;
    for (const QJsonValue& v : resultsArray) {
        MotorSimfile sim = parseSimfile(v.toObject());
        const bool isCert = sim.source == "cert";
        if (!best || (isCert && best->source != "cert")) {
            best = std::move(sim);
        }
    }
    return best;
}

}  // namespace apogee::data
