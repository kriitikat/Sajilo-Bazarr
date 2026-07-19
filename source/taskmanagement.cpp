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
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFrame>
#include <QScrollArea>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSizePolicy>
#include <QStringList>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════════
//  NOTE ON THE ADD-TASK / VIEW-TASK DIALOGS
//
//  Everything that is *static* (the summary panel, the table and its
//  headers, the metric labels) is defined once in taskmanagement.ui and
//  is never touched by styleSheet-setting code here anymore.
//
//  The Add Task / View Task dialogs, however, are still assembled with
//  widget calls below. They can't be pre-built in Designer because their
//  content is different every single time they open (a variable-length
//  list of that staff member's tasks, a title built from their name,
//  etc.) — Qt Designer only knows how to lay out a fixed, static form.
//  This is standard practice for data-driven dialogs in Qt; the style
//  constants below exist only because those specific widgets have no
//  .ui counterpart to carry a styleSheet property for them.
// ═══════════════════════════════════════════════════════════════════
namespace {
const QString kPanelStyle =
    "QFrame { background-color:#FFFFFF; border:1px solid #E2E8F0; border-radius:10px; }";

const QString kPanelTitleStyle =
    "font-size:14px; font-weight:bold; color:#2D4A7A; letter-spacing:0.5px; border:none;";

const QString kLabelStyle = "QLabel { font-weight:600; color:#1A202C; border:none; }";

const QString kInputStyle =
    "QLineEdit, QTextEdit, QComboBox, QDateEdit {"
    "  background-color:#FFFFFF; border:1px solid #CBD5E0; color:#000000;"
    "  border-radius:6px; padding:6px; font-size:13px; outline:none; }"
    "QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QDateEdit:focus {"
    "  border:1px solid #2D4A7A; outline:none; }"
    "QComboBox QAbstractItemView {"
    "  color:#000000; background-color:#FFFFFF; outline:none;"
    "  selection-background-color:#EBF2FF; selection-color:#000000; }"
    "QComboBox QAbstractItemView::item:hover {"
    "  background-color:#EBF2FF; color:#000000; }";

const QString kPrimaryButtonStyle =
    "QPushButton { background-color:#2D4A7A; color:#FFFFFF; border:none;"
    "  border-radius:6px; padding:8px 20px; font-weight:bold; font-size:13px; outline:none; }"
    "QPushButton:hover    { background-color:#223A61; }"
    "QPushButton:pressed  { background-color:#1B2E4D; }"
    "QPushButton:focus    { outline:none; border:none; }";

const QString kSecondaryButtonStyle =
    "QPushButton { background-color:#FFFFFF; color:#4A5568; border:1px solid #CBD5E0;"
    "  border-radius:6px; padding:8px 18px; font-size:13px; font-weight:500; outline:none; }"
    "QPushButton:hover   { background-color:#F7FAFC; border-color:#A0AEC0; }"
    "QPushButton:pressed { background-color:#EDF2F7; }"
    "QPushButton:focus   { outline:none; }";

// Colors below are intentionally identical to staffperform.ui's
// QPushButton[actionRole="expire"] / QPushButton[actionRole="disable"]
// rules, so the two pages read consistently to an admin - "Add Task"
// picks up the garnet "expire" color, "View Task" picks up the amber
// "disable" color.
const QString kAddTaskButtonStyle =
    "QPushButton { background-color:#8B0000; color:white; border:none;"
    "  border-radius:6px; padding:4px 10px; font-size:11px; font-weight:bold; outline:none; }"
    "QPushButton:hover   { background-color:#660000; }"
    "QPushButton:pressed { background-color:#4d0000; }"
    "QPushButton:focus   { outline:none; }";

const QString kViewTaskButtonStyle =
    "QPushButton { background-color:#d97706; color:white; border:none;"
    "  border-radius:6px; padding:4px 10px; font-size:11px; font-weight:bold; outline:none; }"
    "QPushButton:hover   { background-color:#b45309; }"
    "QPushButton:pressed { background-color:#92400e; }"
    "QPushButton:focus   { outline:none; }";
}

TaskManagement::TaskManagement(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TaskManagement)
{
    ui->setupUi(this);
    resize(950, 980);

    // Behavioural (not visual) header setup - column stretch/resize rules
    // aren't exposed as static .ui column properties, so they're applied
    // here rather than duplicated as styling.
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->assignedTasksTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->assignedTasksTable->setColumnWidth(5, 210);

    connect(ui->searchBox, &QLineEdit::textChanged, this, &TaskManagement::onSearchFilterChanged);

    // 1) Build one row per staff member (counters start at 0/0/0).
    loadStaffFromDatabase();
    // 2) Read back every previously saved task and fold its counts into
    //    the rows just built, keyed by staff_id so renames can't break it.
    loadTasksFromDatabase();

    updateSummaryMetrics();
}

TaskManagement::~TaskManagement()
{
    delete ui;
}

void TaskManagement::loadStaffFromDatabase()
{
    QSqlQuery query;
    query.prepare(
        "SELECT id, first_name, last_name FROM information "
        "WHERE role = 'staff' OR role = 'frontdesk' "
        "ORDER BY id ASC");

    if (!query.exec()) {
        qDebug() << "Failed to load staff list:" << query.lastError().text();
        QMessageBox::critical(this, "Database Error",
                              "Failed to load staff list: " + query.lastError().text());
        return;
    }

    while (query.next()) {
        const int staffId = query.value(0).toInt();
        const QString fullName =
            (query.value(1).toString() + " " + query.value(2).toString()).trimmed();
        if (fullName.isEmpty())
            continue;

        insertTaskRow(staffId, fullName, "0", "0", "0");
    }
}

void TaskManagement::loadTasksFromDatabase()
{
    QSqlQuery q;
    if (!q.exec(
            "SELECT id, staff_id, task_title, task_description, priority, deadline, status "
            "FROM tasks ORDER BY id ASC")) {
        qDebug() << "Failed to load tasks:" << q.lastError().text();
        QMessageBox::critical(this, "Database Error",
                              "Failed to load tasks: " + q.lastError().text());
        return;
    }

    while (q.next()) {
        TaskDetail detail;
        detail.id           = q.value(0).toInt();
        const int staffId   = q.value(1).toInt();
        detail.title        = q.value(2).toString();
        detail.description  = q.value(3).toString();
        detail.priority     = q.value(4).toString();
        detail.dueDate      = q.value(5).toString();
        detail.status       = q.value(6).toString();

        staffTaskDetails[staffId].append(detail);
    }

    // Now that staffTaskDetails is populated, sync every row's counters.
    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        const int staffId = staffIdForRow(row);
        if (staffId < 0)
            continue;

        const QVector<TaskDetail> &tasks = staffTaskDetails.value(staffId);
        int pending = 0, completed = 0;
        for (const auto &t : tasks) {
            if (t.status == "Pending") ++pending;
            else if (t.status == "Completed") ++completed;
        }

        if (auto *item = ui->assignedTasksTable->item(row, 2)) item->setText(QString::number(tasks.size()));
        if (auto *item = ui->assignedTasksTable->item(row, 3)) item->setText(QString::number(pending));
        if (auto *item = ui->assignedTasksTable->item(row, 4)) item->setText(QString::number(completed));
    }
}

void TaskManagement::insertTaskRow(int staffId, const QString &staff, const QString &total,
                                   const QString &pending, const QString &completed)
{
    int row = ui->assignedTasksTable->rowCount();
    ui->assignedTasksTable->insertRow(row);

    // The ID column now shows the real information.id, and both the ID and
    // Name cells carry staffId as UserRole data so later lookups never have
    // to depend on matching the (renamable) display name.
    auto *itemId = new QTableWidgetItem(QString::number(staffId));
    itemId->setData(Qt::UserRole, staffId);
    itemId->setTextAlignment(Qt::AlignCenter);

    auto *itemName = new QTableWidgetItem(staff);
    itemName->setData(Qt::UserRole, staffId);

    ui->assignedTasksTable->setItem(row, 0, itemId);
    ui->assignedTasksTable->setItem(row, 1, itemName);
    ui->assignedTasksTable->setItem(row, 2, new QTableWidgetItem(total));
    ui->assignedTasksTable->setItem(row, 3, new QTableWidgetItem(pending));
    ui->assignedTasksTable->setItem(row, 4, new QTableWidgetItem(completed));

    attachRowActionButtons(row, staffId);
    updateSummaryMetrics();
}

void TaskManagement::attachRowActionButtons(int row, int staffId)
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
    btnAdd->setAutoDefault(false);
    btnAdd->setDefault(false);
    btnAdd->setProperty("staffId", staffId);

    auto *btnView = new QPushButton("View Task");
    btnView->setStyleSheet(kViewTaskButtonStyle);
    btnView->setMinimumWidth(85);
    btnView->setCursor(Qt::PointingHandCursor);
    btnView->setAutoDefault(false);
    btnView->setDefault(false);
    btnView->setProperty("staffId", staffId);


    connect(btnView, &QPushButton::clicked, this, &TaskManagement::onRowViewTaskClicked);

    layout->addWidget(btnAdd);
    layout->addWidget(btnView);

    ui->assignedTasksTable->setCellWidget(row, 5, container);
}

int TaskManagement::rowForActionButton(QPushButton *button) const
{
    if (!button || !button->parentWidget())
        return -1;

    QWidget *container = button->parentWidget();
    for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
        if (ui->assignedTasksTable->cellWidget(row, 5) == container)
            return row;
    }
    return -1;
}

int TaskManagement::staffIdForRow(int row) const
{
    QTableWidgetItem *item = ui->assignedTasksTable->item(row, 1);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

bool TaskManagement::insertTask(int staffId, const QString &staff, const QString &title,
                                const QString &description, const QString &priority,
                                const QString &category, const QString &dueDate,
                                const QString &status, int *outId)
{
    // Positional ('?') placeholders — named placeholders can miscount when
    // punctuation/colons appear inside bound text.
    QSqlQuery q;
    const bool prepared = q.prepare(
        "INSERT INTO tasks "
        "(staff_id, staff_name, task_title, task_description, priority, category, deadline, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    if (!prepared) {
        qDebug() << "SQL Prepare Error:" << q.lastError().text();
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }

    q.addBindValue(staffId);
    q.addBindValue(staff);
    q.addBindValue(title);
    q.addBindValue(description);
    q.addBindValue(priority);
    q.addBindValue(category);
    q.addBindValue(dueDate);
    q.addBindValue(status);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!q.exec()) {
        qDebug() << "SQL Exec Error:" << q.lastError().text();
        qDebug() << "Bound values count:" << q.boundValues().size();
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }

    if (outId)
        *outId = q.lastInsertId().toInt();

    return true;
}

bool TaskManagement::updateTaskStatus(int taskId, const QString &newStatus)
{
    QSqlQuery q;
    q.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    q.addBindValue(newStatus);
    q.addBindValue(taskId);
    if (!q.exec()) {
        qDebug() << "SQL Error:" << q.lastError().text();
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }

    emit taskDataChanged();
    return true;
}

void TaskManagement::onSearchFilterChanged()
{
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

void TaskManagement::onRowAddTaskClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    int row = rowForActionButton(button);
    if (row < 0)
        return;

    const int staffId = button->property("staffId").toInt();
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
    formTitle->setAlignment(Qt::AlignCenter);
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
    btnAssign->setAutoDefault(false);
    btnAssign->setDefault(false);
    btnCancel->setStyleSheet(kSecondaryButtonStyle);
    btnCancel->setMinimumWidth(90);
    btnCancel->setAutoDefault(false);
    btnCancel->setDefault(false);

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
        int newId = -1;
        const bool ok = insertTask(
            staffId,
            staffName,
            titleInput->text().trimmed(),
            descInput->toPlainText().trimmed(),
            priorityInput->currentText(),
            categoryInput->currentText(),
            dueDate->date().toString("yyyy-MM-dd"),
            "Pending",
            &newId);

        if (!ok)
            return;

        QTableWidgetItem *totalItem   = ui->assignedTasksTable->item(row, 2);
        QTableWidgetItem *pendingItem = ui->assignedTasksTable->item(row, 3);
        if (totalItem && pendingItem) {
            totalItem->setText(QString::number(totalItem->text().toInt() + 1));
            pendingItem->setText(QString::number(pendingItem->text().toInt() + 1));
        }

        TaskDetail detail;
        detail.id          = newId;
        detail.title       = titleInput->text().trimmed();
        detail.description = descInput->toPlainText().trimmed();
        detail.priority    = priorityInput->currentText();
        detail.dueDate     = dueDate->date().toString("yyyy-MM-dd");
        detail.status      = "Pending";
        staffTaskDetails[staffId].append(detail);

        updateSummaryMetrics();
        emit taskDataChanged();
    }
}

void TaskManagement::onRowViewTaskClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    int row = rowForActionButton(button);
    if (row < 0)
        return;

    const int staffId       = button->property("staffId").toInt();
    const QString id        = ui->assignedTasksTable->item(row, 0)->text();
    const QString staff     = ui->assignedTasksTable->item(row, 1)->text();

    QDialog dialog(this);
    dialog.setWindowTitle("View Task");
    dialog.resize(500, 580);
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
    title->setAlignment(Qt::AlignCenter);
    title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    cardLayout->addWidget(title, 0, Qt::AlignCenter);

    auto *infoGrid = new QGridLayout();
    infoGrid->setSpacing(10);
    infoGrid->setColumnStretch(1, 1);

    auto addInfoRow = [&](int r, const QString &labelText, const QString &value) {
        auto *label = new QLabel(labelText);
        label->setStyleSheet(kLabelStyle);
        auto *valueLabel = new QLabel(value);
        valueLabel->setStyleSheet("color:#000000; font-size:13px; border:none;");
        valueLabel->setWordWrap(true);
        infoGrid->addWidget(label, r, 0);
        infoGrid->addWidget(valueLabel, r, 1);
    };

    addInfoRow(0, "Staff ID", id);
    addInfoRow(1, "Employee Name", staff);
    cardLayout->addLayout(infoGrid);

    auto *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color:#E2E8F0;");
    cardLayout->addWidget(divider);

    auto *statsRow = new QHBoxLayout();
    statsRow->setSpacing(12);

    int total = 0, pending = 0, completed = 0;
    const QVector<TaskDetail> &currentTasks = staffTaskDetails.value(staffId);
    for (const auto &t : currentTasks) {
        total++;
        if (t.status == "Pending") pending++;
        else if (t.status == "Completed") completed++;
    }

    auto makeStatChip = [](const QString &labelName, const QString &val, const QString &bg, const QString &fg) {
        auto *chip = new QFrame();
        chip->setStyleSheet(QString("QFrame { background-color:%1; border:none; border-radius:6px; }").arg(bg));
        auto *chipLayout = new QVBoxLayout(chip);
        chipLayout->setContentsMargins(8, 8, 8, 8);
        chipLayout->setSpacing(2);

        auto *titleLbl = new QLabel(labelName);
        titleLbl->setStyleSheet(QString("color:%1; font-size:10px; font-weight:bold; border:none;").arg(fg));
        titleLbl->setAlignment(Qt::AlignCenter);

        auto *valLbl = new QLabel(val);
        valLbl->setStyleSheet(QString("color:%1; font-size:16px; font-weight:bold; border:none;").arg(fg));
        valLbl->setAlignment(Qt::AlignCenter);

        chipLayout->addWidget(titleLbl);
        chipLayout->addWidget(valLbl);
        return chip;
    };

    statsRow->addWidget(makeStatChip("TOTAL", QString::number(total), "#EBF2FF", "#2D4A7A"));
    statsRow->addWidget(makeStatChip("PENDING", QString::number(pending), "#FFF3E0", "#DD6B20"));
    statsRow->addWidget(makeStatChip("COMPLETED", QString::number(completed), "#E9F9EF", "#38A169"));
    cardLayout->addLayout(statsRow);

    auto *divider2 = new QFrame();
    divider2->setFrameShape(QFrame::HLine);
    divider2->setStyleSheet("color:#E2E8F0;");
    cardLayout->addWidget(divider2);

    auto *detailHeading = new QLabel("TASK DETAILS");
    detailHeading->setStyleSheet(
        "color:#2D4A7A; font-size:12px; font-weight:bold; letter-spacing:0.5px; border:none;");
    cardLayout->addWidget(detailHeading);

    if (currentTasks.isEmpty()) {
        auto *noData = new QLabel("No detailed task records yet for this staff member.");
        noData->setStyleSheet("color:#718096; font-size:12px; border:none;");
        noData->setWordWrap(true);
        cardLayout->addWidget(noData);
    } else {
        auto *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setMaximumHeight(230);
        scrollArea->setStyleSheet("background:transparent;");

        auto *listContainer = new QWidget();
        auto *listLayout = new QVBoxLayout(listContainer);
        listLayout->setContentsMargins(0, 0, 0, 0);
        listLayout->setSpacing(8);

        for (int i = currentTasks.size() - 1; i >= 0; --i)
            listLayout->addWidget(buildTaskDetailCard(currentTasks.at(i), staffId));
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
    btnClose->setAutoDefault(false);
    btnClose->setDefault(false);
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonRow->addWidget(btnClose);
    dialogLayout->addLayout(buttonRow);

    dialog.exec();
}

QWidget *TaskManagement::buildTaskDetailCard(const TaskDetail &task, int staffId)
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

    auto *statusRow = new QHBoxLayout();
    statusRow->setSpacing(8);

    auto *statusLabel = new QLabel("Status:");
    statusLabel->setStyleSheet(kLabelStyle);

    auto *statusCombo = new QComboBox();
    statusCombo->addItems({"Pending", "In Progress", "Completed"});
    statusCombo->setStyleSheet(kInputStyle);
    int idx = statusCombo->findText(task.status);
    statusCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    statusCombo->setEnabled(task.id >= 0);

    const int taskId = task.id;
    connect(statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, taskId, staffId](int) {
                auto *combo = qobject_cast<QComboBox *>(sender());
                if (!combo || taskId < 0)
                    return;

                const QString newStatus = combo->currentText();
                if (!updateTaskStatus(taskId, newStatus))
                    return;

                auto &tasks = staffTaskDetails[staffId];
                for (auto &t : tasks) {
                    if (t.id == taskId) {
                        t.status = newStatus;
                        break;
                    }
                }

                // Update counters directly in the master table view, matched
                // by staff_id (UserRole data) rather than the name string.
                for (int row = 0; row < ui->assignedTasksTable->rowCount(); ++row) {
                    if (staffIdForRow(row) == staffId) {
                        int tCount = 0, pCount = 0, cCount = 0;
                        for (const auto &t : tasks) {
                            tCount++;
                            if (t.status == "Pending") pCount++;
                            else if (t.status == "Completed") cCount++;
                        }
                        if (auto *item = ui->assignedTasksTable->item(row, 2)) item->setText(QString::number(tCount));
                        if (auto *item = ui->assignedTasksTable->item(row, 3)) item->setText(QString::number(pCount));
                        if (auto *item = ui->assignedTasksTable->item(row, 4)) item->setText(QString::number(cCount));
                        break;
                    }
                }
                updateSummaryMetrics();
            });

    statusRow->addWidget(statusLabel);
    statusRow->addWidget(statusCombo, 1);
    layout->addLayout(statusRow);

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

    ui->totalTasksLabel->setText(QString("Total Tasks: %1").arg(totalTasks));
    ui->pendingLabel->setText(QString("Pending: %1").arg(totalPending));
    ui->inProgressLabel->setText(QString("In Progress: %1").arg(inProgress));
    ui->completedLabel->setText(QString("Completed: %1").arg(totalCompleted));
}