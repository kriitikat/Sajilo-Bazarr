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
        QLabel#titleLabel {
            color: #1B2A4A;
            font-size: 22px;
            font-weight: 700;
            padding: 10px 0px;
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
        QPushButton#loginButton {
            background-color: #185FA5;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#loginButton:hover   { background-color: #2D4A7A; }
        QPushButton#loginButton:pressed { background-color: #1B2A4A; }
        QPushButton#registerButton {
            background-color: #FFFFFF;
            color: #1B2A4A;
            border: 1px solid #E0E4EE;
            border-radius: 8px;
            padding: 8px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton#registerButton:hover { background-color: #E8F1FB; color: #185FA5; }
    )");
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

    // ── Basic input validation ────────────────────────────────────────────────
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login", "Please enter both username and password.");
        return;
    }

    // ── Hash the entered password to compare with stored MD5 hash ────────────
    const QString hashedPassword = hashPassword(password);

    // ── Use the default connection opened in main() ───────────────────────────
    // main() calls QSqlDatabase::addDatabase("QSQLITE") without a name,
    // which registers the "default" connection.  database() without arguments
    // returns exactly that connection.
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "No open database connection.");
        return;
    }

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
        QMessageBox::critical(this, "Database Error",
                              "Login query failed: " + query.lastError().text());
        return;
    }

    if (!query.next()) {
        // No matching row → wrong username or password
        QMessageBox::warning(this, "Login Failed",
                             "Invalid username or password.\n"
                             "If you are new, please press Register.");
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
        QMessageBox::information(this, "Account pending",
                                 "Your account is still awaiting admin approval.\n"
                                 "Please try again later.");
        return;
    }

    if (status != "approved") {
        QMessageBox::information(this, "Account disabled",
                                 "Your account is disabled.\n"
                                 "Please contact your administrator.");
        return;
    }

    // ── All good — open the correct dashboard ─────────────────────────────────
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