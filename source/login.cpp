#include "../include/login.h"
#include "../ui/ui_login.h"

// ── Dashboard headers ─────────────────────────────────────────────────────────
// admindashboard.h / staffdashboard.h / frontdesk.h / register.h all live in
// the include/ folder, so they're pulled in with the same "../include/"
// relative path used for login.h above.
#include "../include/admindashboard.h"
#include "../include/staffdashboard.h"
#include "../include/frontdesk.h"
#include "../include/register.h"
#include "../include/attendancemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QAction>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

// ─────────────────────────────────────────────────────────────────────────────
//  Local helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Renders a small "eye" glyph for the password show/hide action, drawn with
// QPainter instead of shipping a .qrc/icon asset. crossed == true draws the
// slashed variant (used while the password is currently visible).
QIcon makeEyeIcon(bool crossed)
{
    const int size = 20;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Matches the muted label/placeholder tone already used in login.ui
    // (QLabel#subtitleLabel), so the glyph blends with the existing look.
    const QColor strokeColor(QStringLiteral("#7a8aaa"));

    QPen pen(strokeColor);
    pen.setWidthF(1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Eye outline: two symmetric curves meeting at the outer corners.
    QPainterPath eyePath;
    eyePath.moveTo(2.5, size / 2.0);
    eyePath.quadTo(size / 2.0, 3.5, size - 2.5, size / 2.0);
    eyePath.quadTo(size / 2.0, size - 3.5, 2.5, size / 2.0);
    painter.drawPath(eyePath);

    // Pupil
    painter.setBrush(strokeColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(size / 2.0, size / 2.0), 2.6, 2.6);

    if (crossed) {
        // Diagonal slash — password is currently visible, click to hide.
        pen.setWidthF(1.8);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(3.5, size - 4.5), QPointF(size - 3.5, 4.5));
    }

    painter.end();
    return QIcon(pixmap);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Login::Login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    // All visual styling (colors, fonts, borders, hover/pressed states, etc.)
    // lives entirely in login.ui as the top-level widget's styleSheet
    // property. Nothing is set programmatically here — including the
    // status label's color, which is driven by a "status" dynamic
    // property toggled in setStatus() and matched via [status="..."]
    // selectors in the .ui stylesheet.

    // Password show/hide toggle (trailing action inside the field).
    // Starts hidden (echoMode is Password by default in login.ui), so the
    // plain (non-slashed) eye icon is shown first — clicking it reveals
    // the password.
    m_togglePasswordAction = ui->passwordEdit->addAction(
        makeEyeIcon(false), QLineEdit::TrailingPosition);
    m_togglePasswordAction->setText("Show");
    m_togglePasswordAction->setToolTip("Show/hide password");
    connect(m_togglePasswordAction, &QAction::triggered,
            this, &Login::togglePasswordVisibility);

    // Enter in username jumps to password; Enter in password submits.
    connect(ui->usernameEdit, &QLineEdit::returnPressed,
            ui->passwordEdit, qOverload<>(&QWidget::setFocus));
    connect(ui->passwordEdit, &QLineEdit::returnPressed,
            ui->loginButton, &QPushButton::click);

    ui->usernameEdit->setFocus();
}

Login::~Login()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString Login::hashPassword(const QString &plainText) const
{
    // The seed data in main.cpp stores MD5 hashes (hex, lowercase).
    return QString::fromLatin1(
        QCryptographicHash::hash(plainText.toUtf8(), QCryptographicHash::Md5).toHex()
        );
}

void Login::setStatus(const QString &message, bool isError)
{
    ui->statusLabel->setText(message);

    // Color is entirely determined by the .ui stylesheet via
    // QLabel#statusLabel[status="error"/"success"]. We only flip the
    // dynamic property here and ask the style system to re-evaluate it —
    // no colors/fonts are ever set from C++.
    ui->statusLabel->setProperty("status", isError ? "error" : "success");
    ui->statusLabel->style()->unpolish(ui->statusLabel);
    ui->statusLabel->style()->polish(ui->statusLabel);
    ui->statusLabel->update();
}

void Login::setBusy(bool busy)
{
    ui->loginButton->setEnabled(!busy);
    ui->loginButton->setText(busy ? "Signing in…" : "Log In");
    ui->usernameEdit->setEnabled(!busy);
    ui->passwordEdit->setEnabled(!busy);
}

void Login::togglePasswordVisibility()
{
    const bool showing = ui->passwordEdit->echoMode() == QLineEdit::Normal;
    ui->passwordEdit->setEchoMode(showing ? QLineEdit::Password : QLineEdit::Normal);

    // `showing` is the state *before* this toggle; after it, visibility is
    // the opposite. Swap in the matching icon (slashed while visible).
    const bool nowVisible = !showing;
    m_togglePasswordAction->setText(nowVisible ? "Hide" : "Show");
    m_togglePasswordAction->setIcon(makeEyeIcon(nowVisible));
}

void Login::openDashboard(int userId,
                          const QString &role,
                          const QString &username,
                          const QString &firstName,
                          const QString &lastName)
{
    Q_UNUSED(username)

    // Display name only - used for greeting text/dialog titles. Actual task
    // lookups now go through userId (information.id), not this string, so a
    // later name edit or two staff sharing a name can't cross tasks.
    const QString fullName = (firstName + " " + lastName).trimmed();

    if (role == "admin") {
        // AdminDashboard class name — matches admindashboard.h
        AdminDashboard *w = new AdminDashboard();
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();

    } else if (role == "staff") {
        // StaffDashboard class name — matches staffdashboard.h
        // Constructor is StaffDashboard(int staffId, const QString &staffName,
        // QWidget *parent = nullptr) — it forwards both straight into MyTasks,
        // which filters the `tasks` table by staff_id.
        StaffDashboard *w = new StaffDashboard(userId, fullName);
        w->setAttribute(Qt::WA_DeleteOnClose);

        // Attendance: this login IS the check-in. The check-out is
        // recorded when this window is destroyed (i.e. the user closes
        // their dashboard) — hooked via QObject::destroyed so nothing
        // inside StaffDashboard itself needs to change. See
        // attendancemanager.h for the on-time / late / early rules.
        AttendanceManager::recordCheckIn(userId);
        connect(w, &QObject::destroyed, this, [userId]() {
            AttendanceManager::recordCheckOut(userId);
        });

        w->show();

    } else if (role == "frontdesk") {
        // frontdesk class name (lower-case) — matches frontdesk.h exactly
        frontdesk *w = new frontdesk();
        w->setAttribute(Qt::WA_DeleteOnClose);

        // Same attendance hook as the staff branch above.
        AttendanceManager::recordCheckIn(userId);
        connect(w, &QObject::destroyed, this, [userId]() {
            AttendanceManager::recordCheckOut(userId);
        });

        w->show();

    } else {
        QMessageBox::warning(this, "Login Error",
                             "Unknown role \"" + role + "\". Contact your administrator.");
        return;
    }

    // Hide the login window so it isn't destroyed; dashboards can close
    // independently. Change hide() → close() if you prefer to destroy it.
    this->hide();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slots
// ─────────────────────────────────────────────────────────────────────────────

void Login::on_loginButton_clicked()
{
    const QString username = ui->usernameEdit->text().trimmed();
    const QString password = ui->passwordEdit->text();   // plain-text from field

    // ── Basic input validation (inline, no modal) ─────────────────────────────
    if (username.isEmpty() || password.isEmpty()) {
        setStatus("Please enter both username and password.", true);
        return;
    }

    setBusy(true);
    setStatus(QString(), false);

    // ── Hash the entered password to compare with stored MD5 hash ────────────
    const QString hashedPassword = hashPassword(password);

    // ── Use the default connection opened in main() ───────────────────────────
    // main() calls QSqlDatabase::addDatabase("QSQLITE") without a name,
    // which registers the "default" connection. database() without arguments
    // returns exactly that connection.
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        setBusy(false);
        QMessageBox::critical(this, "Database Error", "No open database connection.");
        return;
    }

    // `username` is UNIQUE in the schema, so SQLite already maintains an
    // index for this WHERE clause — the lookup below is a single indexed hit.
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, first_name, last_name, role, status "
        "FROM   information "
        "WHERE  username = :username "
        "  AND  password = :password "
        "LIMIT  1"
        );
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        setBusy(false);
        QMessageBox::critical(this, "Database Error",
                              "Login query failed: " + query.lastError().text());
        return;
    }

    if (!query.next()) {
        // No matching row in `information` — could be wrong credentials,
        // OR an account that's still awaiting admin approval and hasn't
        // been moved into `information` yet. checkPendingStatus() tells
        // the two apart and sets the appropriate status message.
        checkPendingStatus(username, hashedPassword);
        return;
    }

    // ── Row found: check account status ──────────────────────────────────────
    // Schema: status TEXT DEFAULT 'enabled' CHECK(status IN ('enabled','disabled','expired'))
    // Self-registrations never appear here with a "pending" status — they sit
    // in the separate `pending_requests` table until an admin approves them,
    // at which point they're inserted into `information` with status='enabled'.
    //
    // staff.cpp is the only place that ever changes this value post-seed
    // (Disable/Enable and Expire/Unexpire buttons on the staff table), so
    // the three branches below map 1:1 onto what an admin can set there.
    const int     userId    = query.value("id").toInt();
    const QString status    = query.value("status").toString().trimmed().toLower();
    const QString role      = query.value("role").toString();
    const QString firstName = query.value("first_name").toString();
    const QString lastName  = query.value("last_name").toString();

    if (status == "enabled") {
        // ── All good — open the correct dashboard ──────────────────────────
        setStatus("Login successful. Opening dashboard…", false);
        openDashboard(userId, role, username, firstName, lastName);
        return;
    }

    setBusy(false);

    if (status == "disabled") {
        setStatus("Your account has been disabled. Please contact your administrator.", true);
        ui->passwordEdit->clear();
        return;
    }

    if (status == "expired") {
        setStatus("Your account has expired. Please contact your administrator.", true);
        ui->passwordEdit->clear();
        return;
    }

    // Anything else is a data problem (bad manual DB edit, migration bug,
    // etc.) rather than an expected account state — surface it as a real
    // error instead of a generic status message.
    QMessageBox::critical(this, "Login Error",
                          QString("Unknown account status \"%1\". Contact your administrator.")
                              .arg(status));
}

void Login::checkPendingStatus(const QString &username, const QString &hashedPassword)
{
    // Reaching here means there was no matching row in `information` for
    // this username/password pair. Before concluding the credentials are
    // simply wrong, check whether this account is still sitting in the
    // self-registration queue — someone who just registered and hasn't
    // been approved yet should see "pending approval", not a generic
    // "invalid username or password" message.
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery pendingQuery(db);
    pendingQuery.prepare(
        "SELECT 1 FROM pending_requests "
        "WHERE username = :username AND password = :password "
        "LIMIT 1"
        );
    pendingQuery.bindValue(":username", username);
    pendingQuery.bindValue(":password", hashedPassword);

    if (!pendingQuery.exec()) {
        setBusy(false);
        QMessageBox::critical(this, "Database Error",
                              "Login query failed: " + pendingQuery.lastError().text());
        return;
    }

    setBusy(false);

    if (pendingQuery.next()) {
        // Username + password match a row still awaiting admin approval.
        setStatus("Your account is pending approval by an administrator. "
                  "Please check back later.", true);
        ui->passwordEdit->clear();
        return;
    }

    // No matching row in `information` OR `pending_requests` → the
    // username/password combination is simply wrong (this also covers a
    // previously-declined request, since declining permanently deletes the
    // row with no trace left behind).
    setStatus("Invalid username or password.", true);
    ui->passwordEdit->clear();
}

void Login::on_registerButton_clicked()
{
    // NOTE: adjust the class name below if register.h declares its window
    // class under a different name (this project's headers are inconsistent —
    // e.g. AdminDashboard/StaffDashboard use PascalCase, frontdesk uses
    // lower-case). "Register" is assumed here to match the register.h include.
    Register *w = new Register();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();

    // Hide (not close) for the same reason openDashboard() hides rather
    // than closes: keeps this window alive so it can be shown again later
    // without recreating it.
    this->hide();
}