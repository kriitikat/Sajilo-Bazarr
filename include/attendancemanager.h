#ifndef ATTENDANCEMANAGER_H
#define ATTENDANCEMANAGER_H

#include <QString>
#include <QDate>

// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Attendance helper
//
//  Centralises the "what counts as on-time / late / absent" business
//  rules so login.cpp (which only ever WRITES check-in/out timestamps)
//  and staffperform.cpp (which only ever READS them back into stats)
//  don't each carry their own copy of the shift/grace-period constants.
//
//  Schema (created in main.cpp; only INSERT / UPDATE / SELECT used here):
//
//     attendance (
//         id             INTEGER PRIMARY KEY AUTOINCREMENT,
//         staff_id       INTEGER NOT NULL,   -- FK -> information.id
//         date           TEXT NOT NULL,      -- 'yyyy-MM-dd'
//         check_in_time  TEXT,               -- 'HH:mm:ss'
//         check_out_time TEXT,               -- 'HH:mm:ss', NULL until sign-out
//         UNIQUE(staff_id, date)
//     )
// ═══════════════════════════════════════════════════════════════════

namespace AttendanceManager {

// Shift window used to classify a day as on-time vs irregular. Adjust
// these two if Sajilo Bazar's actual staff shift hours differ.
constexpr const char *kShiftStart = "09:00:00"; // expected check-in
constexpr const char *kShiftEnd   = "17:00:00"; // expected check-out

// Grace periods either side of the shift boundaries before a day gets
// flagged as irregular. A check-in at 09:10 is fine; 09:20 is late.
constexpr int kLateGraceMinutes  = 15; // check-in after 09:15  -> "late"
constexpr int kEarlyGraceMinutes = 15; // check-out before 16:45 -> "early"

// Nepal's standard weekly-off day for most businesses (matches this
// app's own seed data / audience) is Saturday, not Sunday. computeStats()
// skips this day entirely - it never counts toward present, irregular,
// or absent.
constexpr Qt::DayOfWeek kWeeklyOff = Qt::Saturday;

// Call once, right after a successful login for a 'staff' or 'frontdesk'
// account (see Login::openDashboard). Uses INSERT OR IGNORE keyed on
// (staff_id, date), so logging in more than once on the same day never
// overwrites that day's original check-in time.
void recordCheckIn(int staffId);

// Call when that user's dashboard window is destroyed/closed. Wired up
// via QObject::destroyed in Login::openDashboard rather than requiring
// any change inside StaffDashboard/frontdesk itself. Only ever touches
// *today's* row, and only if it doesn't already have a check-out time
// recorded (so re-showing/re-closing a window the same day keeps the
// first close-out, not the last).
void recordCheckOut(int staffId);

// Aggregate attendance counts for one staff member over an inclusive
// date range, already classified into the three buckets shown on the
// Staff Performance page.
struct AttendanceStats {
    int presentDays   = 0; // checked in AND checked out within shift hours
    int irregularDays = 0; // checked in, but late in and/or checked out early
    int absentDays    = 0; // no check-in row at all on a working day
};

// Walks every working day (i.e. every day that isn't kWeeklyOff) from
// `from` to min(to, today) inclusive and classifies each one for
// `staffId` using the attendance table. A day with no row is "absent";
// today's row missing only a check-out (shift still in progress) is
// never penalized as "early" since there's nothing to compare yet.
AttendanceStats computeStats(int staffId, const QDate &from, const QDate &to);

} // namespace AttendanceManager

#endif // ATTENDANCEMANAGER_H
