#include <catch_amalgamated.hpp>

#include <QFile>

#include "data/thrustcurve_client.hpp"
#include "support/fixture_http_transport.hpp"

using namespace apogee::data;

namespace {
QByteArray readFixture(const QString& name) {
    QFile file(QString(APOGEE_TEST_FIXTURES_DIR) + "/" + name);
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}
}  // namespace

// Fixtures were recorded from real thrustcurve.org API responses (2026-08-15,
// see tests/fixtures/*.json) so this test verifies the adapter against the
// actual field names/shape, not an assumed one.

TEST_CASE("ThrustCurveClient parses a real search.json response", "[data][thrustcurve]") {
    FixtureHttpTransport transport;
    transport.addFixture("search.json", readFixture("thrustcurve_search_estes_c6.json"));
    ThrustCurveClient client(transport);

    const QVector<MotorSummary> results =
        client.searchMotors({"Estes", "C6", 5});

    REQUIRE(results.size() == 1);
    const MotorSummary& m = results.first();
    CHECK(m.motorId == "5f4294d20002310000000015");
    CHECK(m.manufacturer == "Estes Industries");
    CHECK(m.designation == "C6");
    CHECK(m.impulseClass == "C");
    CHECK(m.diameterMm == Catch::Approx(18.0));
    CHECK(m.avgThrustN == Catch::Approx(4.74));
    CHECK(m.totImpulseNs == Catch::Approx(8.82));
    CHECK(m.burnTimeS == Catch::Approx(1.86));
    CHECK(m.delays == "0,3,5,7");
    CHECK(m.delayAdjustable == false);
    CHECK(m.propWeightG == Catch::Approx(10.8));
}

TEST_CASE("ThrustCurveClient parses a real download.json samples response", "[data][thrustcurve]") {
    FixtureHttpTransport transport;
    transport.addFixture("download.json", readFixture("thrustcurve_download_samples.json"));
    ThrustCurveClient client(transport);

    const auto simfile = client.downloadSamples("5f4294d20002310000000015");
    REQUIRE(simfile.has_value());
    CHECK(simfile->format == "RASP");
    CHECK(simfile->source == "cert");
    REQUIRE(simfile->samples.size() > 10);
    CHECK(simfile->samples.first().timeS == Catch::Approx(0.031));
    CHECK(simfile->samples.first().thrustN == Catch::Approx(0.946));
    CHECK(simfile->samples.last().timeS == Catch::Approx(1.86));
    CHECK(simfile->samples.last().thrustN == Catch::Approx(0.0));
}

TEST_CASE("ThrustCurveClient returns empty/nullopt on a 404", "[data][thrustcurve]") {
    FixtureHttpTransport transport;  // no fixtures registered -> every request 404s
    ThrustCurveClient client(transport);

    CHECK(client.searchMotors({"Nonexistent", "X1", 5}).isEmpty());
    CHECK_FALSE(client.downloadSamples("does-not-exist").has_value());
}
