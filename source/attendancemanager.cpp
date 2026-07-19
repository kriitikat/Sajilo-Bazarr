#include "../include/attendancemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QTime>
#include <QMap>
#include <QPair>
#include <QDebug>

namespace AttendanceManager {

void recordCheckIn(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database(); // default connection opened in main()
    if (!db.isOpen()) {
        qWarning() << "AttendanceManager::recordCheckIn - no open database connection.";
        return;
    }

    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString now   = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

    QSqlQuery insert(db);
    insert.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO attendance (staff_id, date, check_in_time) "
        "VALUES (:staff_id, :date, :check_in_time)"));
    insert.bindValue(QStringLiteral(":staff_id"),      staffId);
    insert.bindValue(QStringLiteral(":date"),          today);
    insert.bindValue(QStringLiteral(":check_in_time"), now);

    if (!insert.exec())
        qWarning() << "AttendanceManager::recordCheckIn failed:" << insert.lastError().text();
}

void recordCheckOut(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "AttendanceManager::recordCheckOut - no open database connection.";
        return;
    }

    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString now   = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

    // Only fill an empty check_out_time - if this staff member's window
    // somehow closes twice in one day (or recordCheckOut is ever called
    // more than once), the first close-out stands.
    QSqlQuery update(db);
    update.prepare(QStringLiteral(
        "UPDATE attendance SET check_out_time = :check_out_time "
        "WHERE staff_id = :staff_id AND date = :date "
        "  AND (check_out_time IS NULL OR check_out_time = '')"));
    update.bindValue(QStringLiteral(":check_out_time"), now);
    update.bindValue(QStringLiteral(":staff_id"),        staffId);
    update.bindValue(QStringLiteral(":date"),            today);

    if (!update.exec())
        qWarning() << "AttendanceManager::recordCheckOut failed:" << update.lastError().text();
}

AttendanceStats computeStats(int staffId, const QDate &from, const QDate &to)
{
    AttendanceStats stats;

    const QDate today = QDate::currentDate();
    QDate periodEnd = to;
    if (periodEnd > today)
        periodEnd = today; // never count days that haven't happened yet

    if (!from.isValid() || !periodEnd.isValid() || from > periodEnd)
        return stats;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "AttendanceManager::computeStats - no open database connection.";
        return stats;
    }

    // Pull every attendance row for this staff member in the window in
    // one query instead of one SELECT per day in the loop below.
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT date, check_in_time, check_out_time FROM attendance "
        "WHERE staff_id = :staff_id AND date >= :from_date AND date <= :to_date"));
    query.bindValue(QStringLiteral(":staff_id"),  staffId);
    query.bindValue(QStringLiteral(":from_date"), from.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":to_date"),   periodEnd.toString(Qt::ISODate));

    // date ('yyyy-MM-dd') -> (check_in_time, check_out_time)
    QMap<QString, QPair<QString, QString>> rowsByDate;

    if (query.exec()) {
        while (query.next()) {
            rowsByDate.insert(query.value(0).toString(),
                              qMakePair(query.value(1).toString(), query.value(2).toString()));
        }
    } else {
        qWarning() << "AttendanceManager::computeStats query failed:" << query.lastError().text();
    }

    // Pre-compute the late/early cutoff times once, outside the day loop.
    const QTime lateCutoff  = QTime::fromString(QLatin1String(kShiftStart), QStringLiteral("HH:mm:ss"))
                                  .addSecs(kLateGraceMinutes * 60);
    const QTime earlyCutoff = QTime::fromString(QLatin1String(kShiftEnd), QStringLiteral("HH:mm:ss"))
                                  .addSecs(-kEarlyGraceMinutes * 60);

    for (QDate d = from; d <= periodEnd; d = d.addDays(1)) {
        if (static_cast<Qt::DayOfWeek>(d.dayOfWeek()) == kWeeklyOff)
            continue; // weekly off - never present, irregular, or absent

        const QString key = d.toString(Qt::ISODate);
        const auto it = rowsByDate.constFind(key);

        if (it == rowsByDate.constEnd()) {
            ++stats.absentDays; // no check-in row at all this working day
            continue;
        }

        const QTime checkIn  = QTime::fromString(it.value().first,  QStringLiteral("HH:mm:ss"));
        const QTime checkOut = QTime::fromString(it.value().second, QStringLiteral("HH:mm:ss"));

        const bool lateIn   = checkIn.isValid()  && checkIn  > lateCutoff;
        // Only judge "early" once a check-out actually exists - a shift
        // still in progress (e.g. today, before the user has signed off)
        // must never be counted as an early leave.
        const bool earlyOut = checkOut.isValid() && checkOut < earlyCutoff;

        if (lateIn || earlyOut)
            ++stats.irregularDays;
        else
            ++stats.presentDays;
    }

    return stats;
}

} // namespace AttendanceManager
