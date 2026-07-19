#ifndef MYTASKS_H
#define MYTASKS_H

// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – My Tasks Module (Staff side)
//  mytasks.h
//
//  Opened from StaffDashboard::on_btnViewTasks_clicked() as:
//      new MyTasks(m_staffId, m_staffName, this)
//  It loads its own data as soon as it's constructed - no separate
//  call is needed from the caller. refreshTasks() is exposed publicly
//  in case a caller wants to re-pull tasks for an already-open window
//  (e.g. after coming back from somewhere tasks might have changed).
//
//  All static layout/styling lives in mytasks.ui (Ui::MyTasks): the
//  greeting banner, the three summary cards, and the scroll area. Only
//  the per-task card list is genuinely dynamic (its count depends on
//  the DB query), so that part is still built in mytasks.cpp - the
//  same reasoning as the Add/View Task dialogs in TaskManagement.
//
//  Reads from the `tasks` table, filtered by staff_id = this staff
//  member's information.id:
//
//     tasks (
//         id, staff_id, staff_name, task_title, task_description,
//         deadline, priority, category, status, created_at
//     )
// ═══════════════════════════════════════════════════════════════════

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MyTasks; }
QT_END_NAMESPACE

class MyTasks : public QMainWindow
{
    Q_OBJECT

public:
    // staffId = the logged-in employee's id from the `information` table.
    // staffName = only used to show "Active Tasks: <name>" on screen.
    explicit MyTasks(int staffId, const QString &staffName, QWidget *parent = nullptr);
    ~MyTasks();

    // Loads/reloads the task list for a given staff member.
    void loadTasksForStaff(int staffId, const QString &staffName);

    // Call this if this window is still open and you want it to pick up
    // any tasks assigned/updated since it was last loaded.
    void refreshTasks();

private:
    // One task card is created per row returned by the DB query. Content
    // and count are only known at runtime, so this can't live in the .ui.
    QWidget *createTaskCard(const QString &title, const QString &description,
                            const QString &priority, const QString &dueText,
                            const QString &statusText) const;

    // Removes every card currently in taskCardsLayout before a reload.
    void clearTaskCards();

    Ui::MyTasks *ui;

    int      m_currentStaffId = -1;
    QString  m_currentStaffName;
};

#endif // MYTASKS_H