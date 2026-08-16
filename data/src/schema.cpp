#include "data/schema.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

namespace apogee::data {

namespace {

// clang-format off
const QStringList kStatements = {
    R"(CREATE TABLE IF NOT EXISTS motors (
        id                    INTEGER PRIMARY KEY,
        thrustcurve_motor_id  TEXT UNIQUE NOT NULL,
        manufacturer          TEXT, manufacturer_abbrev TEXT,
        designation           TEXT, common_name TEXT,
        impulse_class         TEXT,
        diameter_mm           REAL, length_mm REAL,
        motor_type            TEXT,
        cert_org              TEXT,
        avg_thrust_n          REAL, max_thrust_n REAL,
        tot_impulse_ns        REAL, burn_time_s REAL,
        total_weight_g        REAL, prop_weight_g REAL,
        delays                TEXT,
        delay_adjustable      INTEGER, prop_info TEXT, sparky INTEGER,
        availability          TEXT, info_url TEXT, updated_on TEXT,
        cached_at             TEXT NOT NULL
    ))",
    R"(CREATE TABLE IF NOT EXISTS motor_simfiles (
        id                      INTEGER PRIMARY KEY,
        motor_id                INTEGER NOT NULL REFERENCES motors(id),
        thrustcurve_simfile_id  TEXT,
        format                  TEXT,
        source                  TEXT,
        data_url                TEXT, info_url TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS motor_thrust_samples (
        id           INTEGER PRIMARY KEY,
        simfile_id   INTEGER NOT NULL REFERENCES motor_simfiles(id),
        seq          INTEGER NOT NULL,
        time_s       REAL NOT NULL,
        thrust_n     REAL NOT NULL
    ))",
    R"(CREATE TABLE IF NOT EXISTS components (
        id            INTEGER PRIMARY KEY,
        type          TEXT NOT NULL,
        manufacturer  TEXT, name TEXT NOT NULL, sku TEXT,
        mass_g        REAL, price_usd REAL, notes TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_nose_cones (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        shape TEXT,
        length_mm REAL, base_diameter_mm REAL,
        shoulder_length_mm REAL, shoulder_diameter_mm REAL, material TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_body_tubes (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        outer_diameter_mm REAL, inner_diameter_mm REAL, length_mm REAL, material TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_transitions (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        fore_diameter_mm REAL, aft_diameter_mm REAL, length_mm REAL,
        fore_shoulder_length_mm REAL, aft_shoulder_length_mm REAL, material TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_fin_sets (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        fin_count INTEGER, root_chord_mm REAL, tip_chord_mm REAL,
        semi_span_mm REAL, sweep_length_mm REAL, thickness_mm REAL,
        mounting_diameter_mm REAL, cross_section TEXT, material TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_parachutes (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        diameter_mm REAL, cd REAL DEFAULT 0.75, shroud_lines INTEGER
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_streamers (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        length_mm REAL, width_mm REAL, cd REAL DEFAULT 1.0
    ))",
    R"(CREATE TABLE IF NOT EXISTS component_motor_mounts (
        component_id INTEGER PRIMARY KEY REFERENCES components(id),
        motor_diameter_mm REAL, mount_length_mm REAL, centering_ring_count INTEGER
    ))",
    R"(CREATE TABLE IF NOT EXISTS kits (
        id INTEGER PRIMARY KEY, manufacturer TEXT, name TEXT NOT NULL, sku TEXT, description TEXT
    ))",
    R"(CREATE TABLE IF NOT EXISTS kit_components (
        kit_id INTEGER REFERENCES kits(id), component_id INTEGER REFERENCES components(id),
        quantity INTEGER DEFAULT 1, role TEXT
    ))",
};
// clang-format on

}  // namespace

bool createSchema(QSqlDatabase& db) {
    for (const QString& statement : kStatements) {
        QSqlQuery query(db);
        if (!query.exec(statement)) {
            return false;
        }
    }
    return true;
}

}  // namespace apogee::data
