#include "../include/pending.h"
#include "../ui/ui_pending.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QColor>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
//
//  NOTE: All static visual styling (table colors, header colors, alternating
//  row colors, fonts, card frame, etc.) now lives ENTIRELY in pending.ui.
//  This file only sets things Qt Designer cannot express as a static
//  property — per-column resize *behavior* (a runtime layout policy) and the
//  dynamically-generated Approve/Decline button widgets, which don't exist
//  until a row is populated from the database.
// ─────────────────────────────────────────────────────────────────────────────

pending::pending(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::pending)
{
    ui->setupUi(this);
    setWindowTitle("Pending Registration Requests");

    // ── Column resize modes (behavioral, not visual — kept in code) ─────────
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // NAME
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);          // USERNAME
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // ROLE
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);          // EMAIL
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // PHONE
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);            // ACTIONS
    ui->pendingTable->setColumnWidth(6, 200);

    loadPendingTable();
}

pending::~pending()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load table
//
//  Pulls straight from `pending_requests` — the self-registration queue.
//  That table has no status column at all (see main.cpp's schema comment):
//  every row sitting in it is implicitly "pending", so no WHERE clause is
//  needed.
// ─────────────────────────────────────────────────────────────────────────────

void pending::loadPendingTable()
{
    QSqlDatabase db = QSqlDatabase::database(); // default connection opened in main()
    if (!db.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("No open database connection."));
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, first_name, last_name, username, role, email, phone "
        "FROM pending_requests ORDER BY id ASC"));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load pending requests: %1").arg(query.lastError().text()));
        return;
    }

    ui->pendingTable->setRowCount(0);

    int row = 0;
    while (query.next()) {
        const int     id       = query.value(0).toInt();
        const QString fullName = query.value(1).toString().trimmed()
                                 + QLatin1Char(' ')
                                 + query.value(2).toString().trimmed();
        const QString username = query.value(3).toString();
        const QString role     = query.value(4).toString();
        const QString email    = query.value(5).toString();
        const QString phone    = query.value(6).toString();

        ui->pendingTable->insertRow(row);

        // Helper: create a non-editable cell item. Text color intentionally
        // NOT set here — the .ui stylesheet's QTableWidget::item { color: ... }
        // rule already applies to every cell, so setting it again per-item
        // would just be a second, redundant place styling could drift out
        // of sync.
        auto makeCell = [](const QString &text) -> QTableWidgetItem * {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        ui->pendingTable->setItem(row, 0, makeCell(QString::number(id)));
        ui->pendingTable->setItem(row, 1, makeCell(fullName));
        ui->pendingTable->setItem(row, 2, makeCell(username));

        // Capitalise the role string for display (e.g. "frontdesk" -> "Front Desk").
        QString roleDisplay = role;
        if (role == QLatin1String("staff"))
            roleDisplay = tr("Staff");
        else if (role == QLatin1String("frontdesk"))
            roleDisplay = tr("Front Desk");
        ui->pendingTable->setItem(row, 3, makeCell(roleDisplay));

        ui->pendingTable->setItem(row, 4, makeCell(email));
        ui->pendingTable->setItem(row, 5, makeCell(phone));

        ui->pendingTable->setCellWidget(row, 6, buildActionsWidget(id));

        ++row;
    }

    ui->pendingTable->resizeRowsToContents();

    // Friendly placeholder message when the queue is empty.
    if (row == 0) {
        ui->pendingTable->insertRow(0);
        auto *placeholder = new QTableWidgetItem(tr("No pending requests at this time."));
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEditable);
        placeholder->setForeground(QColor(QStringLiteral("#94a3b8"))); // matches subtitle muted color
        placeholder->setTextAlignment(Qt::AlignCenter);
        ui->pendingTable->setItem(0, 0, placeholder);
        ui->pendingTable->setSpan(0, 0, 1, 7); // merge all columns
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build per-row Approve / Decline widget
//
//  These buttons are generated dynamically, one pair per row, only after the
//  database returns results — Qt Designer has no way to predefine a widget
//  that doesn't exist until runtime, so their styling has to stay here.
//  Colors are semantic action colors (green = approve, red = decline) and
//  deliberately kept separate from the neutral table theme.
// ─────────────────────────────────────────────────────────────────────────────

QWidget *pending::buildActionsWidget(int userId)
{
    auto *container = new QWidget(ui->pendingTable);
    auto *layout    = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    const QString baseStyle = QStringLiteral(
        "QPushButton {"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  color: white;"
        "}");

    // Approve — green (semantic action colour).
    auto *approveBtn = new QPushButton(tr("Approve"), container);
    approveBtn->setCursor(Qt::PointingHandCursor);
    approveBtn->setStyleSheet(baseStyle + QStringLiteral(
                                  "QPushButton { background-color: #16A34A; }"
                                  "QPushButton:hover { background-color: #15803D; }"));
    connect(approveBtn, &QPushButton::clicked,
            this, [this, userId]() { approveRequest(userId); });

    // Decline — red (semantic action colour).
    auto *declineBtn = new QPushButton(tr("Decline"), container);
    declineBtn->setCursor(Qt::PointingHandCursor);
    declineBtn->setStyleSheet(baseStyle + QStringLiteral(
                                  "QPushButton { background-color: #DC2626; }"
                                  "QPushButton:hover { background-color: #991B1B; }"));
    connect(declineBtn, &QPushButton::clicked,
            this, [this, userId]() { declineRequest(userId); });

    layout->addWidget(approveBtn);
    layout->addWidget(declineBtn);
    layout->addStretch();

    container->setLayout(layout);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Approve
//
//  Moves the row from `pending_requests` into `information` with
//  status = 'approved' (the registrant becomes an employee), then removes it
//  from the pending queue. Both steps run inside a single transaction: if the
//  INSERT or DELETE fails for any reason (e.g. a UNIQUE username collision
//  against an existing account), everything rolls back so the request is
//  never silently lost or duplicated.
// ─────────────────────────────────────────────────────────────────────────────

void pending::approveRequest(int userId)
{
    QSqlDatabase db = QSqlDatabase::database();

    // Fetch the full pending record — building the new `information` row
    // needs every column, not just the name shown in the confirmation dialog.
    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral(
        "SELECT first_name, last_name, username, role, email, phone, password "
        "FROM pending_requests WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), userId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Could not load pending request: %1").arg(fetch.lastError().text()));
        return;
    }

    const QString firstName = fetch.value(0).toString();
    const QString lastName  = fetch.value(1).toString();
    const QString username  = fetch.value(2).toString();
    const QString role      = fetch.value(3).toString();
    const QString email     = fetch.value(4).toString();
    const QString phone     = fetch.value(5).toString();
    const QString password  = fetch.value(6).toString();

    const QString name = (firstName + QLatin1Char(' ') + lastName).trimmed();

    const auto reply = QMessageBox::question(
        this, tr("Confirm Approval"),
        tr("Approve the registration request for %1 (username: %2)?\n\n"
           "They will be added to the system as an employee and will be able to log in.")
            .arg(name, username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (!db.transaction()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Could not start database transaction: %1").arg(db.lastError().text()));
        return;
    }

    QSqlQuery insert(db);
    insert.prepare(QStringLiteral(
        "INSERT INTO information (first_name, last_name, username, role, email, phone, password, status) "
        "VALUES (:first_name, :last_name, :username, :role, :email, :phone, :password, 'approved')"));
    insert.bindValue(QStringLiteral(":first_name"), firstName);
    insert.bindValue(QStringLiteral(":last_name"), lastName);
    insert.bindValue(QStringLiteral(":username"), username);
    insert.bindValue(QStringLiteral(":role"), role);
    insert.bindValue(QStringLiteral(":email"), email);
    insert.bindValue(QStringLiteral(":phone"), phone);
    insert.bindValue(QStringLiteral(":password"), password);

    if (!insert.exec()) {
        db.rollback();
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to create the approved account (the username may already be "
                                 "taken): %1").arg(insert.lastError().text()));
        return;
    }

    QSqlQuery del(db);
    del.prepare(QStringLiteral("DELETE FROM pending_requests WHERE id = :id"));
    del.bindValue(QStringLiteral(":id"), userId);

    if (!del.exec()) {
        db.rollback();
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to remove the request from the pending queue: %1")
                                  .arg(del.lastError().text()));
        return;
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to finalize approval: %1").arg(db.lastError().text()));
        return;
    }

    QMessageBox::information(this, tr("Approved"),
                             tr("%1's account has been approved. They can now log in.")
                                 .arg(name));

    loadPendingTable(); // refresh
}

// ─────────────────────────────────────────────────────────────────────────────
//  Decline — permanently delete the row from `pending_requests`.
// ─────────────────────────────────────────────────────────────────────────────

void pending::declineRequest(int userId)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral(
        "SELECT first_name, last_name, username FROM pending_requests WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), userId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Could not load pending request: %1").arg(fetch.lastError().text()));
        return;
    }

    const QString name     = (fetch.value(0).toString() + QLatin1Char(' ') + fetch.value(1).toString()).trimmed();
    const QString username = fetch.value(2).toString();

    const auto reply = QMessageBox::question(
        this, tr("Confirm Decline"),
        tr("Decline and permanently remove the registration request for %1 (username: %2)?\n\n"
           "This cannot be undone.")
            .arg(name, username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlQuery del(db);
    del.prepare(QStringLiteral("DELETE FROM pending_requests WHERE id = :id"));
    del.bindValue(QStringLiteral(":id"), userId);

    if (!del.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to decline request: %1").arg(del.lastError().text()));
        return;
    }

    QMessageBox::information(this, tr("Declined"),
                             tr("The registration request for %1 has been declined and removed.")
                                 .arg(name));

    loadPendingTable(); // refresh
}