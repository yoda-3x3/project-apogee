#include <catch_amalgamated.hpp>

#include <QSqlQuery>

#include "data/component_repository.hpp"
#include "data/database.hpp"
#include "data/kit_repository.hpp"
#include "data/seed_loader.hpp"

using namespace apogee::data;

TEST_CASE("seedIfNeeded populates components and kits", "[data][seed]") {
    Database db = Database::open(":memory:");

    seedIfNeeded(db.handle());

    ComponentRepository components(db.handle());
    KitRepository kits(db.handle());
    const int componentCountAfterFirstSeed = components.componentCount();
    const int kitCountAfterFirstSeed = kits.kitCount();

    REQUIRE(componentCountAfterFirstSeed > 0);
    REQUIRE(kitCountAfterFirstSeed > 0);

    SECTION("a nose cone's detail row was inserted alongside it") {
        QSqlQuery query(db.handle());
        REQUIRE(query.exec(
            "SELECT c.id FROM components c "
            "JOIN component_nose_cones n ON n.component_id = c.id "
            "WHERE c.sku = 'PNC-50KA'"));
        REQUIRE(query.next());
    }

    SECTION("a kit's components resolve to real component rows") {
        QSqlQuery query(db.handle());
        REQUIRE(query.exec(
            "SELECT COUNT(*) FROM kit_components kc "
            "JOIN kits k ON k.id = kc.kit_id "
            "JOIN components c ON c.id = kc.component_id "
            "WHERE k.sku = 'EST-2456'"));
        query.next();
        REQUIRE(query.value(0).toInt() == 5);  // Alpha III: nose, tube, fins, mount, chute
    }

    SECTION("re-seeding an already-current database is a no-op") {
        seedIfNeeded(db.handle());
        REQUIRE(components.componentCount() == componentCountAfterFirstSeed);
        REQUIRE(kits.kitCount() == kitCountAfterFirstSeed);
    }

    SECTION("a database seeded by an older catalog version gets wiped and reseeded") {
        // Simulate an install that was seeded before the embedded seed data
        // changed (see seed_loader.cpp's kSeedVersion comment) -- e.g. the
        // OpenRocket catalog import that replaced the original ~25-part
        // hand-curated set. A plain "componentCount() > 0" guard would leave
        // this database stuck on the stale data forever.
        QSqlQuery downgrade(db.handle());
        REQUIRE(downgrade.exec("UPDATE seed_meta SET value = '1' WHERE key = 'seed_version'"));
        REQUIRE(downgrade.exec("DELETE FROM kit_components"));
        REQUIRE(downgrade.exec("DELETE FROM kits"));
        REQUIRE(downgrade.exec("DELETE FROM component_nose_cones WHERE component_id NOT IN "
                                "(SELECT id FROM components LIMIT 1)"));
        REQUIRE(downgrade.exec("DELETE FROM components WHERE id NOT IN "
                                "(SELECT id FROM components LIMIT 1)"));
        REQUIRE(components.componentCount() < componentCountAfterFirstSeed);

        // A motor the user cached from ThrustCurve.org, unrelated to the
        // seed JSON, must survive the reseed.
        QSqlQuery insertMotor(db.handle());
        REQUIRE(insertMotor.exec(
            "INSERT INTO motors (thrustcurve_motor_id, manufacturer, designation, cached_at) "
            "VALUES ('test-motor-1', 'Estes', 'C6', '2026-01-01')"));

        seedIfNeeded(db.handle());

        REQUIRE(components.componentCount() == componentCountAfterFirstSeed);
        REQUIRE(kits.kitCount() == kitCountAfterFirstSeed);

        QSqlQuery motorCheck(db.handle());
        REQUIRE(motorCheck.exec("SELECT COUNT(*) FROM motors WHERE thrustcurve_motor_id = 'test-motor-1'"));
        motorCheck.next();
        REQUIRE(motorCheck.value(0).toInt() == 1);
    }
}
