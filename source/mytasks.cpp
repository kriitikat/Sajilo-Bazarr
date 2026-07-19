#include "../include/mytasks.h"
#include "../ui/ui_mytasks.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QLayoutItem>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════════
//  NOTE ON createTaskCard()
//
//  Everything static - the greeting banner, the Refresh button, the
//  three summary cards, and the scroll area - is defined once in
//  mytasks.ui and is never restyled from here. The one thing that
//  can't live in the .ui is the task card list itself: the number of
//  cards and their content depend entirely on what the DB query
//  returns, so each card is still built with widget calls below. This
//  mirrors how TaskManagement's Add/View Task dialogs are handled.
// ═══════════════════════════════════════════════════════════════════

MyTasks::MyTasks(int staffId, const QString &staffName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MyTasks)
{
    ui->setupUi(this);

    connect(ui->btnRefresh, &QPushButton::clicked, this, &MyTasks::refreshTasks);

    // "Back to Dashboard" just closes this window - StaffDashboard is the
    // one that owns bringing itself back: it hides itself before opening
    // MyTasks, and re-shows itself (refreshed) via a connect(...destroyed...)
    // hook once this window is destroyed. Since MyTasks is created with
    // WA_DeleteOnClose (set in StaffDashboard::on_btnViewTasks_clicked),
    // a simple close() here is all that's needed to trigger that hand-off -
    // this is the exact same pattern ProductStaff already uses.
    connect(ui->btnBackToDashboard, &QPushButton::clicked, this, &QWidget::close);

    loadTasksForStaff(staffId, staffName);
}

MyTasks::~MyTasks()
{
    delete ui;
}

void MyTasks::refreshTasks()
{
    if (m_currentStaffId < 0) {
        qDebug() << "refreshTasks() called before any staff was loaded - ignoring.";
        return;
    }
    loadTasksForStaff(m_currentStaffId, m_currentStaffName);
}

void MyTasks::clearTaskCards()
{
    QLayoutItem *item;
    while ((item = ui->taskCardsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MyTasks::loadTasksForStaff(int staffId, const QString &staffName)
{
    if (staffId < 0) {
        qDebug() << "loadTasksForStaff() got an invalid staffId:" << staffId;
        return;
    }

    m_currentStaffId   = staffId;
    m_currentStaffName = staffName;

    ui->greetingLabel->setText(QString("Active Tasks: %1").arg(staffName));

    // KEY FIX (kept from the original): filter by staff_id (a number),
    // not staff_name (text). This is what makes sure a staff member only
    // ever sees their own tasks, even if two people share a name or a
    // name gets edited.
    QSqlQuery query;
    query.prepare(
        "SELECT task_title, task_description, priority, deadline, status "
        "FROM tasks WHERE staff_id = ? ORDER BY id DESC");
    query.addBindValue(staffId);

    clearTaskCards();

    int totalCount = 0, pendingCount = 0, completedCount = 0;

    if (query.exec()) {
        while (query.next()) {
            const QString title    = query.value(0).toString();
            const QString desc     = query.value(1).toString();
            const QString priority = query.value(2).toString();
            const QString due      = query.value(3).toString();
            QString status         = query.value(4).toString();
            if (status.isEmpty())
                status = "Pending";

            totalCount++;
            if (status == "Pending") ++pendingCount;
            else if (status == "Completed") ++completedCount;

            ui->taskCardsLayout->addWidget(createTaskCard(title, desc, priority, due, status));
        }
    } else {
        qDebug() << "Failed to load tasks for staff_id" << staffId << ":" << query.lastError().text();
    }

    ui->lblAssignedValue->setText(QString::number(totalCount));
    ui->lblPendingValue->setText(QString::number(pendingCount));
    ui->lblCompletedValue->setText(QString::number(completedCount));

    ui->emptyStateLabel->setVisible(totalCount == 0);
}

QWidget *MyTasks::createTaskCard(const QString &title, const QString &description,
                                 const QString &priority, const QString &dueText,
                                 const QString &statusText) const
{
    auto *card = new QFrame();
    card->setStyleSheet(
        "QFrame { background-color:#FFFFFF; border:1px solid #f0dde1; border-radius:10px; }");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(10);

    // Colors intentionally match the same enable/disable/expire palette used
    // in staffperform.ui, so priority severity reads consistently across
    // the whole theme: Low = green ("enable"), Medium = amber ("disable"),
    // High = garnet ("expire").
    QString priorityBg = "#F0FDF4", priorityFg = "#16a34a"; // Low (default)
    if (priority == "Medium") { priorityBg = "#FFFAF0"; priorityFg = "#d97706"; }
    else if (priority == "High") { priorityBg = "#FFF5F5"; priorityFg = "#8B0000"; }

    auto *topRow = new QHBoxLayout();

    auto *priorityBadge = new QLabel(priority.isEmpty() ? "Medium" : priority);
    priorityBadge->setStyleSheet(QString(
                                     "background-color:%1; color:%2; border:none; border-radius:4px;"
                                     " padding:4px 10px; font-size:11px; font-weight:bold;").arg(priorityBg, priorityFg));

    auto *dueLabel = new QLabel("Due: " + (dueText.isEmpty() ? "N/A" : dueText));
    dueLabel->setStyleSheet("color:#9c7480; font-size:12px; font-weight:500; border:none;");

    topRow->addWidget(priorityBadge);
    topRow->addStretch();
    topRow->addWidget(dueLabel);

    auto *titleLabel = new QLabel(title.isEmpty() ? "Untitled Task" : title);
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#4a1626; border:none;");
    titleLabel->setWordWrap(true);

    auto *descLabel = new QLabel(description.isEmpty() ? "No description provided." : description);
    descLabel->setStyleSheet("color:#4a1626; font-size:13px; border:none;");
    descLabel->setWordWrap(true);

    auto *statusLabel = new QLabel("Status: " + statusText);
    statusLabel->setStyleSheet("color:#4a1626; font-size:12px; font-weight:bold; border:none;");

    cardLayout->addLayout(topRow);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(descLabel);
    cardLayout->addWidget(statusLabel);

    return card;
}