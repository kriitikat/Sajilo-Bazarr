#ifndef STAFFPERFORM_H
#define STAFFPERFORM_H

// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Staff Performance Module (Admin side)
//  staffperform.h
//
//  Previously opened as its own top-level QMainWindow (via show()),
//  which is what caused two separate windows to appear alongside the
//  Admin Dashboard. This page is now a plain QWidget - the same base
//  class category.h uses - so it can be embedded directly as a page
//  inside AdminDashboard's stacked widget instead of popping up a
//  second window. Whatever creates this widget should add it as a page
//  and connect backToDashboardRequested() to switch back, rather than
//  calling show() on it directly.
//
//  One row per staff/frontdesk account, combining:
//
//    - Task completion, read from `tasks` (joined by staff_id, same as
//      taskmanagement.cpp) — split into Incomplete (Pending/In Progress)
//      and Completed.
//    - Attendance, read from `attendance` via AttendanceManager - split
//      into Present / Irregular / Absent working days for the current
//      calendar month to date. See attendancemanager.h for exactly what
//      "irregular" (late check-in or early check-out) means and for the
//      weekly-off day used when counting absences.
//
//  All static layout/styling lives in staffperform.ui (Ui::StaffPerform).
//  This class only contains logic: DB access and row population.
// ═══════════════════════════════════════════════════════════════════

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class StaffPerform; }
QT_END_NAMESPACE

class StaffPerform : public QWidget
{
    Q_OBJECT

public:
    explicit StaffPerform(QWidget *parent = nullptr);
    ~StaffPerform();

signals:
    // Emitted when the "Back to Dashboard" button is clicked. Whatever
    // owns this page (e.g. AdminDashboard's QStackedWidget) should connect
    // to this and switch back to the dashboard page - this class does not
    // know about AdminDashboard directly, keeping it reusable/decoupled.
    void backToDashboardRequested();

private slots:
    // Reloads every row from `information` + `tasks` + `attendance`.
    void loadPerformanceTable();

    // Same semantics as staff.cpp's staff::disableStaff/expireStaff - both
    // act directly on the shared `information` table, so a staff member
    // disabled/expired from here is exactly as disabled/expired as one
    // toggled from the Staff page (and vice versa).
    void disableStaff(int staffId); // toggles status between 'enabled' and 'disabled'
    void expireStaff(int staffId);  // toggles status between 'expired' and 'enabled'

private:
    Ui::StaffPerform *ui;

    // Task counts for one staff_id, read from `tasks`.
    void taskCounts(int staffId, int *incomplete, int *completed) const;

    // Colors an ACTIONS-less data cell (no widgets, just item text/color)
    // based on whether the count is a "good" or "bad" signal for that
    // column (e.g. 0 absent days is good, any absent days is a flag).
    void setCountCell(int row, int col, int value, bool flagIfPositive);

    // Builds the ACTIONS cell widget (Disable/Enable + Expire/Unexpire
    // buttons) for a given row + staff id, mirroring staff.cpp's
    // buildActionsWidget. Styling lives in staffperform.ui via the
    // "actionRole" dynamic property, same convention as staff.ui.
    QWidget *buildActionsWidget(int staffId, const QString &status);
};

#endif // STAFFPERFORM_H