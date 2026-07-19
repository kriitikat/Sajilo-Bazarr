#include "../include/staffperform.h"
#include "../ui/ui_staffperform.h"
#include "../include/attendancemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QColor>
#include <QDate>
#include <QLocale>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>

// ─── Column indices ────────────────────────────────────────────────────────
// 0: ID  1: NAME  2: INCOMPLETE TASKS  3: COMPLETED TASKS
// 4: PRESENT DAYS  5: IRREGULAR DAYS  6: ABSENT DAYS  7: ACTIONS
static constexpr int COL_ID         = 0;
static constexpr int COL_NAME       = 1;
static constexpr int COL_INCOMPLETE = 2;
static constexpr int COL_COMPLETED  = 3;
static constexpr int COL_PRESENT    = 4;
static constexpr int COL_IRREGULAR  = 5;
static constexpr int COL_ABSENT     = 6;
static constexpr int COL_ACTIONS    = 7;
static constexpr int COL_COUNT      = 8;

StaffPerform::StaffPerform(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StaffPerform)
{
    ui->setupUi(this);

    // ── Table setup ────────────────────────────────────────────────────
    // Header text, column count, row height, colors etc. all live in
    // staffperform.ui now. Only the per-column resize MODE stays here,
    // since Qt Designer has no property for QHeaderView::ResizeMode on a
    // plain QTableWidget column (see the identical note in category.cpp).
    auto *header = ui->performanceTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionResizeMode(COL_ID,         QHeaderView::Fixed);
    header->setSectionResizeMode(COL_NAME,       QHeaderView::Stretch);
    header->setSectionResizeMode(COL_INCOMPLETE, QHeaderView::Interactive);
    header->setSectionResizeMode(COL_COMPLETED,  QHeaderView::Interactive);
    header->setSectionResizeMode(COL_PRESENT,    QHeaderView::Interactive);
    header->setSectionResizeMode(COL_IRREGULAR,  QHeaderView::Interactive);
    header->setSectionResizeMode(COL_ABSENT,     QHeaderView::Fixed);
    header->setSectionResizeMode(COL_ACTIONS,    QHeaderView::Fixed);

    header->resizeSection(COL_ID,         48);
    header->resizeSection(COL_INCOMPLETE, 140);
    header->resizeSection(COL_COMPLETED,  130);
    header->resizeSection(COL_PRESENT,    110);
    header->resizeSection(COL_IRREGULAR,  120);
    header->resizeSection(COL_ABSENT,     110);
    header->resizeSection(COL_ACTIONS,    170);

    // "IRREGULAR DAYS" needs a tooltip explaining what it actually means,
    // since the header text alone can't carry the late-in/early-out
    // definition - the .ui file can't express a per-header tooltip either,
    // so it's set here.
    if (auto *item = ui->performanceTable->horizontalHeaderItem(COL_IRREGULAR)) {
        item->setToolTip(tr("Checked in but arrived late and/or left early "
                            "(shift: %1–%2)")
                              .arg(QLatin1String(AttendanceManager::kShiftStart),
                                   QLatin1String(AttendanceManager::kShiftEnd)));
    }

    connect(ui->refreshBtn, &QPushButton::clicked, this, &StaffPerform::loadPerformanceTable);
    connect(ui->backToDashboardBtn, &QPushButton::clicked, this, &StaffPerform::backToDashboardRequested);

    // ── Period label: current calendar month, to date ─────────────────
    const QDate today      = QDate::currentDate();
    const QDate monthStart = QDate(today.year(), today.month(), 1);
    ui->periodLabel->setText(
        tr("Attendance period: %1 – %2 (weekly off: Saturday)")
            .arg(QLocale().toString(monthStart, QStringLiteral("d MMM yyyy")),
                 QLocale().toString(today, QStringLiteral("d MMM yyyy"))));

    loadPerformanceTable();
}

StaffPerform::~StaffPerform()
{
    delete ui;
}

void StaffPerform::taskCounts(int staffId, int *incomplete, int *completed) const
{
    *incomplete = 0;
    *completed  = 0;

    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT status, COUNT(*) FROM tasks WHERE staff_id = :staff_id GROUP BY status"));
    query.bindValue(QStringLiteral(":staff_id"), staffId);

    if (!query.exec()) {
        qWarning() << "StaffPerform::taskCounts failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        const QString status = query.value(0).toString();
        const int     count  = query.value(1).toInt();
        if (status == QLatin1String("Completed"))
            *completed += count;
        else
            *incomplete += count; // "Pending" and "In Progress" both count as not-done-yet
    }
}

void StaffPerform::setCountCell(int row, int col, int value, bool flagIfPositive)
{
    auto *item = new QTableWidgetItem(QString::number(value));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignCenter);

    // Luxury/burgundy palette accents: garnet for a flagged (bad) count,
    // a muted slate for a clean (zero) one. Matches the theme carried in
    // staffperform.ui's stylesheet.
    if (flagIfPositive && value > 0)
        item->setForeground(QColor(QStringLiteral("#8B0000"))); // garnet
    else if (value == 0 && flagIfPositive)
        item->setForeground(QColor(QStringLiteral("#64748b"))); // muted slate

    ui->performanceTable->setItem(row, col, item);
}

void StaffPerform::loadPerformanceTable()
{
    QSqlDatabase db = QSqlDatabase::database(); // default connection opened in main()
    if (!db.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("No open database connection."));
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, first_name, last_name, status FROM information "
        "WHERE role = 'staff' OR role = 'frontdesk' "
        "ORDER BY id ASC"));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff: %1").arg(query.lastError().text()));
        return;
    }

    // Current calendar month to date - same window described in
    // periodLabel, set up in the constructor.
    const QDate today      = QDate::currentDate();
    const QDate monthStart = QDate(today.year(), today.month(), 1);

    ui->performanceTable->setRowCount(0);

    int row = 0;
    while (query.next()) {
        const int     staffId = query.value(0).toInt();
        const QString name    = (query.value(1).toString() + QLatin1Char(' ')
                                  + query.value(2).toString()).trimmed();
        const QString status  = query.value(3).toString(); // 'enabled' | 'disabled' | 'expired'

        ui->performanceTable->insertRow(row);

        auto *idItem = new QTableWidgetItem(QString::number(staffId));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        ui->performanceTable->setItem(row, COL_ID, idItem);

        auto *nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->performanceTable->setItem(row, COL_NAME, nameItem);

        int incomplete = 0, completed = 0;
        taskCounts(staffId, &incomplete, &completed);
        setCountCell(row, COL_INCOMPLETE, incomplete, /*flagIfPositive=*/false);
        setCountCell(row, COL_COMPLETED,  completed,  /*flagIfPositive=*/false);

        const AttendanceManager::AttendanceStats stats =
            AttendanceManager::computeStats(staffId, monthStart, today);
        setCountCell(row, COL_PRESENT,   stats.presentDays,   /*flagIfPositive=*/false);
        setCountCell(row, COL_IRREGULAR, stats.irregularDays, /*flagIfPositive=*/true);
        setCountCell(row, COL_ABSENT,    stats.absentDays,    /*flagIfPositive=*/true);

        ui->performanceTable->setCellWidget(row, COL_ACTIONS, buildActionsWidget(staffId, status));

        ++row;
    }

    ui->performanceTable->resizeRowsToContents();
}

// ─── Actions cell ────────────────────────────────────────────────────────────
// Deliberately just Disable/Enable + Expire/Unexpire - no Edit button here,
// since editing name/email/username/role/password belongs on the Staff
// page (staff.cpp), which already owns that dialog. This page only needs
// to let an admin act on what the performance numbers show them.

QWidget *StaffPerform::buildActionsWidget(int staffId, const QString &status)
{
    const bool disabled = (status == QLatin1String("disabled"));
    const bool expired  = (status == QLatin1String("expired"));

    auto *container = new QWidget(ui->performanceTable);
    auto *layout    = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    // Buttons only set WHAT they are via the "actionRole" dynamic
    // property; staffperform.ui's stylesheet matches on that property
    // (e.g. QPushButton[actionRole="disable"]) for all color/hover/font
    // styling - same convention as staff.ui's buildActionsWidget.

    auto *disableBtn = new QPushButton(disabled ? tr("Enable") : tr("Disable"), container);
    disableBtn->setProperty("actionRole", disabled ? QStringLiteral("enable")
                                                    : QStringLiteral("disable"));
    disableBtn->setCursor(Qt::PointingHandCursor);
    connect(disableBtn, &QPushButton::clicked, this, [this, staffId]() { disableStaff(staffId); });

    auto *expireBtn = new QPushButton(expired ? tr("Unexpire") : tr("Expire"), container);
    expireBtn->setProperty("actionRole", expired ? QStringLiteral("unexpire")
                                                  : QStringLiteral("expire"));
    expireBtn->setCursor(Qt::PointingHandCursor);
    connect(expireBtn, &QPushButton::clicked, this, [this, staffId]() { expireStaff(staffId); });

    layout->addWidget(disableBtn);
    layout->addWidget(expireBtn);
    layout->addStretch();

    container->setLayout(layout);
    return container;
}

// ─── Disable / Enable Staff ────────────────────────────────────────────────
// Identical semantics to staff::disableStaff (staff.cpp) - both toggle the
// same `information.status` column, so the two pages can never disagree.

void StaffPerform::disableStaff(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral("SELECT status FROM information WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), staffId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff status: %1").arg(fetch.lastError().text()));
        return;
    }

    const bool    currentlyDisabled = (fetch.value(0).toString() == QLatin1String("disabled"));
    const QString newStatus = currentlyDisabled
                                  ? QStringLiteral("enabled")
                                  : QStringLiteral("disabled");

    const auto reply = QMessageBox::question(
        this,
        currentlyDisabled ? tr("Confirm Enable") : tr("Confirm Disable"),
        currentlyDisabled
            ? tr("Re-enable this staff member's account?")
            : tr("Disable this staff member's account? They will not be treated as active staff."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE information SET status = :status WHERE id = :id"));
    update.bindValue(QStringLiteral(":status"), newStatus);
    update.bindValue(QStringLiteral(":id"),     staffId);

    if (!update.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to update status: %1").arg(update.lastError().text()));
        return;
    }

    loadPerformanceTable();
}

// ─── Expire / Unexpire Staff ────────────────────────────────────────────────
// Identical semantics to staff::expireStaff (staff.cpp).

void StaffPerform::expireStaff(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral("SELECT status FROM information WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), staffId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff status: %1").arg(fetch.lastError().text()));
        return;
    }

    const bool currentlyExpired = (fetch.value(0).toString() == QLatin1String("expired"));
    const QString newStatus = currentlyExpired
                                  ? QStringLiteral("enabled")
                                  : QStringLiteral("expired");

    const auto reply = QMessageBox::question(
        this,
        currentlyExpired ? tr("Confirm Unexpire") : tr("Confirm Expire"),
        currentlyExpired
            ? tr("Restore this staff member's account to active (enabled)?")
            : tr("Mark this staff member's account as expired? This is typically used when their "
                 "contract or engagement period has ended."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE information SET status = :status WHERE id = :id"));
    update.bindValue(QStringLiteral(":status"), newStatus);
    update.bindValue(QStringLiteral(":id"),     staffId);

    if (!update.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to update status: %1").arg(update.lastError().text()));
        return;
    }

    loadPerformanceTable();
}