#include "../include/register.h"
#include "../ui/ui_register.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <QAction>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

namespace {
// A text-only QAction on a QLineEdit renders nothing at runtime — only an
// icon is shown by Qt's built-in line-edit action widget. Drawn here
// instead of shipping a separate icon asset. This is functional icon
// drawing, not a design/coloring choice, so it stays in code the same way
// any other procedurally-generated graphic would. Stroke color (#8A7580)
// matches the muted tone used by Login::makeEyeIcon() in login.cpp, kept
// in sync with register.ui's new Burgundy/Garnet palette.
QIcon eyeIcon(bool passwordVisible)
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor("#8A7580"));
    pen.setWidthF(1.6);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(2, 6, 16, 8));
    p.setBrush(QColor("#8A7580"));
    p.drawEllipse(QRectF(8, 8, 4, 4));
    if (passwordVisible)
        p.drawLine(QPointF(3, 17), QPointF(17, 3));
    return QIcon(pixmap);
}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Register::Register(QWidget *loginWindow)
    : QMainWindow(nullptr)
    , ui(new Ui::Register)
    , m_loginWindow(loginWindow)
{
    ui->setupUi(this);
    // All visual styling (colors, fonts, borders, hover/pressed/disabled
    // states, hint-label colors, password-strength-bar colors, etc.) lives
    // entirely in register.ui's styleSheet property. Nothing is set
    // programmatically here — runtime state changes only flip a "state" /
    // "strength" dynamic property that the .ui stylesheet selectors key off.

    // Role options — display text shown to the user vs. the value stored in
    // the database (must match the CHECK constraint on pending_requests.role).
    ui->roleCombo->addItem("Staff", "staff");
    ui->roleCombo->addItem("Front Desk", "frontdesk");

    ui->usernameEdit->setReadOnly(true);
    ui->passwordStrengthBar->setValue(0);

    // Password show/hide toggles.
    m_togglePasswordAction = ui->passwordEdit->addAction(eyeIcon(false), QLineEdit::TrailingPosition);
    m_togglePasswordAction->setToolTip("Show/hide password");
    connect(m_togglePasswordAction, &QAction::triggered,
            this, &Register::togglePasswordVisibility);

    m_toggleConfirmPasswordAction = ui->confirmPasswordEdit->addAction(eyeIcon(false), QLineEdit::TrailingPosition);
    m_toggleConfirmPasswordAction->setToolTip("Show/hide password");
    connect(m_toggleConfirmPasswordAction, &QAction::triggered,
            this, &Register::toggleConfirmPasswordVisibility);

    // Regenerate the username live as the user types/changes role.
    connect(ui->firstNameEdit, &QLineEdit::textChanged,
            this, &Register::updateGeneratedUsername);
    connect(ui->lastNameEdit, &QLineEdit::textChanged,
            this, &Register::updateGeneratedUsername);
    connect(ui->roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Register::updateGeneratedUsername);

    // Live inline validation as the user types, instead of only on Submit.
    connect(ui->emailEdit, &QLineEdit::textChanged,
            this, &Register::validateEmailLive);
    connect(ui->phoneEdit, &QLineEdit::textChanged,
            this, &Register::validatePhoneLive);
    connect(ui->passwordEdit, &QLineEdit::textChanged,
            this, &Register::updatePasswordStrength);

    // Note: submitButton/backToLoginButton are NOT wired here on purpose —
    // Qt's uic-generated setupUi() already connects them automatically via
    // connectSlotsByName(), since on_submitButton_clicked() and
    // on_backToLoginButton_clicked() follow the on_<objectName>_<signal>
    // naming convention (the same pattern Login already relies on for its
    // own buttons). Adding an explicit connect() on top of that made every
    // click fire twice — double DB insert, double success dialog.

    updateGeneratedUsername();
}

Register::~Register()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString Register::hashPassword(const QString &plainText) const
{
    // Same MD5-hex convention used by Login::hashPassword / the seed data.
    return QString::fromLatin1(
        QCryptographicHash::hash(plainText.toUtf8(), QCryptographicHash::Md5).toHex()
        );
}

QString Register::sanitizeNamePart(const QString &part) const
{
    QString cleaned = part.trimmed();
    cleaned.remove(QRegularExpression("[^A-Za-z]"));
    if (cleaned.isEmpty())
        return cleaned;
    return QString(cleaned.at(0).toUpper()) + cleaned.mid(1).toLower();
}

QString Register::generateUsername(const QString &firstName,
                                   const QString &lastName,
                                   const QString &role) const
{
    const QString first = sanitizeNamePart(firstName);
    const QString last  = sanitizeNamePart(lastName);

    QString roleLabel;
    if (role == "staff")
        roleLabel = "Staff";
    else if (role == "frontdesk")
        roleLabel = "Frontdesk";

    if (first.isEmpty() || last.isEmpty() || roleLabel.isEmpty())
        return QString();

    // Mirrors the "FirstName_LastName_Role" convention used in the seed data
    // (e.g. "Suyeshna_Shrestha_Staff").
    return first + "_" + last + "_" + roleLabel;
}

bool Register::usernameExistsInDb(const QString &username) const
{
    if (username.isEmpty())
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen())
        return false;

    // Username must be unique across BOTH tables so that once a pending
    // request is approved and moved into information there is no collision.
    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM information WHERE username = :u LIMIT 1");
    q.bindValue(":u", username);
    if (q.exec() && q.next())
        return true;

    q.prepare("SELECT 1 FROM pending_requests WHERE username = :u LIMIT 1");
    q.bindValue(":u", username);
    return q.exec() && q.next();
}

bool Register::nameRoleExists(const QString &firstName,
                              const QString &lastName,
                              const QString &role) const
{
    if (firstName.isEmpty() || lastName.isEmpty() || role.isEmpty())
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen())
        return false;

    // Same person/role combination can exist in either table: someone
    // already approved (information) or someone still awaiting approval
    // (pending_requests). Case-insensitive, since "John" and "john" are
    // the same person for this purpose.
    auto checkTable = [&](const QString &table) -> bool {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
                      "SELECT 1 FROM %1 "
                      "WHERE first_name = :fn COLLATE NOCASE "
                      "  AND last_name  = :ln COLLATE NOCASE "
                      "  AND role       = :role "
                      "LIMIT 1").arg(table));
        q.bindValue(":fn", firstName);
        q.bindValue(":ln", lastName);
        q.bindValue(":role", role);
        return q.exec() && q.next();
    };

    return checkTable("information") || checkTable("pending_requests");
}

QString Register::pickUniqueCandidate(const QString &baseUsername)
{
    if (baseUsername.isEmpty())
        return QString();

    // Same as uniqueUsername(), but also skips anything the user has
    // already been shown and dismissed via the Regenerate button this
    // session, so clicking it repeatedly never repeats a suggestion.
    QString candidate = baseUsername;
    int suffix = 2;
    while (usernameExistsInDb(candidate) || m_rejectedUsernames.contains(candidate)) {
        candidate = baseUsername + "_" + QString::number(suffix);
        ++suffix;
    }
    return candidate;
}

bool Register::isValidEmail(const QString &email) const
{
    static const QRegularExpression pattern(
        R"(^[\w\.\-]+@[\w\-]+\.[A-Za-z]{2,}$)"
        );
    return pattern.match(email).hasMatch();
}

bool Register::isValidPhone(const QString &phone) const
{
    static const QRegularExpression pattern(R"(^\d{7,15}$)");
    return pattern.match(phone).hasMatch();
}

int Register::passwordStrength(const QString &password) const
{
    // Simple 0-4 heuristic: length + character-class variety.
    int score = 0;
    if (password.length() >= 6)  ++score;
    if (password.length() >= 10) ++score;
    if (password.contains(QRegularExpression("[A-Z]")) &&
        password.contains(QRegularExpression("[a-z]"))) ++score;
    if (password.contains(QRegularExpression("[0-9]")) ||
        password.contains(QRegularExpression("[^A-Za-z0-9]"))) ++score;
    return score; // 0..4
}

void Register::setStatus(const QString &message, bool isError)
{
    setLabelState(ui->statusLabel, message, isError ? "error" : "success");
}

void Register::setLabelState(QLabel *label, const QString &message, const QString &state)
{
    label->setText(message);
    // Color/font live in register.ui's [state="..."] selectors — this just
    // flips the property and asks the style system to re-evaluate it.
    label->setProperty("state", state);
    label->style()->unpolish(label);
    label->style()->polish(label);
    label->update();
}

void Register::setProgressStrength(int score)
{
    static const char *barStrengths[] = {"weak", "weak", "fair", "good", "strong"};

    if (score < 0) {
        ui->passwordStrengthBar->setValue(0);
        ui->passwordStrengthBar->setProperty("strength", "");
    } else {
        ui->passwordStrengthBar->setValue(score);
        ui->passwordStrengthBar->setProperty("strength", barStrengths[score]);
    }
    ui->passwordStrengthBar->style()->unpolish(ui->passwordStrengthBar);
    ui->passwordStrengthBar->style()->polish(ui->passwordStrengthBar);
    ui->passwordStrengthBar->update();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slots
// ─────────────────────────────────────────────────────────────────────────────

void Register::updateGeneratedUsername()
{
    const QString firstName = ui->firstNameEdit->text().trimmed();
    const QString lastName  = ui->lastNameEdit->text().trimmed();
    const QString role      = ui->roleCombo->currentData().toString();
    const QString base      = generateUsername(firstName, lastName, role);

    // A different name/role combination starts a fresh regeneration
    // session — suffixes rejected for a previous person shouldn't carry
    // over and block a suffix that's actually free for this one.
    if (base != m_lastBaseUsername) {
        m_rejectedUsernames.clear();
        m_lastBaseUsername = base;
    }

    if (base.isEmpty()) {
        ui->usernameEdit->setText(QString());
        setLabelState(ui->usernameHintLabel, QString(), "neutral");
        ui->regenerateUsernameButton->setEnabled(false);
        return;
    }

    ui->regenerateUsernameButton->setEnabled(true);

    const bool duplicatePerson = nameRoleExists(firstName, lastName, role);
    const QString candidate    = pickUniqueCandidate(base);
    ui->usernameEdit->setText(candidate);

    if (duplicatePerson) {
        setLabelState(ui->usernameHintLabel,
                      "Someone with this name and role is already registered or pending. "
                      "A unique ID has been generated below — tap Regenerate for a different one.",
                      "error");
    } else {
        setLabelState(ui->usernameHintLabel, QString(), "neutral");
    }
}

void Register::on_regenerateUsernameButton_clicked()
{
    const QString current = ui->usernameEdit->text().trimmed();
    if (!current.isEmpty())
        m_rejectedUsernames.insert(current);

    if (m_lastBaseUsername.isEmpty())
        return;

    const QString candidate = pickUniqueCandidate(m_lastBaseUsername);
    ui->usernameEdit->setText(candidate);

    setLabelState(ui->usernameHintLabel, "New ID generated: " + candidate, "success");
}

void Register::validateEmailLive()
{
    const QString email = ui->emailEdit->text().trimmed();
    if (email.isEmpty()) {
        setLabelState(ui->emailHintLabel, QString(), "neutral");
        return;
    }
    if (isValidEmail(email))
        setLabelState(ui->emailHintLabel, "Looks good", "success");
    else
        setLabelState(ui->emailHintLabel, "Enter a valid email address", "error");
}

void Register::validatePhoneLive()
{
    const QString phone = ui->phoneEdit->text().trimmed();
    if (phone.isEmpty()) {
        setLabelState(ui->phoneHintLabel, QString(), "neutral");
        return;
    }
    if (isValidPhone(phone))
        setLabelState(ui->phoneHintLabel, "Looks good", "success");
    else
        setLabelState(ui->phoneHintLabel, "Digits only, 7-15 digits", "error");
}

void Register::updatePasswordStrength(const QString &text)
{
    if (text.isEmpty()) {
        setProgressStrength(-1);
        setLabelState(ui->passwordHintLabel, "At least 6 characters", "neutral");
        return;
    }

    const int score = passwordStrength(text);
    static const char *labels[]      = {"Too short", "Weak", "Fair", "Good", "Strong"};
    static const char *labelStates[] = {"error", "error", "warning", "info", "success"};

    setProgressStrength(score);
    setLabelState(ui->passwordHintLabel, labels[score], labelStates[score]);
}

void Register::togglePasswordVisibility()
{
    const bool showing = ui->passwordEdit->echoMode() == QLineEdit::Normal;
    ui->passwordEdit->setEchoMode(showing ? QLineEdit::Password : QLineEdit::Normal);
    m_togglePasswordAction->setIcon(eyeIcon(!showing));
}

void Register::toggleConfirmPasswordVisibility()
{
    const bool showing = ui->confirmPasswordEdit->echoMode() == QLineEdit::Normal;
    ui->confirmPasswordEdit->setEchoMode(showing ? QLineEdit::Password : QLineEdit::Normal);
    m_toggleConfirmPasswordAction->setIcon(eyeIcon(!showing));
}

void Register::on_submitButton_clicked()
{
    const QString firstName       = ui->firstNameEdit->text().trimmed();
    const QString lastName        = ui->lastNameEdit->text().trimmed();
    const QString role            = ui->roleCombo->currentData().toString();
    const QString email           = ui->emailEdit->text().trimmed();
    const QString phone           = ui->phoneEdit->text().trimmed();
    const QString password        = ui->passwordEdit->text();
    const QString confirmPassword = ui->confirmPasswordEdit->text();

    // ── Basic field validation (inline status, no modal for expected cases) ──
    if (firstName.isEmpty() || lastName.isEmpty() || email.isEmpty() ||
        phone.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        setStatus("Please fill in all fields.", true);
        return;
    }

    if (!isValidEmail(email)) {
        setStatus("Please enter a valid email address.", true);
        ui->emailEdit->setFocus();
        return;
    }

    if (!isValidPhone(phone)) {
        setStatus("Please enter a valid phone number (digits only, 7-15 digits).", true);
        ui->phoneEdit->setFocus();
        return;
    }

    if (password.length() < 6) {
        setStatus("Password must be at least 6 characters long.", true);
        ui->passwordEdit->setFocus();
        return;
    }

    if (password != confirmPassword) {
        setStatus("Passwords do not match.", true);
        ui->confirmPasswordEdit->clear();
        ui->confirmPasswordEdit->setFocus();
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "No open database connection.");
        return;
    }

    ui->submitButton->setEnabled(false);
    ui->submitButton->setText("Submitting…");

    // ── Reject duplicate emails (check both tables) ─────────────────────────
    {
        QSqlQuery emailCheck(db);
        emailCheck.prepare("SELECT 1 FROM information WHERE email = :email LIMIT 1");
        emailCheck.bindValue(":email", email);
        if (emailCheck.exec() && emailCheck.next()) {
            setStatus("An account with this email already exists.", true);
            ui->submitButton->setEnabled(true);
            ui->submitButton->setText("Submit for Approval");
            return;
        }

        emailCheck.prepare("SELECT 1 FROM pending_requests WHERE email = :email LIMIT 1");
        emailCheck.bindValue(":email", email);
        if (emailCheck.exec() && emailCheck.next()) {
            setStatus("A registration request with this email is already pending approval.", true);
            ui->submitButton->setEnabled(true);
            ui->submitButton->setText("Submit for Approval");
            return;
        }
    }

    // ── Use whatever's currently displayed — already resolved live, and
    //    possibly deliberately changed via the Regenerate button. Only
    //    regenerate here if someone else grabbed it in the moment between
    //    it being shown and Submit being clicked.
    QString username = ui->usernameEdit->text().trimmed();
    if (username.isEmpty()) {
        setStatus("Could not generate a username — please check the name fields.", true);
        ui->submitButton->setEnabled(true);
        ui->submitButton->setText("Submit for Approval");
        return;
    }
    if (usernameExistsInDb(username)) {
        username = pickUniqueCandidate(generateUsername(firstName, lastName, role));
        ui->usernameEdit->setText(username);
    }

    // ── Timestamp (local time, ISO-8601 format) ────────────────────────────
    const QString requestedAt =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // ── Insert into pending_requests (no status column needed) ─────────────
    //    The admin dashboard is responsible for reading this table and either
    //    moving the row into information (approved) or deleting it (rejected).
    QSqlQuery insertQuery(db);
    insertQuery.prepare(
        "INSERT INTO pending_requests "
        "(first_name, last_name, username, role, email, phone, password, requested_at) "
        "VALUES (:first_name, :last_name, :username, :role, :email, :phone, :password, :requested_at)"
        );
    insertQuery.bindValue(":first_name",   firstName);
    insertQuery.bindValue(":last_name",    lastName);
    insertQuery.bindValue(":username",     username);
    insertQuery.bindValue(":role",         role);
    insertQuery.bindValue(":email",        email);
    insertQuery.bindValue(":phone",        phone);
    insertQuery.bindValue(":password",     hashPassword(password));
    insertQuery.bindValue(":requested_at", requestedAt);

    if (!insertQuery.exec()) {
        ui->submitButton->setEnabled(true);
        ui->submitButton->setText("Submit for Approval");
        QMessageBox::critical(this, "Database Error",
                              "Could not submit registration: " + insertQuery.lastError().text());
        return;
    }

    QMessageBox::information(this, "Registration Submitted",
                             "Thank you, " + firstName + "!\n\n"
                                                         "Your account (username: " + username + ") has been "
                                              "submitted for admin approval. You'll be able to log in "
                                              "once an administrator approves it.");

    on_backToLoginButton_clicked();
}

void Register::on_backToLoginButton_clicked()
{
    if (m_loginWindow)
        m_loginWindow->show();

    this->close();
}