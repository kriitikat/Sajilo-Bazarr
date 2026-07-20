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
#include <QMessageBox>
#include <QStyle>
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
//
//  Every widget built here is tagged with a "role" dynamic property;
//  mytasks.ui's master stylesheet supplies the actual colors by
//  matching on that role. This file never contains a hex color or a
//  QSS string.
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
    //
    // `id` is now selected too, so each card knows which DB row to update
    // when "Mark Completed" is clicked.
    QSqlQuery query;
    query.prepare(
        "SELECT id, task_title, task_description, priority, deadline, status "
        "FROM tasks WHERE staff_id = ? ORDER BY id DESC");
    query.addBindValue(staffId);

    clearTaskCards();

    int totalCount = 0, pendingCount = 0, completedCount = 0;

    if (query.exec()) {
        while (query.next()) {
            const int     taskId   = query.value(0).toInt();
            const QString title    = query.value(1).toString();
            const QString desc     = query.value(2).toString();
            const QString priority = query.value(3).toString();
            const QString due      = query.value(4).toString();
            QString status         = query.value(5).toString();
            if (status.isEmpty())
                status = "Pending";

            totalCount++;
            if (status == "Pending") ++pendingCount;
            else if (status == "Completed") ++completedCount;

            ui->taskCardsLayout->addWidget(createTaskCard(taskId, title, desc, priority, due, status));
        }
    } else {
        qDebug() << "Failed to load tasks for staff_id" << staffId << ":" << query.lastError().text();
    }

    ui->lblAssignedValue->setText(QString::number(totalCount));
    ui->lblPendingValue->setText(QString::number(pendingCount));
    ui->lblCompletedValue->setText(QString::number(completedCount));

    ui->emptyStateLabel->setVisible(totalCount == 0);
}

bool MyTasks::updateTaskStatus(int taskId, const QString &newStatus)
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
    return true;
}

QWidget *MyTasks::createTaskCard(int taskId, const QString &title, const QString &description,
                                 const QString &priority, const QString &dueText,
                                 const QString &statusText)
{
    auto *card = new QFrame();
    card->setProperty("role", "taskCard");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(10);

    // Priority determines which of the three fixed badge roles is used;
    // the actual colors live entirely in mytasks.ui.
    QString priorityRole = "priorityLow";
    if (priority == "Medium")
        priorityRole = "priorityMedium";
    else if (priority == "High")
        priorityRole = "priorityHigh";

    auto *topRow = new QHBoxLayout();

    auto *priorityBadge = new QLabel(priority.isEmpty() ? "Medium" : priority);
    priorityBadge->setProperty("role", priorityRole);

    auto *dueLabel = new QLabel("Due: " + (dueText.isEmpty() ? "N/A" : dueText));
    dueLabel->setProperty("role", "cardDue");

    topRow->addWidget(priorityBadge);
    topRow->addStretch();
    topRow->addWidget(dueLabel);

    auto *titleLabel = new QLabel(title.isEmpty() ? "Untitled Task" : title);
    titleLabel->setProperty("role", "cardTitle");
    titleLabel->setWordWrap(true);

    auto *descLabel = new QLabel(description.isEmpty() ? "No description provided." : description);
    descLabel->setProperty("role", "cardDescription");
    descLabel->setWordWrap(true);

    auto *statusLabel = new QLabel("Status: " + statusText);
    statusLabel->setProperty("role", "cardStatus");

    // "Mark Completed" lets staff close out a task themselves. It writes
    // straight to the same `tasks` row admin's TaskManagement page reads,
    // so the change shows up there (e.g. in "View Task") the next time
    // that page loads this staff member's tasks - no direct link between
    // the two windows is needed.
    auto *actionButton = new QPushButton();
    const bool alreadyCompleted = (statusText == "Completed");
    actionButton->setText(alreadyCompleted ? "\u2713 Completed" : "Mark Completed");
    actionButton->setProperty("role", alreadyCompleted ? "completedTag" : "markCompleted");
    actionButton->setEnabled(!alreadyCompleted);
    actionButton->setCursor(alreadyCompleted ? Qt::ArrowCursor : Qt::PointingHandCursor);
    actionButton->setAutoDefault(false);
    actionButton->setDefault(false);

    if (!alreadyCompleted) {
        connect(actionButton, &QPushButton::clicked, this,
                [this, actionButton, statusLabel, taskId]() {
                    if (!updateTaskStatus(taskId, "Completed"))
                        return;

                    statusLabel->setText("Status: Completed");

                    actionButton->setText("\u2713 Completed");
                    actionButton->setEnabled(false);
                    actionButton->setCursor(Qt::ArrowCursor);
                    actionButton->setProperty("role", "completedTag");
                    // The role property changed after the button was already
                    // shown, so force the stylesheet to re-evaluate it.
                    actionButton->style()->unpolish(actionButton);
                    actionButton->style()->polish(actionButton);

                    // Move one task from Pending to Completed in the summary
                    // cards without a full reload.
                    const int newPending   = ui->lblPendingValue->text().toInt() - 1;
                    const int newCompleted = ui->lblCompletedValue->text().toInt() + 1;
                    ui->lblPendingValue->setText(QString::number(qMax(0, newPending)));
                    ui->lblCompletedValue->setText(QString::number(newCompleted));

                    emit taskStatusChanged();
                });
    }

    auto *actionRow = new QHBoxLayout();
    actionRow->addStretch();
    actionRow->addWidget(actionButton);

    cardLayout->addLayout(topRow);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(descLabel);
    cardLayout->addWidget(statusLabel);
    cardLayout->addLayout(actionRow);

    return card;
}
