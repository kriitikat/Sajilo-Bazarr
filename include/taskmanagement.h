#ifndef TASKMANAGEMENT_H
#define TASKMANAGEMENT_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QVector>

// Forward declarations of Qt classes to keep compilation fast
class QTableWidget;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QWidget;

class TaskManagement : public QMainWindow
{
    Q_OBJECT

public:
    // Brief detail record for a single assigned task, shown inside the
    // "View Task" dialog.
    struct TaskDetail {
        QString title;
        QString description;
        QString priority;   // Low / Medium / High
        QString dueDate;    // yyyy-MM-dd
        QString status;     // Pending / Completed
    };

    explicit TaskManagement(QWidget *parent = nullptr);
    ~TaskManagement() = default;

private slots:
    // Assign-task form actions
    void onAssignTaskClicked();
    void onClearFieldsClicked();

    // Search box action
    void onSearchFilterChanged();

    // Per-row action button actions (Action column of the table)
    void onRowAddTaskClicked();
    void onRowViewTaskClicked();

private:
    // ---- UI construction helpers (keep the constructor short & readable) ----
    QWidget *buildSummaryPanel();       // Box 1: search + summary metrics
    QWidget *buildAssignTaskForm();     // Box 2: form used to assign a brand-new task (unused, kept optional)
    QWidget *buildTaskTablePanel();     // Box 3: the assigned-tasks table itself

    // ---- table / row helpers ----
    void insertTaskRow(const QString &staff, const QString &total,
                       const QString &pending, const QString &completed);
    void attachRowActionButtons(int row);
    int  rowForActionButton(QPushButton *button) const;
    void updateSummaryMetrics();
    QWidget *buildTaskDetailCard(const TaskDetail &task) const;

    // Search & summary widgets
    QLineEdit *searchBox        = nullptr;
    QLabel    *totalTasksLabel  = nullptr;
    QLabel    *pendingLabel     = nullptr;
    QLabel    *inProgressLabel  = nullptr;
    QLabel    *completedLabel   = nullptr;

    // Assign-task form widgets
    QLineEdit *taskTitleInput   = nullptr;
    QTextEdit *descriptionInput = nullptr;
    QComboBox *assigneeDropdown = nullptr;
    QComboBox *priorityDropdown = nullptr;
    QDateEdit *dueDateInput     = nullptr;
    QComboBox *categoryDropdown = nullptr;

    // Table widget (6 columns: ID, Staff Name, Total, Pending, Completed, Action)
    QTableWidget *assignedTasksTable = nullptr;

    // Brief task details keyed by staff name, populated whenever a task is
    // assigned so "View Task" can show title/description/priority/due date.
    QMap<QString, QVector<TaskDetail>> staffTaskDetails;
};

#endif // TASKMANAGEMENT_H