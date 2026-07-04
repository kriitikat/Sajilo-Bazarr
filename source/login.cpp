#include "login.h"
#include "ui_login.h"

// ── Dashboard headers ─────────────────────────────────────────────────────────
// admindashboard.h / staffdashboard.h / frontdesk.h must all exist in the
// same source directory (flat layout that matches CMakeLists.txt).
#include "admindashboard.h"
#include "staffdashboard.h"
#include "frontdesk.h"
#include "register.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QAction>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Login::Login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    setWindowTitle("Sajilo Bazar - Login");

    setStyleSheet(R"(
        QMainWindow, QWidget#centralwidget {
            background-color: #F4F6FA;
        }

        /* ── Brand (left) panel ─────────────────────────────────────── */
        QFrame#brandPanel {
            background-color: #185FA5;
        }
        QLabel#logoMark {
            background-color: rgba(255, 255, 255, 0.15);
            border-radius: 28px;
            color: #FFFFFF;
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#brandTitle {
            color: #FFFFFF;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#brandSubtitle {
            color: #D7E6F7;
            font-size: 13px;
        }
        QFrame#brandDivider {
            background-color: rgba(255, 255, 255, 0.25);
        }
        QLabel#brandFeature1, QLabel#brandFeature2, QLabel#brandFeature3 {
            color: #E8F1FB;
            font-size: 13px;
        }

        /* ── Form (right) panel ─────────────────────────────────────── */
        QFrame#formPanel {
            background-color: #FFFFFF;
        }
        QFrame#card {
            background-color: #FFFFFF;
        }
        QLabel#titleLabel {
            color: #1B2A4A;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#subtitleLabel {
            color: #7a8aaa;
            font-size: 13px;
            padding-bottom: 6px;
        }
        QLabel#usernameLabel, QLabel#passwordLabel {
            color: #1B2A4A;
            font-size: 12px;
            font-weight: 600;
        }
        QLineEdit {
            border: 1px solid #E0E4EE;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 14px;
            background-color: #FFFFFF;
            color: #1B2A4A;
        }
        QLineEdit:focus {
            border: 1px solid #185FA5;
            background-color: #E8F1FB;
        }
        QCheckBox {
            color: #4a5a7a;
            font-size: 12px;
        }
        QLabel#statusLabel {
            font-size: 12px;
        }
        QPushButton#loginButton {
            background-color: #185FA5;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#loginButton:hover    { background-color: #2D4A7A; }
        QPushButton#loginButton:pressed  { background-color: #1B2A4A; }
        QPushButton#loginButton:disabled { background-color: #A9BCD8; }
        QPushButton#registerButton {
            background-color: #FFFFFF;
            color: #1B2A4A;
            border: 1px solid #E0E4EE;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton#registerButton:hover { background-color: #E8F1FB; color: #185FA5; }
    )");

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
    ui->statusLabel->setStyleSheet(isError
        ? "color: #C0392B; font-size: 12px;"
        : "color: #1E8449; font-size: 12px;");
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
    // independently.  Change hide() → close() if you prefer to destroy it.
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
    // which registers the "default" connection.  database() without arguments
    // returns exactly that connection.
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        setBusy(false);
        QMessageBox::critical(this, "Database Error", "No open database connection.");
        return;
    }

    // Static/prepared once per process: avoids re-parsing the same SQL text
    // on every login attempt (QSqlQuery::prepare() plans the statement, so
    // reusing a prepared QSqlQuery object across calls is the efficient path
    // when a class survives multiple attempts; kept local here since Login
    // is a short-lived window, but the -single- indexed lookup below is what
    // actually matters for speed: `username` is UNIQUE in the schema, so
    // SQLite already maintains an index for this WHERE clause.
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
        // No matching row → wrong username or password
        setBusy(false);
        setStatus("Invalid username or password. New here? Tap Create a new account below.", true);
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
        return;
    }

    // ── Row found: check account status ──────────────────────────────────────
    const QString status    = query.value("status").toString();
    const QString role      = query.value("role").toString();
    const QString firstName = query.value("first_name").toString();
    const QString lastName  = query.value("last_name").toString();

    if (status == "pending") {
        setBusy(false);
        setStatus("Your account is still awaiting admin approval.", true);
        return;
    }

    if (status != "approved") {
        setBusy(false);
        setStatus("Your account is disabled. Please contact your administrator.", true);
        return;
    }

    // ── All good — open the correct dashboard ─────────────────────────────────
    setStatus("Login successful. Opening dashboard…", false);
    openDashboard(role, username, firstName, lastName);
}

void Login::on_registerButton_clicked()
{
    // Pass `this` so Register can show the login window again (and close
    // itself) once the user goes back or finishes registering. This does
    // NOT make Register a Qt child window — it still opens as its own
    // independent top-level window, just like the dashboards do.
    Register *registerWindow = new Register(this);
    registerWindow->setAttribute(Qt::WA_DeleteOnClose);
    registerWindow->show();

    this->hide();
}
