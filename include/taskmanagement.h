#ifndef TASKMANAGEMENT_H
#define TASKMANAGEMENT_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class TaskManagement; }
QT_END_NAMESPACE

// Forward declarations of Qt classes to keep compilation fast. Most static
// widgets (labels, the search box, the table) now live in taskmanagement.ui
// and are reached via ui->; these are only needed for the still-dynamic
// bits built at runtime (per-staff attendance cards, per-row action
// buttons, and the optional/unwired assign-task form).
class QTextEdit;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
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

    // Attendance state for one staff member. Currently seeded with sample
    // data so the panel looks alive out of the box, but it's kept separate
    // from TaskDetail on purpose: attendance is a per-staff/per-day concept,
    // not a per-task one. Wire this up to real sessions later by having the
    // Login window call recordLogin()/recordLogout() on the live
    // TaskManagement instance (e.g. via a signal emitted from Login on a
    // successful sign-in), and call recordCheckIn() from wherever "arrived
    // at work" is captured (front-desk kiosk, staff dashboard button, etc).
    struct StaffAttendance {
        bool    online = false;
        QString loginTime;    // e.g. "9:02 AM" — most recent login, empty = never
        QString checkInTime;  // e.g. "9:05 AM" — today's physical check-in
        QString logoutTime;   // e.g. "5:30 PM" — most recent logout, empty = still in
        QString hoursToday;   // e.g. "3h 15m" — running or last completed duration
    };

    explicit TaskManagement(QWidget *parent = nullptr);
    ~TaskManagement();

public slots:
    // Public on purpose: call these from wherever a staff member's real
    // session events happen so the attendance panel reflects reality
    // instead of the seeded sample data.
    void recordLogin(const QString &staffName);
    void recordCheckIn(const QString &staffName);
    void recordLogout(const QString &staffName);

private slots:
    // Assign-task form actions
    void onAssignTaskClicked();
    void onClearFieldsClicked();

    // Search box action
    void onSearchFilterChanged();

    // Per-row action button actions (Action column of the table)
    void onRowAddTaskClicked();
    void onRowViewTaskClicked();

    // Attendance card quick-action buttons (manual/demo triggers for the
    // same recordLogin/recordCheckIn/recordLogout logic above)
    void onAttendanceLoginClicked();
    void onAttendanceCheckInClicked();
    void onAttendanceLogoutClicked();

private:
    // ---- setup helpers (keep the constructor short & readable) ----
    // The header bar, summary cards, and table headers are all static
    // widgets defined in taskmanagement.ui; these just wire up signals and
    // runtime-only configuration (column resize modes, staff cards) on top
    // of what setupUi() already built.
    void setupSummaryPanel();       // connects the search box
    void setupAttendancePanel();    // builds one card per staff into ui->attendanceCardsLayout
    void setupTaskTablePanel();     // column resize modes, header sizing
    QWidget *buildAssignTaskForm(); // form used to assign a brand-new task (unused, kept optional)

    // ---- table / row helpers ----
    void insertTaskRow(const QString &staff, const QString &total,
                       const QString &pending, const QString &completed);
    void attachRowActionButtons(int row);
    int  rowForActionButton(QPushButton *button) const;
    void updateSummaryMetrics();
    QWidget *buildTaskDetailCard(const TaskDetail &task) const;

    // ---- attendance helpers ----
    QWidget *buildAttendanceCard(const QString &staffName);
    void refreshAttendanceCard(const QString &staffName);
    void refreshStatusInTable(const QString &staffName);
    QString currentTimeLabel() const;

    Ui::TaskManagement *ui;

    // Assign-task form widgets (only ever populated if buildAssignTaskForm()
    // is wired up somewhere — currently unused/optional, same as before)
    QLineEdit *taskTitleInput   = nullptr;
    QTextEdit *descriptionInput = nullptr;
    QComboBox *assigneeDropdown = nullptr;
    QComboBox *priorityDropdown = nullptr;
    QDateEdit *dueDateInput     = nullptr;
    QComboBox *categoryDropdown = nullptr;

    // Brief task details keyed by staff name, populated whenever a task is
    // assigned so "View Task" can show title/description/priority/due date.
    QMap<QString, QVector<TaskDetail>> staffTaskDetails;

    // Attendance state keyed by staff name, plus the live label widgets for
    // each staff's card so a login/check-in/logout event can refresh just
    // that one card instead of rebuilding the whole panel.
    QMap<QString, StaffAttendance> staffAttendance;
    QMap<QString, QLabel *> attendanceDot;
    QMap<QString, QLabel *> attendanceStatusText;
    QMap<QString, QLabel *> attendanceLoginLabel;
    QMap<QString, QLabel *> attendanceCheckInLabel;
    QMap<QString, QLabel *> attendanceLogoutLabel;
    QMap<QString, QLabel *> attendanceHoursLabel;
    QMap<QString, QPushButton *> attendanceLoginButton;
    QMap<QString, QPushButton *> attendanceCheckInButton;
    QMap<QString, QPushButton *> attendanceLogoutButton;

    // Row lookup so the Status/Login Time columns in the main table can be
    // updated in place after an attendance event, keyed by staff name.
    QMap<QString, int> staffRowIndex;
};

#endif // TASKMANAGEMENT_H