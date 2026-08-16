#include "data/motor_repository.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace apogee::data {

namespace {
MotorSummary motorFromQuery(const QSqlQuery& q) {
    MotorSummary m;
    m.id = q.value("id").toLongLong();
    m.motorId = q.value("thrustcurve_motor_id").toString();
    m.manufacturer = q.value("manufacturer").toString();
    m.manufacturerAbbrev = q.value("manufacturer_abbrev").toString();
    m.designation = q.value("designation").toString();
    m.commonName = q.value("common_name").toString();
    m.impulseClass = q.value("impulse_class").toString();
    m.diameterMm = q.value("diameter_mm").toDouble();
    m.lengthMm = q.value("length_mm").toDouble();
    m.motorType = q.value("motor_type").toString();
    m.certOrg = q.value("cert_org").toString();
    m.avgThrustN = q.value("avg_thrust_n").toDouble();
    m.maxThrustN = q.value("max_thrust_n").toDouble();
    m.totImpulseNs = q.value("tot_impulse_ns").toDouble();
    m.burnTimeS = q.value("burn_time_s").toDouble();
    m.totalWeightG = q.value("total_weight_g").toDouble();
    m.propWeightG = q.value("prop_weight_g").toDouble();
    m.delays = q.value("delays").toString();
    m.delayAdjustable = q.value("delay_adjustable").toInt() != 0;
    m.propInfo = q.value("prop_info").toString();
    m.sparky = q.value("sparky").toInt() != 0;
    m.availability = q.value("availability").toString();
    m.infoUrl = q.value("info_url").toString();
    m.updatedOn = q.value("updated_on").toString();
    return m;
}
}  // namespace

MotorRepository::MotorRepository(QSqlDatabase& db) : db_(db) {}

qint64 MotorRepository::upsertMotor(const MotorSummary& motor) {
    QSqlQuery find(db_);
    find.prepare("SELECT id FROM motors WHERE thrustcurve_motor_id = ?");
    find.addBindValue(motor.motorId);
    find.exec();

    const QString nowIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const bool exists = find.next();
    const qint64 existingId = exists ? find.value(0).toLongLong() : -1;

    QSqlQuery query(db_);
    if (exists) {
        query.prepare(
            "UPDATE motors SET manufacturer=?, manufacturer_abbrev=?, designation=?, "
            "common_name=?, impulse_class=?, diameter_mm=?, length_mm=?, motor_type=?, "
            "cert_org=?, avg_thrust_n=?, max_thrust_n=?, tot_impulse_ns=?, burn_time_s=?, "
            "total_weight_g=?, prop_weight_g=?, delays=?, delay_adjustable=?, prop_info=?, "
            "sparky=?, availability=?, info_url=?, updated_on=?, cached_at=? "
            "WHERE id=?");
    } else {
        query.prepare(
            "INSERT INTO motors (thrustcurve_motor_id, manufacturer, manufacturer_abbrev, "
            "designation, common_name, impulse_class, diameter_mm, length_mm, motor_type, "
            "cert_org, avg_thrust_n, max_thrust_n, tot_impulse_ns, burn_time_s, "
            "total_weight_g, prop_weight_g, delays, delay_adjustable, prop_info, sparky, "
            "availability, info_url, updated_on, cached_at) "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(motor.motorId);
    }
    query.addBindValue(motor.manufacturer);
    query.addBindValue(motor.manufacturerAbbrev);
    query.addBindValue(motor.designation);
    query.addBindValue(motor.commonName);
    query.addBindValue(motor.impulseClass);
    query.addBindValue(motor.diameterMm);
    query.addBindValue(motor.lengthMm);
    query.addBindValue(motor.motorType);
    query.addBindValue(motor.certOrg);
    query.addBindValue(motor.avgThrustN);
    query.addBindValue(motor.maxThrustN);
    query.addBindValue(motor.totImpulseNs);
    query.addBindValue(motor.burnTimeS);
    query.addBindValue(motor.totalWeightG);
    query.addBindValue(motor.propWeightG);
    query.addBindValue(motor.delays);
    query.addBindValue(motor.delayAdjustable ? 1 : 0);
    query.addBindValue(motor.propInfo);
    query.addBindValue(motor.sparky ? 1 : 0);
    query.addBindValue(motor.availability);
    query.addBindValue(motor.infoUrl);
    query.addBindValue(motor.updatedOn);
    query.addBindValue(nowIso);
    if (exists) query.addBindValue(existingId);
    query.exec();

    return exists ? existingId : query.lastInsertId().toLongLong();
}

qint64 MotorRepository::upsertSimfile(qint64 motorId, const MotorSimfile& simfile) {
    // Replace: drop any previously cached simfile(s)/samples for this motor,
    // then insert the fresh one. Keeps a "Refresh" action simple and correct
    // rather than trying to diff sample sets.
    QSqlQuery findOld(db_);
    findOld.prepare("SELECT id FROM motor_simfiles WHERE motor_id = ?");
    findOld.addBindValue(motorId);
    findOld.exec();
    while (findOld.next()) {
        const qint64 oldSimfileId = findOld.value(0).toLongLong();
        QSqlQuery deleteSamples(db_);
        deleteSamples.prepare("DELETE FROM motor_thrust_samples WHERE simfile_id = ?");
        deleteSamples.addBindValue(oldSimfileId);
        deleteSamples.exec();
    }
    QSqlQuery deleteSimfiles(db_);
    deleteSimfiles.prepare("DELETE FROM motor_simfiles WHERE motor_id = ?");
    deleteSimfiles.addBindValue(motorId);
    deleteSimfiles.exec();

    QSqlQuery insertSimfile(db_);
    insertSimfile.prepare(
        "INSERT INTO motor_simfiles (motor_id, thrustcurve_simfile_id, format, source, "
        "data_url, info_url) VALUES (?,?,?,?,?,?)");
    insertSimfile.addBindValue(motorId);
    insertSimfile.addBindValue(simfile.simfileId);
    insertSimfile.addBindValue(simfile.format);
    insertSimfile.addBindValue(simfile.source);
    insertSimfile.addBindValue(simfile.dataUrl);
    insertSimfile.addBindValue(simfile.infoUrl);
    insertSimfile.exec();
    const qint64 simfileId = insertSimfile.lastInsertId().toLongLong();

    QSqlQuery insertSample(db_);
    insertSample.prepare(
        "INSERT INTO motor_thrust_samples (simfile_id, seq, time_s, thrust_n) VALUES (?,?,?,?)");
    for (int i = 0; i < simfile.samples.size(); ++i) {
        insertSample.addBindValue(simfileId);
        insertSample.addBindValue(i);
        insertSample.addBindValue(simfile.samples[i].timeS);
        insertSample.addBindValue(simfile.samples[i].thrustN);
        insertSample.exec();
    }

    return simfileId;
}

int MotorRepository::motorCount() {
    QSqlQuery query(db_);
    query.exec("SELECT COUNT(*) FROM motors");
    query.next();
    return query.value(0).toInt();
}

QVector<MotorSummary> MotorRepository::listAll() {
    QVector<MotorSummary> result;
    QSqlQuery query(db_);
    query.exec("SELECT * FROM motors ORDER BY manufacturer, designation");
    while (query.next()) {
        result.push_back(motorFromQuery(query));
    }
    return result;
}

std::optional<MotorSummary> MotorRepository::getById(qint64 id) {
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM motors WHERE id = ?");
    query.addBindValue(id);
    query.exec();
    if (!query.next()) return std::nullopt;
    return motorFromQuery(query);
}

QVector<ThrustSample> MotorRepository::getCachedSamples(qint64 motorId) {
    QVector<ThrustSample> result;
    QSqlQuery findSimfile(db_);
    findSimfile.prepare("SELECT id FROM motor_simfiles WHERE motor_id = ? LIMIT 1");
    findSimfile.addBindValue(motorId);
    findSimfile.exec();
    if (!findSimfile.next()) return result;
    const qint64 simfileId = findSimfile.value(0).toLongLong();

    QSqlQuery query(db_);
    query.prepare("SELECT time_s, thrust_n FROM motor_thrust_samples WHERE simfile_id = ? ORDER BY seq");
    query.addBindValue(simfileId);
    query.exec();
    while (query.next()) {
        result.push_back({query.value(0).toDouble(), query.value(1).toDouble()});
    }
    return result;
}

}  // namespace apogee::data
