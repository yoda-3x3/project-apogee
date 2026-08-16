#include "data/motor_repository.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace apogee::data {

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

}  // namespace apogee::data
