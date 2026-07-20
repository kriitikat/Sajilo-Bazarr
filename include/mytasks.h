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
//  All colors/styling live in mytasks.ui (Ui::MyTasks) as one master
//  stylesheet: object-name selectors for the static greeting banner,
//  summary cards, and scroll area, and "role"-tagged selectors (e.g.
//  QPushButton[role="markCompleted"]) for the per-task cards, which
//  are genuinely dynamic - their count and content depend on the DB
//  query, so Designer can't lay them out ahead of time. mytasks.cpp
//  never contains a hex color or QSS string; it only tags widgets
//  with setProperty("role", ...). Same convention as TaskManagement's
//  Add/View Task dialogs.
//
//  Reads from and writes to the `tasks` table, filtered by
//  staff_id = this staff member's information.id:
//
//     tasks (
//         id, staff_id, staff_name, task_title, task_description,
//         deadline, priority, category, status, created_at
//     )
//
//  Marking a task "Completed" here is a plain UPDATE tasks SET
//  status = 'Completed' WHERE id = ? against that same row admin's
//  TaskManagement page reads - so the admin's "View Task" dialog
//  picks up the change automatically the next time it loads that
//  staff member's tasks. No direct link between the two windows is
//  needed or attempted.
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

signals:
    // Fired after a task's status is successfully changed from here, in
    // case a caller (e.g. StaffDashboard) wants to react to it. Mirrors
    // TaskManagement::taskDataChanged().
    void taskStatusChanged();

private:
    // One task card is created per row returned by the DB query. Content
    // and count are only known at runtime, so this can't live in the .ui.
    // Not const: the "Mark Completed" button it wires up needs to call
    // back into updateTaskStatus() and mutate the summary labels.
    QWidget *createTaskCard(int taskId, const QString &title, const QString &description,
                            const QString &priority, const QString &dueText,
                            const QString &statusText);

    // Removes every card currently in taskCardsLayout before a reload.
    void clearTaskCards();

    // Persists a task's new status (UPDATE only) and returns whether it
    // succeeded. Does not touch the on-screen cards or summary counts
    // itself - the caller updates those after a successful write.
    bool updateTaskStatus(int taskId, const QString &newStatus);

    Ui::MyTasks *ui;

    int      m_currentStaffId = -1;
    QString  m_currentStaffName;
};

#endif // MYTASKS_H
