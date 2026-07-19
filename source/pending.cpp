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
#include <QByteArray>

#include <cstdio>   // snprintf  (Ch. 4.4, Features of stdio.h)
#include <cstring>  // strcmp    (Ch. 5.1c mentions switch works on int/char, not strings)

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
//
//  NOTE: All static visual styling (table colors, header colors, alternating
//  row colors, fonts, card frame, etc.) lives ENTIRELY in pending.ui and is
//  untouched here. This file only sets things Qt Designer cannot express as
//  a static property — per-column resize *behavior* (a runtime layout
//  policy) and the dynamically-generated Approve/Decline button widgets,
//  which don't exist until a row is populated from the database.
// ─────────────────────────────────────────────────────────────────────────────

pending::pending(QWidget *parent)
    : BackBase<QWidget>(parent)
    , ui(new Ui::pending)
    , rowCount(0)
{
    ui->setupUi(this);
    setWindowTitle("Pending Registration Requests");

    // Back to Dashboard — inherited from BackBase<QWidget>. Closing this
    // window is enough: AdminDashboard is already open underneath it
    // (see admindashboard.cpp's handlePending_Request_clicked()).
    wireBackButton(ui->btnBackToDashboard);

    // ── Column resize modes (behavioral, not visual — kept in code) ─────────
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // NAME
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);          // USERNAME
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // ROLE
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);          // EMAIL
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // PHONE
    ui->pendingTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);            // ACTIONS
    ui->pendingTable->setColumnWidth(6, 160); // fits the compact, staff-style Approve/Decline pills

    // Row height now follows the buttons' real rendered size via
    // resizeRowsToContents() in loadPendingTable(), the same approach
    // staff.cpp uses. The previous fixed-52px-row approach is what let
    // stale, merely-hidden cell widgets from earlier reloads peek out as
    // "ghosted" duplicate buttons whenever a reload's rendered height
    // didn't exactly match the hard-coded value.

    loadPendingTable();
}

pending::~pending()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Fetch rows into a caller-provided array
//
//  Pulls straight from `pending_requests` — the self-registration queue.
//  That table has no status column at all: every row sitting in it is
//  implicitly "pending", so no WHERE clause is needed.
//
//  outRows / outCount are written through, the way a C function "returns"
//  more than one value via pointer parameters (Ch. 9.4, Pointer and Array).
// ─────────────────────────────────────────────────────────────────────────────

void pending::fetchPendingRows(PendingRow *outRows, int *outCount)
{
    int count = 0;

    QSqlDatabase db = QSqlDatabase::database(); // default connection opened in main()
    if (!db.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("No open database connection."));
        *outCount = 0;
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, first_name, last_name, username, role, email, phone "
        "FROM pending_requests ORDER BY id ASC"));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load pending requests: %1").arg(query.lastError().text()));
        *outCount = 0;
        return;
    }

    // Copy each result row into the fixed array, one struct at a time,
    // stopping if we ever hit MAX_PENDING_ROWS (the same bounds-check a C
    // program has to do by hand when filling a fixed-size array).
    while (query.next() && count < MAX_PENDING_ROWS) {
        PendingRow *row = &outRows[count];

        row->id = query.value(0).toInt();

        QByteArray firstNameBytes = query.value(1).toString().trimmed().toUtf8();
        QByteArray lastNameBytes  = query.value(2).toString().trimmed().toUtf8();
        QByteArray usernameBytes  = query.value(3).toString().toUtf8();
        QByteArray roleBytes      = query.value(4).toString().toUtf8();
        QByteArray emailBytes     = query.value(5).toString().toUtf8();
        QByteArray phoneBytes     = query.value(6).toString().toUtf8();

        snprintf(row->firstName, sizeof(row->firstName), "%s", firstNameBytes.constData());
        snprintf(row->lastName,  sizeof(row->lastName),  "%s", lastNameBytes.constData());
        snprintf(row->username,  sizeof(row->username),  "%s", usernameBytes.constData());
        snprintf(row->role,      sizeof(row->role),      "%s", roleBytes.constData());
        snprintf(row->email,     sizeof(row->email),     "%s", emailBytes.constData());
        snprintf(row->phone,     sizeof(row->phone),     "%s", phoneBytes.constData());

        count = count + 1;
    }

    *outCount = count;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Role code -> display label (Ch. 5.1c: C can only switch on int/char, not
//  a string, so this is an if-else chain rather than a switch statement).
// ─────────────────────────────────────────────────────────────────────────────

void pending::roleDisplayLabel(const char *roleCode, char *outLabel, int outLabelSize)
{
    if (strcmp(roleCode, "staff") == 0) {
        snprintf(outLabel, outLabelSize, "%s", "Staff");
    } else if (strcmp(roleCode, "frontdesk") == 0) {
        snprintf(outLabel, outLabelSize, "%s", "Front Desk");
    } else {
        snprintf(outLabel, outLabelSize, "%s", roleCode);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load table — refills pendingRows[] via fetchPendingRows(), then walks
//  the array with a plain indexed for-loop to build each table row
//  (Ch. 8.2, Processing an Array).
// ─────────────────────────────────────────────────────────────────────────────

void pending::loadPendingTable()
{
    fetchPendingRows(pendingRows, &rowCount);

    // QTableWidget::setCellWidget() only *hides* whatever widget was
    // previously in a cell — it does not delete it (see Qt docs). Simply
    // calling setRowCount(0) below removes the rows, but on some Qt
    // versions/styles a still-alive-but-hidden Approve/Decline pair from
    // an earlier reload can briefly repaint underneath the new one,
    // which is exactly the doubled/"ghosted" button look on this page.
    // Explicitly tearing down every ACTIONS cell widget first guarantees
    // each one is actually deleted before its row disappears.
    for (int r = 0; r < ui->pendingTable->rowCount(); ++r)
        ui->pendingTable->removeCellWidget(r, 6);

    ui->pendingTable->setRowCount(0);

    int i;
    for (i = 0; i < rowCount; i = i + 1) {
        PendingRow *row = &pendingRows[i];

        char fullName[128];
        snprintf(fullName, sizeof(fullName), "%s %s", row->firstName, row->lastName);

        char roleLabel[32];
        roleDisplayLabel(row->role, roleLabel, sizeof(roleLabel));

        ui->pendingTable->insertRow(i);

        // Non-editable cell items. Text color intentionally NOT set here —
        // the .ui stylesheet's QTableWidget::item { color: ... } rule
        // already applies to every cell.
        QTableWidgetItem *idItem       = new QTableWidgetItem(QString::number(row->id));
        QTableWidgetItem *nameItem     = new QTableWidgetItem(QString::fromUtf8(fullName));
        QTableWidgetItem *usernameItem = new QTableWidgetItem(QString::fromUtf8(row->username));
        QTableWidgetItem *roleItem     = new QTableWidgetItem(QString::fromUtf8(roleLabel));
        QTableWidgetItem *emailItem    = new QTableWidgetItem(QString::fromUtf8(row->email));
        QTableWidgetItem *phoneItem    = new QTableWidgetItem(QString::fromUtf8(row->phone));

        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        usernameItem->setFlags(usernameItem->flags() & ~Qt::ItemIsEditable);
        roleItem->setFlags(roleItem->flags() & ~Qt::ItemIsEditable);
        emailItem->setFlags(emailItem->flags() & ~Qt::ItemIsEditable);
        phoneItem->setFlags(phoneItem->flags() & ~Qt::ItemIsEditable);

        ui->pendingTable->setItem(i, 0, idItem);
        ui->pendingTable->setItem(i, 1, nameItem);
        ui->pendingTable->setItem(i, 2, usernameItem);
        ui->pendingTable->setItem(i, 3, roleItem);
        ui->pendingTable->setItem(i, 4, emailItem);
        ui->pendingTable->setItem(i, 5, phoneItem);

        ui->pendingTable->setCellWidget(i, 6, buildActionsWidget(row->id));
    }

    // Friendly placeholder message when the queue is empty.
    if (rowCount == 0) {
        ui->pendingTable->insertRow(0);
        QTableWidgetItem *placeholder = new QTableWidgetItem(tr("No pending requests at this time."));
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEditable);
        placeholder->setForeground(QColor(QStringLiteral("#94a3b8"))); // matches subtitle muted color
        placeholder->setTextAlignment(Qt::AlignCenter);
        ui->pendingTable->setItem(0, 0, placeholder);
        ui->pendingTable->setSpan(0, 0, 1, 7); // merge all columns
    }

    // Size every row to its actual rendered content — same approach
    // staff.cpp uses for its ACTIONS column — instead of a hard-coded
    // pixel height that can drift out of sync with the buttons' real size.
    ui->pendingTable->resizeRowsToContents();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build per-row Approve / Decline widget
//
//  These buttons are generated dynamically, one pair per row, only after the
//  database returns results. Each button's row id is stored as a plain
//  "userId" property on the button itself rather than captured by a lambda —
//  onApproveClicked()/onDeclineClicked() below read it back out via
//  sender(), the same idea as passing an extra argument into a function.
// ─────────────────────────────────────────────────────────────────────────────

QWidget *pending::buildActionsWidget(int userId)
{
    QWidget     *container = new QWidget(ui->pendingTable);
    QHBoxLayout *layout    = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    // No colors/padding/sizes set here at all — pending.ui's stylesheet
    // targets these via the "actionRole" dynamic property (the same
    // pattern staff.cpp/staff.ui use for their ACTIONS buttons: e.g.
    // QPushButton[actionRole="approve"]) and Qt applies it automatically
    // even though these widgets are created after the window is already
    // built. setProperty("actionRole", ...) is the only thing tying a
    // button to its look, so both pages' action buttons render as the
    // same compact pill style.

    // Approve.
    QPushButton *approveBtn = new QPushButton(tr("Approve"), container);
    approveBtn->setProperty("actionRole", QStringLiteral("approve"));
    approveBtn->setCursor(Qt::PointingHandCursor);
    approveBtn->setProperty("userId", userId);
    connect(approveBtn, &QPushButton::clicked, this, &pending::onApproveClicked);

    // Decline.
    QPushButton *declineBtn = new QPushButton(tr("Decline"), container);
    declineBtn->setProperty("actionRole", QStringLiteral("decline"));
    declineBtn->setCursor(Qt::PointingHandCursor);
    declineBtn->setProperty("userId", userId);
    connect(declineBtn, &QPushButton::clicked, this, &pending::onDeclineClicked);

    layout->addWidget(approveBtn);
    layout->addWidget(declineBtn);
    layout->addStretch();

    container->setLayout(layout);
    return container;
}

void pending::onApproveClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr)
        return;

    int userId = button->property("userId").toInt();
    approveRequest(userId);
}

void pending::onDeclineClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr)
        return;

    int userId = button->property("userId").toInt();
    declineRequest(userId);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Approve
//
//  Moves the row from `pending_requests` into `information` with
//  status = 'enabled' (the registrant becomes an employee who can log in
//  immediately), then removes it from the pending queue. Both steps run
//  inside a single transaction: if the INSERT or DELETE fails for any
//  reason (e.g. a UNIQUE username collision against an existing account),
//  everything rolls back so the request is never silently lost or
//  duplicated.
// ─────────────────────────────────────────────────────────────────────────────

void pending::approveRequest(int userId)
{
    QSqlDatabase db = QSqlDatabase::database();

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

    QString firstName = fetch.value(0).toString();
    QString lastName  = fetch.value(1).toString();
    QString username  = fetch.value(2).toString();
    QString role      = fetch.value(3).toString();
    QString email     = fetch.value(4).toString();
    QString phone     = fetch.value(5).toString();
    QString password  = fetch.value(6).toString();

    char fullName[128];
    QByteArray firstNameBytes = firstName.toUtf8();
    QByteArray lastNameBytes  = lastName.toUtf8();
    snprintf(fullName, sizeof(fullName), "%s %s", firstNameBytes.constData(), lastNameBytes.constData());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm Approval"),
        tr("Approve the registration request for %1 (username: %2)?\n\n"
           "They will be added to the system as an employee and will be able to log in.")
            .arg(QString::fromUtf8(fullName), username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (!db.transaction()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Could not start database transaction: %1").arg(db.lastError().text()));
        return;
    }

    // NOTE: `information.status` has CHECK(status IN ('enabled','disabled','expired')) —
    // 'approved' is NOT a valid value and would violate that constraint. A
    // newly-approved account should simply be 'enabled' so it can log in.
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral(
        "INSERT INTO information (first_name, last_name, username, role, email, phone, password, status) "
        "VALUES (:first_name, :last_name, :username, :role, :email, :phone, :password, 'enabled')"));
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
                                 .arg(QString::fromUtf8(fullName)));

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

    char fullName[128];
    QByteArray firstNameBytes = fetch.value(0).toString().toUtf8();
    QByteArray lastNameBytes  = fetch.value(1).toString().toUtf8();
    snprintf(fullName, sizeof(fullName), "%s %s", firstNameBytes.constData(), lastNameBytes.constData());

    QString username = fetch.value(2).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm Decline"),
        tr("Decline and permanently remove the registration request for %1 (username: %2)?\n\n"
           "This cannot be undone.")
            .arg(QString::fromUtf8(fullName), username),
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
                                 .arg(QString::fromUtf8(fullName)));

    loadPendingTable(); // refresh
}