#include <catch_amalgamated.hpp>

#include <QSqlQuery>

#include "data/component_repository.hpp"
#include "data/database.hpp"
#include "data/kit_repository.hpp"
#include "data/seed_loader.hpp"

using namespace apogee::data;

TEST_CASE("seedIfEmpty populates components and kits exactly once", "[data][seed]") {
    Database db = Database::open(":memory:");

    seedIfEmpty(db.handle());

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

    SECTION("re-seeding an already-populated database is a no-op") {
        seedIfEmpty(db.handle());
        REQUIRE(components.componentCount() == componentCountAfterFirstSeed);
        REQUIRE(kits.kitCount() == kitCountAfterFirstSeed);
    }
}
