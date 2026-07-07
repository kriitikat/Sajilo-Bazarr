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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QAction>
#include <QStyle>

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
    m_togglePasswordAction = ui->passwordEdit->addAction(
        QIcon(), QLineEdit::TrailingPosition);
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
    m_togglePasswordAction->setText(showing ? "Show" : "Hide");
}

void Login::openDashboard(const QString &role,
                          const QString &username,
                          const QString &firstName,
                          const QString &lastName)
{
    Q_UNUSED(username)
    Q_UNUSED(firstName)
    Q_UNUSED(lastName)

    if (role == "admin") {
        // AdminDashboard class name — matches admindashboard.h
        AdminDashboard *w = new AdminDashboard();
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();

    } else if (role == "staff") {
        // StaffDashboard class name — matches staffdashboard.h
        StaffDashboard *w = new StaffDashboard();
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();

    } else if (role == "frontdesk") {
        // frontdesk class name (lower-case) — matches frontdesk.h exactly
        frontdesk *w = new frontdesk();
        w->setAttribute(Qt::WA_DeleteOnClose);
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
    const QString status    = query.value("status").toString();
    const QString role      = query.value("role").toString();
    const QString firstName = query.value("first_name").toString();
    const QString lastName  = query.value("last_name").toString();

    if (status == "expired") {
        setBusy(false);
        setStatus("Your account has expired. Please contact your administrator.", true);
        return;
    }

    if (status != "enabled") {
        // Covers "disabled" and any unexpected/unknown value.
        setBusy(false);
        setStatus("Your account is disabled. Please contact your administrator.", true);
        return;
    }

    // ── All good — open the correct dashboard ─────────────────────────────────
    setStatus("Login successful. Opening dashboard…", false);
    openDashboard(role, username, firstName, lastName);
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