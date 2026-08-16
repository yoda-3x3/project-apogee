#include <catch_amalgamated.hpp>

#include <QSqlQuery>

#include "data/database.hpp"

using apogee::data::Database;

TEST_CASE("schema creates every expected table", "[data][schema]") {
    Database db = Database::open(":memory:");
    REQUIRE(db.handle().isOpen());

    const QStringList expectedTables = {
        "motors",
        "motor_simfiles",
        "motor_thrust_samples",
        "components",
        "component_nose_cones",
        "component_body_tubes",
        "component_transitions",
        "component_fin_sets",
        "component_parachutes",
        "component_streamers",
        "component_motor_mounts",
        "kits",
        "kit_components",
    };

    for (const QString& table : expectedTables) {
        QSqlQuery query(db.handle());
        query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
        query.addBindValue(table);
        REQUIRE(query.exec());
        INFO("missing table: " << table.toStdString());
        REQUIRE(query.next());
    }
}
