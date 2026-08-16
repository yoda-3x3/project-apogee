// Phase 2 demo tool: seeds the local parts/kits database from the embedded
// seed data, then runs a live ThrustCurve.org search to prove the network
// client works end to end (not just against fixtures).
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QStandardPaths>
#include <QTextStream>

#include "data/component_repository.hpp"
#include "data/database.hpp"
#include "data/kit_repository.hpp"
#include "data/motor_repository.hpp"
#include "data/network_http_transport.hpp"
#include "data/seed_loader.hpp"
#include "data/thrustcurve_client.hpp"

using namespace apogee::data;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // seed.qrc lives in rocket_data, a STATIC library -- without this, its
    // resource-registering initializer never gets linked in and
    // :/seed/*.json silently doesn't exist at runtime (see tests/main.cpp).
    Q_INIT_RESOURCE(seed);
    QTextStream out(stdout);

    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + "/apogee.sqlite";

    out << "Opening database at " << dbPath << "\n";
    Database db = Database::open(dbPath);
    if (!db.handle().isOpen()) {
        out << "Failed to open database: " << db.handle().lastError().text() << "\n";
        return 1;
    }

    seedIfEmpty(db.handle());

    ComponentRepository components(db.handle());
    KitRepository kits(db.handle());
    MotorRepository motors(db.handle());
    out << "Components: " << components.componentCount() << "\n";
    out << "Kits: " << kits.kitCount() << "\n";
    out << "Cached motors: " << motors.motorCount() << "\n\n";

    out << "Searching ThrustCurve.org for Estes C6...\n";
    NetworkHttpTransport transport;
    ThrustCurveClient client(transport);
    const QVector<MotorSummary> results = client.searchMotors({"Estes", "C6", 5});

    if (results.isEmpty()) {
        out << "No results (network unavailable?)\n";
        return 1;
    }

    for (const MotorSummary& motor : results) {
        out << "  " << motor.manufacturer << " " << motor.designation
            << " -- avg " << motor.avgThrustN << " N, total impulse " << motor.totImpulseNs
            << " Ns, burn " << motor.burnTimeS << " s\n";
        motors.upsertMotor(motor);

        const auto simfile = client.downloadSamples(motor.motorId);
        if (simfile) {
            out << "    cached " << simfile->samples.size() << " thrust samples ("
                << simfile->format << ", " << simfile->source << ")\n";
            const qint64 motorRowId = motors.upsertMotor(motor);
            motors.upsertSimfile(motorRowId, *simfile);
        }
    }

    out << "\nCached motors after search: " << motors.motorCount() << "\n";
    return 0;
}
