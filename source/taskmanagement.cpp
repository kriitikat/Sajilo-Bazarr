#include "../include/taskmanagement.h"
#include "../ui/ui_taskmanagement.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFrame>
#include <QScrollArea>
#include <QDate>
#include <QTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QBrush>
#include <QColor>
#include <QFont>

namespace {
// Centralised style strings so every panel/widget looks consistent and is
// easy to tweak from one place.
const QString kPanelStyle =
    "QFrame { background-color:#FFFFFF; border:1px solid #E2E8F0; border-radius:10px; }";

const QString kPanelTitleStyle =
    "font-size:14px; font-weight:bold; color:#2D4A7A; letter-spacing:0.5px; border:none;";

const QString kLabelStyle = "QLabel { font-weight:600; color:#1A202C; border:none; }";

const QString kInputStyle =
    "QLineEdit, QTextEdit, QComboBox, QDateEdit {"
    "  background-color:#FFFFFF; border:1px solid #CBD5E0; color:#000000;"
    "  border-radius:6px; padding:6px; font-size:13px; }"
    "QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QDateEdit:focus {"
    "  border:1px solid #2D4A7A; }"
    "QComboBox QAbstractItemView {"
    "  color:#000000; background-color:#FFFFFF;"
    "  selection-background-color:#EBF2FF; selection-color:#000000; }";

const QString kPrimaryButtonStyle =
    "QPushButton { background-color:#2D4A7A; color:#FFFFFF; border:none;"
    "  border-radius:6px; padding:8px 20px; font-weight:bold; font-size:13px; }"
    "QPushButton:hover { background-color:#223A61; }";

const QString kSecondaryButtonStyle =
    "QPushButton { background-color:#FFFFFF; color:#4A5568; border:1px solid #CBD5E0;"
    "  border-radius:6px; padding:8px 18px; font-size:13px; font-weight:500; }"
    "QPushButton:hover { background-color:#F7FAFC; border-color:#A0AEC0; }";

// Row action buttons get a fixed minimum width so "Add Task" / "View Task"
// text is never clipped, no matter the current column width.
const QString kAddTaskButtonStyle =
    "QPushButton { background-color:#38A169; color:white; border:none;"
    "  border-radius:4px; padding:5px 12px; font-size:12px; font-weight:bold; }"
    "QPushButton:hover { background-color:#2F855A; }";

const QString kViewTaskButtonStyle =
    "QPushButton { background-color:#2D4A7A; color:white; border:none;"
    "  border-radius:4px; padding:5px 12px; font-size:12px; font-weight:bold; }"
    "QPushButton:hover { background-color:#223A61; }";

// Small pill-style buttons used on the attendance cards. Disabled state is
// styled explicitly (rather than left to the platform default) so a greyed
// -out "Log In" on an already-online staff member reads as unavailable at a
// glance, not just as a lighter version of the same button.
const QString kAttendanceLoginButtonStyle =
    "QPushButton { background-color:#38A169; color:white; border:none;"
    "  border-radius:5px; padding:6px 10px; font-size:11px; font-weight:bold; }"
    "QPushButton:hover:!disabled { background-color:#2F855A; }"
    "QPushButton:disabled { background-color:#E2E8F0; color:#A0AEC0; }";

const QString kAttendanceCheckInButtonStyle =
    "QPushButton { background-color:#3182CE; color:white; border:none;"
    "  border-radius:5px; padding:6px 10px; font-size:11px; font-weight:bold; }"
    "QPushButton:hover:!disabled { background-color:#2B6CB0; }"
    "QPushButton:disabled { background-color:#E2E8F0; color:#A0AEC0; }";

const QString kAttendanceLogoutButtonStyle =
    "QPushButton { background-color:#E53E3E; color:white; border:none;"
    "  border-radius:5px; padding:6px 10px; font-size:11px; font-weight:bold; }"
    "QPushButton:hover:!disabled { background-color:#C53030; }"
    "QPushButton:disabled { background-color:#E2E8F0; color:#A0AEC0; }";

QString dashIfEmpty(const QString &value)
{
    return value.isEmpty() ? QStringLiteral("—") : value;
}
}

TaskManagement::TaskManagement(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TaskManagement)
{
    ui->setupUi(this);

    setupSummaryPanel();
    setupAttendancePanel();
    setupTaskTablePanel();

    // ---- sample rows so the page is not empty on first run ----
    insertTaskRow("John Doe",    "10", "4", "6");
    insertTaskRow("Jane Smith",  "5",  "2", "3");
    insertTaskRow("Alex Mercer", "8",  "1", "7");

    // ---- sample task details so "View Task" has something to display ----
    staffTaskDetails["John Doe"] = {
        {"Restock Aisle 4", "Refill shelves with the new shipment of canned goods.",
         "Medium", QDate::currentDate().addDays(2).toString("yyyy-MM-dd"), "Pending"},
        {"Inspect Cold Storage", "Check temperature logs and seal integrity.",
         "High", QDate::currentDate().addDays(-1).toString("yyyy-MM-dd"), "Completed"}
    };
    staffTaskDetails["Jane Smith"] = {
        {"Receive Supplier Delivery", "Verify quantities against the purchase order.",
         "Medium", QDate::currentDate().addDays(1).toString("yyyy-MM-dd"), "Pending"}
    };
    staffTaskDetails["Alex Mercer"] = {
        {"Quarterly Stock Count", "Reconcile physical count with system records.",
         "High", QDate::currentDate().addDays(3).toString("yyyy-MM-dd"), "Pending"},
        {"Label Printer Maintenance", "Replace ribbon and clean print heads.",
         "Low", QDate::currentDate().addDays(-3).toString("yyyy-MM-dd"), "Completed"}
    };

    // ---- sample attendance so the panel demonstrates all three states:
    // currently online, logged out after a full day, and never signed in
    // today. Replace these three lines with real recordLogin()/
    // recordCheckIn()/recordLogout() calls once wired to actual sessions. ----
    staffAttendance["John Doe"]    = {true,  "9:02 AM", "9:05 AM", QString(), "3h 40m so far"};
    staffAttendance["Jane Smith"]  = {false, "8:47 AM", "8:50 AM", "5:30 PM", "8h 40m"};
    staffAttendance["Alex Mercer"] = {false, QString(), QString(), QString(), QString()};

    for (const QString &staff : {QStringLiteral("John Doe"), QStringLiteral("Jane Smith"),
                                  QStringLiteral("Alex Mercer")}) {
        refreshAttendanceCard(staff);
        refreshStatusInTable(staff);
    }
}

TaskManagement::~TaskManagement()
{
    delete ui;
}

// ---------------------------------------------------------------------------
// Box 1: Search + summary metrics
// The header bar, panel titles, search box, and metric-card labels are all
// defined as static widgets in taskmanagement.ui (reached via ui->), styled
// centrally through that file's styleSheet property. This just wires up the
// one thing that needs a signal connection.
// ---------------------------------------------------------------------------
void TaskManagement::setupSummaryPanel()
{
    connect(ui->searchBox, &QLineEdit::textChanged, this, &TaskManagement::onSearchFilterChanged);
}

// ---------------------------------------------------------------------------
// Box 2: Staff attendance overview (login / check-in / logout)
// The panel frame/title/online-count label live in the .ui file; the actual
// per-staff cards are built here and dropped into ui->attendanceCardsLayout,
// since the staff roster isn't fixed at design time.
// ---------------------------------------------------------------------------
void TaskManagement::setupAttendancePanel()
{
    ui->attendanceCardsLayout->addWidget(buildAttendanceCard("John Doe"));
    ui->attendanceCardsLayout->addWidget(buildAttendanceCard("Jane Smith"));
    ui->attendanceCardsLayout->addWidget(buildAttendanceCard("Alex Mercer"));
}

// Builds one staff member's attendance card: a status dot + name, the three
// timestamps (Login / Check-in / Logout), hours worked, and three quick
// -action buttons that call recordLogin()/recordCheckIn()/recordLogout() so
// the card, the summary count, and the main table all stay in sync.
QWidget *TaskManagement::buildAttendanceCard(const QString &staffName)
{
    auto *card = new QFrame();
    card->setStyleSheet(
        "QFrame { background-color:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; }");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    // -- name + status dot row --
    auto *nameRow = new QHBoxLayout();
    nameRow->setSpacing(6);

    auto *dot = new QLabel("●");
    dot->setStyleSheet("color:#A0AEC0; font-size:12px; border:none;");
    attendanceDot[staffName] = dot;

    auto *nameLabel = new QLabel(staffName);
    nameLabel->setStyleSheet("color:#1A202C; font-size:13px; font-weight:bold; border:none;");

    auto *statusText = new QLabel("Offline");
    statusText->setStyleSheet("color:#718096; font-size:11px; border:none;");
    attendanceStatusText[staffName] = statusText;

    nameRow->addWidget(dot);
    nameRow->addWidget(nameLabel, 1);
    nameRow->addWidget(statusText);
    layout->addLayout(nameRow);

    auto *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color:#E2E8F0;");
    layout->addWidget(divider);

    // -- timestamps grid --
    auto *grid = new QGridLayout();
    grid->setSpacing(4);
    grid->setColumnStretch(1, 1);

    auto addRow = [&](int r, const QString &caption, QLabel *&outValue) {
        auto *cap = new QLabel(caption);
        cap->setStyleSheet("color:#718096; font-size:11px; border:none;");
        outValue = new QLabel("—");
        outValue->setStyleSheet("color:#1A202C; font-size:11px; font-weight:600; border:none;");
        outValue->setAlignment(Qt::AlignRight);
        grid->addWidget(cap, r, 0);
        grid->addWidget(outValue, r, 1);
    };

    QLabel *loginVal = nullptr, *checkInVal = nullptr, *logoutVal = nullptr, *hoursVal = nullptr;
    addRow(0, "Login",     loginVal);
    addRow(1, "Check-in",  checkInVal);
    addRow(2, "Logout",    logoutVal);
    addRow(3, "Hours",     hoursVal);

    attendanceLoginLabel[staffName]   = loginVal;
    attendanceCheckInLabel[staffName] = checkInVal;
    attendanceLogoutLabel[staffName]  = logoutVal;
    attendanceHoursLabel[staffName]   = hoursVal;

    layout->addLayout(grid);

    // -- quick-action buttons --
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(6);

    auto *btnLogin = new QPushButton("Log In");
    btnLogin->setStyleSheet(kAttendanceLoginButtonStyle);
    btnLogin->setCursor(Qt::PointingHandCursor);
    btnLogin->setProperty("staffName", staffName);
    connect(btnLogin, &QPushButton::clicked, this, &TaskManagement::onAttendanceLoginClicked);
    attendanceLoginButton[staffName] = btnLogin;

    auto *btnCheckIn = new QPushButton("Check In");
    btnCheckIn->setStyleSheet(kAttendanceCheckInButtonStyle);
    btnCheckIn->setCursor(Qt::PointingHandCursor);
    btnCheckIn->setProperty("staffName", staffName);
    connect(btnCheckIn, &QPushButton::clicked, this, &TaskManagement::onAttendanceCheckInClicked);
    attendanceCheckInButton[staffName] = btnCheckIn;

    auto *btnLogout = new QPushButton("Log Out");
    btnLogout->setStyleSheet(kAttendanceLogoutButtonStyle);
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setProperty("staffName", staffName);
    connect(btnLogout, &QPushButton::clicked, this, &TaskManagement::onAttendanceLogoutClicked);
    attendanceLogoutButton[staffName] = btnLogout;

    buttonRow->addWidget(btnLogin);
    buttonRow->addWidget(btnCheckIn);
    buttonRow->addWidget(btnLogout);
    layout->addLayout(buttonRow);

    return card;
}

// Pushes the current staffAttendance[staffName] values into that staff
// member's card widgets, and recomputes the "N online" badge. Called after
// every login/check-in/logout event instead of rebuilding the panel.
void TaskManagement::refreshAttendanceCard(const QString &staffName)
{
    if (!staffAttendance.contains(staffName))
        return;

    const StaffAttendance &state = staffAttendance.value(staffName);

    if (QLabel *dot = attendanceDot.value(staffName))
        dot->setStyleSheet(QString("color:%1; font-size:12px; border:none;")
                                .arg(state.online ? "#38A169" : "#A0AEC0"));

    if (QLabel *statusText = attendanceStatusText.value(staffName))
        statusText->setText(state.online ? "Online" : "Offline");

    if (QLabel *loginVal = attendanceLoginLabel.value(staffName))
        loginVal->setText(dashIfEmpty(state.loginTime));
    if (QLabel *checkInVal = attendanceCheckInLabel.value(staffName))
        checkInVal->setText(dashIfEmpty(state.checkInTime));
    if (QLabel *logoutVal = attendanceLogoutLabel.value(staffName))
        logoutVal->setText(dashIfEmpty(state.logoutTime));
    if (QLabel *hoursVal = attendanceHoursLabel.value(staffName))
        hoursVal->setText(dashIfEmpty(state.hoursToday));

    // Log In only makes sense while offline; Check In / Log Out only make
    // sense once a session is open.
    if (QPushButton *btn = attendanceLoginButton.value(staffName))
        btn->setEnabled(!state.online);
    if (QPushButton *btn = attendanceCheckInButton.value(staffName))
        btn->setEnabled(state.online);
    if (QPushButton *btn = attendanceLogoutButton.value(staffName))
        btn->setEnabled(state.online);

    // Recompute the "N online" badge across everyone we know about.
    int onlineCount = 0;
    for (const StaffAttendance &s : staffAttendance)
        if (s.online) ++onlineCount;

    if (ui->onlineCountLabel)
        ui->onlineCountLabel->setText(QString("%1 online").arg(onlineCount));
}

// ---------------------------------------------------------------------------
// Box 3: Assigned tasks table
// The frame, title, table widget, and its 8 column headers (ID, Staff Name,
// Total/Pending/Completed Task, Status, Login Time, Action) are all defined
// in taskmanagement.ui. Resize-mode/column-width tuning is easiest to keep
// in code (Designer's column editor doesn't expose per-column resize mode),
// so that's all that happens here.
// ---------------------------------------------------------------------------
void TaskManagement::setupTaskTablePanel()
{
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Status, Login Time, and Action size to their content instead of
    // stretching, so the timestamp/badge/buttons are always fully readable.
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    ui->assignedTasksTable->setColumnWidth(5, 100);
    ui->assignedTasksTable->setColumnWidth(6, 100);
    ui->assignedTasksTable->setColumnWidth(7, 210);

    ui->assignedTasksTable->verticalHeader()->setVisible(false);
    ui->assignedTasksTable->verticalHeader()->setDefaultSectionSize(42);
}

// ---------------------------------------------------------------------------
// Row helpers
// ---------------------------------------------------------------------------
void TaskManagement::insertTaskRow(const QString &staff, const QString &total,
                                   const QString &pending, const QString &completed)
{
    int row = ui->assignedTasksTable->rowCount();
    ui->assignedTasksTable->insertRow(row);

    ui->assignedTasksTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 101)));
    ui->assignedTasksTable->setItem(row, 1, new QTableWidgetItem(staff));
    ui->assignedTasksTable->setItem(row, 2, new QTableWidgetItem(total));
    ui->assignedTasksTable->setItem(row, 3, new QTableWidgetItem(pending));
    ui->assignedTasksTable->setItem(row, 4, new QTableWidgetItem(completed));
    // Status (col 5) and Login Time (col 6) are filled in by
    // refreshStatusInTable() once attendance data exists for this staff
    // member; seed them with placeholders so the row isn't blank meanwhile.
    ui->assignedTasksTable->setItem(row, 5, new QTableWidgetItem("Offline"));
    ui->assignedTasksTable->setItem(row, 6, new QTableWidgetItem("—"));

    staffRowIndex[staff] = row;

    attachRowActionButtons(row);
    updateSummaryMetrics();
}

void TaskManagement::attachRowActionButtons(int row)
{
    auto *container = new QWidget();
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    auto *btnAdd = new QPushButton("Add Task");
    btnAdd->setStyleSheet(kAddTaskButtonStyle);
    btnAdd->setMinimumWidth(80);
    btnAdd->setCursor(Qt::PointingHandCursor);

    auto *btnView = new QPushButton("View Task");
    btnView->setStyleSheet(kViewTaskButtonStyle);
    btnView->setMinimumWidth(85);
    btnView->setCursor(Qt::PointingHandCursor);

    connect(btnAdd, &QPushButton::clicked, this, &TaskManagement::onRowAddTaskClicked);
    connect(btnView, &QPushButton::clicked, this, &TaskManagement::onRowViewTaskClicked);

    layout->addWidget(btnAdd);
    layout->addWidget(btnView);

    ui->assignedTasksTable->setCellWidget(row, 7, container);
}

// Finds which row a clicked action button belongs to by matching the cell
// widget pointer directly - far more reliable than mapping click coordinates
// back to a table index.
int TaskManagement::rowForActionButton(QPushButton *button) const
{
    if (!button || !button->parentWidget())
        return -1;

    QWidget *container = button->parentWidget();
    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        if (ui->assignedTasksTable->cellWidget(row, 7) == container)
            return row;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void TaskManagement::onAssignTaskClicked()
{
    if (!taskTitleInput || !assigneeDropdown)
        return;

    if (taskTitleInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Assign Task", "Please enter a task title before assigning.");
        return;
    }

    const QString staff = assigneeDropdown->currentText();
    if (staff == "Select Staff") {
        QMessageBox::warning(this, "Assign Task", "Please select a staff member.");
        return;
    }

    // If this staff member already has a row, bump their counters instead of
    // creating a duplicate entry for the same person.
    TaskDetail detail;
    detail.title       = taskTitleInput->text().trimmed();
    detail.description = descriptionInput ? descriptionInput->toPlainText().trimmed() : QString();
    detail.priority    = priorityDropdown ? priorityDropdown->currentText() : QStringLiteral("Medium");
    detail.dueDate      = dueDateInput ? dueDateInput->date().toString("yyyy-MM-dd") : QString();
    detail.status       = "Pending";

    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        QTableWidgetItem *nameItem = ui->assignedTasksTable->item(row, 1);
        if (nameItem && nameItem->text() == staff) {
            QTableWidgetItem *totalItem   = ui->assignedTasksTable->item(row, 2);
            QTableWidgetItem *pendingItem = ui->assignedTasksTable->item(row, 3);
            if (totalItem && pendingItem) {
                totalItem->setText(QString::number(totalItem->text().toInt() + 1));
                pendingItem->setText(QString::number(pendingItem->text().toInt() + 1));
            }
            staffTaskDetails[staff].append(detail);
            updateSummaryMetrics();
            onClearFieldsClicked();
            return;
        }
    }

    insertTaskRow(staff, "1", "1", "0");
    staffTaskDetails[staff].append(detail);
    onClearFieldsClicked();
}

void TaskManagement::onClearFieldsClicked()
{
    if (taskTitleInput)   taskTitleInput->clear();
    if (descriptionInput) descriptionInput->clear();
    if (assigneeDropdown) assigneeDropdown->setCurrentIndex(0);
    if (priorityDropdown) priorityDropdown->setCurrentIndex(1);
    if (dueDateInput)     dueDateInput->setDate(QDate::currentDate());
    if (categoryDropdown) categoryDropdown->setCurrentIndex(0);
}

void TaskManagement::onSearchFilterChanged()
{
    if (!ui->searchBox || !ui->assignedTasksTable)
        return;

    const QString query = ui->searchBox->text().trimmed().toLower();

    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        bool matchesSearch = query.isEmpty();
        QTableWidgetItem *nameItem = ui->assignedTasksTable->item(row, 1);
        if (!matchesSearch && nameItem && nameItem->text().toLower().contains(query))
            matchesSearch = true;

        ui->assignedTasksTable->setRowHidden(row, !matchesSearch);
    }

    updateSummaryMetrics();
}

// "Add Task" button on a row: opens a dialog titled "Add Task" to assign an
// additional task to that specific staff member.
void TaskManagement::onRowAddTaskClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    int row = rowForActionButton(button);
    if (row < 0)
        return;

    const QString staffName = ui->assignedTasksTable->item(row, 1)->text();

    QDialog dialog(this);
    dialog.setWindowTitle("Add Task");
    dialog.resize(480, 520);
    dialog.setStyleSheet("background-color:#F8FAFC;");

    auto *dialogLayout = new QVBoxLayout(&dialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(16);

    auto *formFrame = new QFrame();
    formFrame->setStyleSheet(kPanelStyle);
    auto *formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(14);

    auto *formTitle = new QLabel("CREATE NEW TASK FOR " + staffName.toUpper());
    formTitle->setStyleSheet(kPanelTitleStyle);
    formTitle->setWordWrap(true);
    formLayout->addWidget(formTitle, 0, Qt::AlignCenter);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->setColumnStretch(1, 1);

    auto addFormRow = [&](int r, const QString &labelText, QWidget *field,
                          Qt::Alignment align = Qt::AlignVCenter) {
        auto *label = new QLabel(labelText);
        label->setStyleSheet(kLabelStyle);
        grid->addWidget(label, r, 0, align);
        grid->addWidget(field, r, 1);
    };

    auto *titleInput = new QLineEdit();
    titleInput->setPlaceholderText("Enter task title...");
    titleInput->setStyleSheet(kInputStyle);
    addFormRow(0, "Task Title", titleInput);

    auto *descInput = new QTextEdit();
    descInput->setPlaceholderText("Provide instructions...");
    descInput->setMaximumHeight(70);
    descInput->setStyleSheet(kInputStyle);
    addFormRow(1, "Description", descInput, Qt::AlignTop);

    auto *priorityInput = new QComboBox();
    priorityInput->addItems({"Low", "Medium", "High"});
    priorityInput->setCurrentIndex(1);
    priorityInput->setStyleSheet(kInputStyle);
    addFormRow(2, "Priority", priorityInput);

    auto *dueDate = new QDateEdit(QDate::currentDate());
    dueDate->setCalendarPopup(true);
    dueDate->setStyleSheet(kInputStyle);
    addFormRow(3, "Due Date", dueDate);

    auto *categoryInput = new QComboBox();
    categoryInput->addItems({"Restocking", "Inspection", "Receiving", "Other"});
    categoryInput->setStyleSheet(kInputStyle);
    addFormRow(4, "Category", categoryInput);

    formLayout->addLayout(grid);

    auto *buttonBox = new QDialogButtonBox(Qt::Horizontal);
    auto *btnAssign = buttonBox->addButton("Assign Task", QDialogButtonBox::AcceptRole);
    auto *btnCancel = buttonBox->addButton("Cancel", QDialogButtonBox::RejectRole);
    btnAssign->setStyleSheet(kPrimaryButtonStyle);
    btnAssign->setMinimumWidth(120);
    btnCancel->setStyleSheet(kSecondaryButtonStyle);
    btnCancel->setMinimumWidth(90);

    formLayout->addWidget(buttonBox);
    dialogLayout->addWidget(formFrame);

    connect(buttonBox, &QDialogButtonBox::accepted, [&dialog, titleInput]() {
        if (titleInput->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "Add Task", "Please enter a task title.");
            return;
        }
        dialog.accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QTableWidgetItem *totalItem   = ui->assignedTasksTable->item(row, 2);
        QTableWidgetItem *pendingItem = ui->assignedTasksTable->item(row, 3);
        if (totalItem && pendingItem) {
            totalItem->setText(QString::number(totalItem->text().toInt() + 1));
            pendingItem->setText(QString::number(pendingItem->text().toInt() + 1));
            updateSummaryMetrics();
        }

        TaskDetail detail;
        detail.title       = titleInput->text().trimmed();
        detail.description = descInput->toPlainText().trimmed();
        detail.priority    = priorityInput->currentText();
        detail.dueDate     = dueDate->date().toString("yyyy-MM-dd");
        detail.status      = "Pending";
        staffTaskDetails[staffName].append(detail);
    }
}

// "View Task" button on a row: opens a themed dialog (matching the rest of
// the page) with the staff member's info, attendance snapshot, and a
// color-coded stat breakdown.
void TaskManagement::onRowViewTaskClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    int row = rowForActionButton(button);
    if (row < 0)
        return;

    const QString id        = ui->assignedTasksTable->item(row, 0)->text();
    const QString staff     = ui->assignedTasksTable->item(row, 1)->text();
    const QString total     = ui->assignedTasksTable->item(row, 2)->text();
    const QString pending   = ui->assignedTasksTable->item(row, 3)->text();
    const QString completed = ui->assignedTasksTable->item(row, 4)->text();

    QDialog dialog(this);
    dialog.setWindowTitle("View Task");
    dialog.resize(460, 640);
    dialog.setStyleSheet("background-color:#F8FAFC;");

    auto *dialogLayout = new QVBoxLayout(&dialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(16);

    auto *card = new QFrame();
    card->setStyleSheet(kPanelStyle);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(16);

    auto *title = new QLabel("TASK SUMMARY - " + staff.toUpper());
    title->setStyleSheet(kPanelTitleStyle);
    title->setWordWrap(true);
    cardLayout->addWidget(title, 0, Qt::AlignCenter);

    // -- staff info rows --
    auto *infoGrid = new QGridLayout();
    infoGrid->setSpacing(10);
    infoGrid->setColumnStretch(1, 1);

    auto addInfoRow = [&](int r, const QString &labelText, const QString &value) {
        auto *label = new QLabel(labelText);
        label->setStyleSheet(kLabelStyle);
        auto *valueLabel = new QLabel(value);
        valueLabel->setStyleSheet("color:#000000; font-size:13px; border:none;");
        infoGrid->addWidget(label, r, 0);
        infoGrid->addWidget(valueLabel, r, 1);
    };

    addInfoRow(0, "Staff ID", id);
    addInfoRow(1, "Employee Name", staff);

    const StaffAttendance attendance = staffAttendance.value(staff);
    addInfoRow(2, "Status", attendance.online ? "Online" : "Offline");
    addInfoRow(3, "Login Time", dashIfEmpty(attendance.loginTime));
    addInfoRow(4, "Check-in Time", dashIfEmpty(attendance.checkInTime));
    addInfoRow(5, "Logout Time", dashIfEmpty(attendance.logoutTime));
    addInfoRow(6, "Hours Today", dashIfEmpty(attendance.hoursToday));

    cardLayout->addLayout(infoGrid);

    auto *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color:#E2E8F0;");
    cardLayout->addWidget(divider);

    // -- color-coded stat chips, matching the summary panel's palette --
    auto makeStatChip = [](const QString &label, const QString &value,
                           const QString &bgColor, const QString &fgColor) {
        auto *chip = new QFrame();
        chip->setStyleSheet(QString("QFrame { background-color:%1; border-radius:8px; }").arg(bgColor));

        auto *chipLayout = new QVBoxLayout(chip);
        chipLayout->setContentsMargins(12, 10, 12, 10);
        chipLayout->setSpacing(4);

        auto *valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(QString("color:%1; font-size:18px; font-weight:bold; border:none;").arg(fgColor));
        valueLabel->setAlignment(Qt::AlignCenter);

        auto *captionLabel = new QLabel(label);
        captionLabel->setStyleSheet(QString("color:%1; font-size:11px; font-weight:600; border:none;").arg(fgColor));
        captionLabel->setAlignment(Qt::AlignCenter);

        chipLayout->addWidget(valueLabel);
        chipLayout->addWidget(captionLabel);
        return chip;
    };

    auto *statsRow = new QHBoxLayout();
    statsRow->setSpacing(12);
    statsRow->addWidget(makeStatChip("TOTAL", total, "#EBF2FF", "#2D4A7A"));
    statsRow->addWidget(makeStatChip("PENDING", pending, "#FFF3E0", "#DD6B20"));
    statsRow->addWidget(makeStatChip("COMPLETED", completed, "#E9F9EF", "#38A169"));
    cardLayout->addLayout(statsRow);

    auto *divider2 = new QFrame();
    divider2->setFrameShape(QFrame::HLine);
    divider2->setStyleSheet("color:#E2E8F0;");
    cardLayout->addWidget(divider2);

    // -- brief task detail cards (title, description, priority, due date) --
    auto *detailHeading = new QLabel("TASK DETAILS");
    detailHeading->setStyleSheet(
        "color:#2D4A7A; font-size:12px; font-weight:bold; letter-spacing:0.5px; border:none;");
    cardLayout->addWidget(detailHeading);

    const QVector<TaskDetail> tasks = staffTaskDetails.value(staff);
    if (tasks.isEmpty()) {
        auto *noData = new QLabel("No detailed task records yet for this staff member.");
        noData->setStyleSheet("color:#718096; font-size:12px; border:none;");
        noData->setWordWrap(true);
        cardLayout->addWidget(noData);
    } else {
        auto *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setMaximumHeight(210);
        scrollArea->setStyleSheet("background:transparent;");

        auto *listContainer = new QWidget();
        auto *listLayout = new QVBoxLayout(listContainer);
        listLayout->setContentsMargins(0, 0, 0, 0);
        listLayout->setSpacing(8);

        // Most recently assigned task first.
        for (int i = tasks.size() - 1; i >= 0; --i)
            listLayout->addWidget(buildTaskDetailCard(tasks.at(i)));
        listLayout->addStretch();

        scrollArea->setWidget(listContainer);
        cardLayout->addWidget(scrollArea);
    }

    dialogLayout->addWidget(card);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto *btnClose = new QPushButton("Close");
    btnClose->setStyleSheet(kSecondaryButtonStyle);
    btnClose->setMinimumWidth(90);
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonRow->addWidget(btnClose);
    dialogLayout->addLayout(buttonRow);

    dialog.exec();
}

// Builds one small card showing a single task's title, description,
// priority badge, and due date - used inside the View Task dialog.
QWidget *TaskManagement::buildTaskDetailCard(const TaskDetail &task) const
{
    auto *card = new QFrame();
    card->setStyleSheet(
        "QFrame { background-color:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; }");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    auto *titleLabel = new QLabel(task.title.isEmpty() ? "Untitled Task" : task.title);
    titleLabel->setStyleSheet("color:#000000; font-size:13px; font-weight:bold; border:none;");
    titleLabel->setWordWrap(true);

    QString priorityBg = "#E9F9EF", priorityFg = "#38A169";
    if (task.priority == "Medium") {
        priorityBg = "#FFF3E0"; priorityFg = "#DD6B20";
    } else if (task.priority == "High") {
        priorityBg = "#FDECEC"; priorityFg = "#E53E3E";
    }

    auto *priorityBadge = new QLabel(task.priority.isEmpty() ? "Medium" : task.priority);
    priorityBadge->setStyleSheet(QString(
                                     "background-color:%1; color:%2; border:none; border-radius:8px;"
                                     " padding:2px 10px; font-size:11px; font-weight:bold;").arg(priorityBg, priorityFg));
    priorityBadge->setAlignment(Qt::AlignCenter);

    topRow->addWidget(titleLabel, 1);
    topRow->addWidget(priorityBadge, 0, Qt::AlignRight | Qt::AlignTop);
    layout->addLayout(topRow);

    if (!task.description.isEmpty()) {
        auto *descLabel = new QLabel(task.description);
        descLabel->setStyleSheet("color:#4A5568; font-size:12px; border:none;");
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);
    }

    auto *dueLabel = new QLabel("Due: " + (task.dueDate.isEmpty() ? "N/A" : task.dueDate));
    dueLabel->setStyleSheet("color:#718096; font-size:11px; border:none;");
    layout->addWidget(dueLabel);

    return card;
}

void TaskManagement::updateSummaryMetrics()
{
    int totalTasks = 0, totalPending = 0, totalCompleted = 0;

    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        if (ui->assignedTasksTable->isRowHidden(row))
            continue;

        QTableWidgetItem *totalItem     = ui->assignedTasksTable->item(row, 2);
        QTableWidgetItem *pendingItem   = ui->assignedTasksTable->item(row, 3);
        QTableWidgetItem *completedItem = ui->assignedTasksTable->item(row, 4);

        if (totalItem)     totalTasks     += totalItem->text().toInt();
        if (pendingItem)   totalPending   += pendingItem->text().toInt();
        if (completedItem) totalCompleted += completedItem->text().toInt();
    }

    int inProgress = totalTasks - totalPending - totalCompleted;
    if (inProgress < 0)
        inProgress = 0;

    if (ui->totalTasksLabel) ui->totalTasksLabel->setText(QString("Total Tasks: %1").arg(totalTasks));
    if (ui->pendingLabel)    ui->pendingLabel->setText(QString("Pending: %1").arg(totalPending));
    if (ui->inProgressLabel) ui->inProgressLabel->setText(QString("In Progress: %1").arg(inProgress));
    if (ui->completedLabel)  ui->completedLabel->setText(QString("Completed: %1").arg(totalCompleted));
}

// ---------------------------------------------------------------------------
// Attendance: public slots (wire these to real login/logout/check-in events)
// ---------------------------------------------------------------------------
QString TaskManagement::currentTimeLabel() const
{
    return QTime::currentTime().toString("h:mm AP");
}

void TaskManagement::recordLogin(const QString &staffName)
{
    StaffAttendance &state = staffAttendance[staffName];
    state.online    = true;
    state.loginTime = currentTimeLabel();
    state.logoutTime.clear();
    state.hoursToday = "In progress";

    refreshAttendanceCard(staffName);
    refreshStatusInTable(staffName);
}

void TaskManagement::recordCheckIn(const QString &staffName)
{
    StaffAttendance &state = staffAttendance[staffName];
    state.checkInTime = currentTimeLabel();

    refreshAttendanceCard(staffName);
    refreshStatusInTable(staffName);
}

void TaskManagement::recordLogout(const QString &staffName)
{
    StaffAttendance &state = staffAttendance[staffName];
    state.online     = false;
    state.logoutTime  = currentTimeLabel();
    state.hoursToday  = QString("Logged out at %1").arg(state.logoutTime);

    refreshAttendanceCard(staffName);
    refreshStatusInTable(staffName);
}

void TaskManagement::onAttendanceLoginClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;
    recordLogin(button->property("staffName").toString());
}

void TaskManagement::onAttendanceCheckInClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    const QString staffName = button->property("staffName").toString();
    if (!staffAttendance.value(staffName).online) {
        QMessageBox::information(this, "Check In",
                                 staffName + " needs to log in before checking in.");
        return;
    }
    recordCheckIn(staffName);
}

void TaskManagement::onAttendanceLogoutClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;
    recordLogout(button->property("staffName").toString());
}

// Writes the Status badge + Login Time text into that staff member's row in
// the main table (columns 5 and 6), keeping the table in sync with the
// attendance card above it.
void TaskManagement::refreshStatusInTable(const QString &staffName)
{
    if (!ui->assignedTasksTable || !staffRowIndex.contains(staffName))
        return;

    const int row = staffRowIndex.value(staffName);
    const StaffAttendance state = staffAttendance.value(staffName);

    if (QTableWidgetItem *statusItem = ui->assignedTasksTable->item(row, 5)) {
        statusItem->setText(state.online ? "Online" : "Offline");
        statusItem->setForeground(QBrush(state.online ? QColor("#38A169") : QColor("#A0AEC0")));
        QFont font = statusItem->font();
        font.setBold(true);
        statusItem->setFont(font);
    }

    if (QTableWidgetItem *loginItem = ui->assignedTasksTable->item(row, 6))
        loginItem->setText(dashIfEmpty(state.loginTime));
}