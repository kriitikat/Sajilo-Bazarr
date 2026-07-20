#ifndef TASKMANAGEMENT_H
#define TASKMANAGEMENT_H

// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Task Management Module (Admin side)
//  taskmanagement.h
//
//  Hard split between the two files:
//    - taskmanagement.ui   -> ALL design/coloring. Every color, font
//                             weight, border, radius, hover/pressed/
//                             focus state used anywhere in this module
//                             lives in ui->styleSheet as one QSS block.
//    - taskmanagement.cpp  -> ALL logic. DB access, row population,
//                             filtering, the Add Task / View Task
//                             dialogs. It never contains a hex color
//                             or a QSS string.
//
//  The Add Task / View Task dialogs are still assembled with widget
//  calls in the .cpp because their content is generated per-row at
//  runtime (title, staff name, variable-length task list) and Qt
//  Designer cannot express "one .ui that re-populates itself
//  differently every time it opens" - this is normal Qt practice for
//  data-driven dialogs. What changed is *how* they're styled: instead
//  of building QSS strings with colors in the .cpp, each dynamically
//  created widget is tagged with a "role" dynamic property (e.g.
//  setProperty("role", "primary")), and taskmanagement.ui's master
//  stylesheet (copied onto every dialog via
//  dialog.setStyleSheet(this->styleSheet())) matches on that role,
//  e.g. QPushButton[role="primary"]. See taskmanagement.ui for the
//  full list of roles and their styling.
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
