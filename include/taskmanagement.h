#ifndef TASKMANAGEMENT_H
#define TASKMANAGEMENT_H

// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Task Management Module (Admin side)
//  taskmanagement.h
//
//  All static layout/styling now lives in taskmanagement.ui (Ui::TaskManagement).
//  This class only contains logic: DB access, row population, filtering,
//  and the Add Task / View Task dialogs. Those two dialogs are still built
//  in taskmanagement.cpp because their content is generated per-row at
//  runtime (title, staff name, task list) and Qt Designer cannot express
//  "one .ui that re-populates itself differently every time it opens" -
//  this is normal Qt practice for data-driven dialogs.
//
//  Tasks are joined to staff by staff_id (information.id), not by the
//  name string, so a name change in `information` never orphans old
//  tasks. Schema (only INSERT / UPDATE / SELECT are used here):
//
//     tasks (
//         id                INTEGER PRIMARY KEY AUTOINCREMENT,
//         staff_id          INTEGER,          -- FK -> information.id
//         staff_name        TEXT,             -- kept for display/debug only
//         task_title        TEXT,
//         task_description  TEXT,
//         deadline          TEXT,
//         priority          TEXT,
//         category          TEXT,
//         status            TEXT,
//         created_at        TEXT
//     )
//
//     information (
//         id, first_name, last_name, role, ...
//     )
// ═══════════════════════════════════════════════════════════════════

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class TaskManagement; }
QT_END_NAMESPACE

class QPushButton;

class TaskManagement : public QMainWindow
{
    Q_OBJECT

public:
    explicit TaskManagement(QWidget *parent = nullptr);
    ~TaskManagement();

signals:
    void taskDataChanged();

private slots:
    void onSearchFilterChanged();
    void onRowAddTaskClicked();
    void onRowViewTaskClicked();

private:
    // One task record kept in memory for "View Task"; `id` is the DB row id.
    struct TaskDetail {
        int     id = -1;
        QString title;
        QString description;
        QString priority;
        QString dueDate;
        QString status;
    };

    // ── Dialogs (data-driven, built at runtime — see note above) ──────
    QWidget *buildTaskDetailCard(const TaskDetail &task, int staffId);

    // ── Loading / row helpers ─────────────────────────────────────────
    void loadStaffFromDatabase();
    void loadTasksFromDatabase();
    void insertTaskRow(int staffId, const QString &staff, const QString &total,
                       const QString &pending, const QString &completed);
    void attachRowActionButtons(int row, int staffId);
    int  rowForActionButton(QPushButton *button) const;
    int  staffIdForRow(int row) const;
    void updateSummaryMetrics();

    // ── Database operations (INSERT / UPDATE / SELECT only) ──────────
    bool insertTask(int staffId, const QString &staff, const QString &title,
                    const QString &description, const QString &priority,
                    const QString &category, const QString &dueDate,
                    const QString &status, int *outId = nullptr);
    bool updateTaskStatus(int taskId, const QString &newStatus);

    Ui::TaskManagement *ui;

    // Local cache of task details, keyed by staff_id (information.id)
    // instead of the name string, so renames can't break the association.
    QMap<int, QVector<TaskDetail>> staffTaskDetails;
};

#endif // TASKMANAGEMENT_H