#include "../include/profile.h"
#include "../ui/ui_profile.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDebug>

Profile::Profile(const QString &username, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Profile)
    , m_username(username)
{
    ui->setupUi(this);
    setWindowTitle("My Profile");

    connect(ui->btnSave,   &QPushButton::clicked, this, &Profile::handleSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &Profile::handleCancelClicked);

    ui->statusMessageLabel->setVisible(false);

    loadUserData();
}

Profile::~Profile()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  Pulls every column except 'password' for this username and
//  populates the dialog. Username/first name/last name/role/status
//  are shown read-only; email + phone are editable.
// ─────────────────────────────────────────────────────────────────
void Profile::loadUserData()
{
    if (m_username.isEmpty()) {
        showStatusMessage("No user is currently logged in.", true);
        ui->btnSave->setEnabled(false);
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT first_name, last_name, username, role, email, phone, "
              "picture, status FROM information WHERE username = :u");
    q.bindValue(":u", m_username);

    if (!q.exec() || !q.next()) {
        qWarning() << "Profile: failed to load user -" << q.lastError().text();
        showStatusMessage("Could not load profile data.", true);
        ui->btnSave->setEnabled(false);
        return;
    }

    const QString firstName = q.value(0).toString();
    const QString lastName  = q.value(1).toString();
    const QString username  = q.value(2).toString();
    const QString role      = q.value(3).toString();
    const QString email     = q.value(4).toString();
    const QString phone     = q.value(5).toString();
    const QString picture   = q.value(6).toString();
    const QString status    = q.value(7).toString();

    ui->nameLabel->setText(QStringLiteral("%1 %2").arg(firstName, lastName));

    QString roleDisplay = role;
    if (!roleDisplay.isEmpty())
        roleDisplay[0] = roleDisplay[0].toUpper();
    ui->roleBadgeLabel->setText(roleDisplay);

    ui->leFirstName->setText(firstName);
    ui->leLastName->setText(lastName);
    ui->leUsername->setText(username);

    QString statusDisplay = status;
    if (!statusDisplay.isEmpty())
        statusDisplay[0] = statusDisplay[0].toUpper();
    ui->leStatus->setText(statusDisplay);

    ui->leEmail->setText(email);
    ui->lePhone->setText(phone);

    m_originalEmail = email;
    m_originalPhone = phone;

    // ── Avatar ───────────────────────────────────────────────────
    QPixmap avatar;
    bool loaded = false;

    if (!picture.isEmpty()) {
        const QString diskPath =
            QCoreApplication::applicationDirPath() + "/resources/staff_photos/" + picture;

        if (QFile::exists(diskPath))
            loaded = avatar.load(diskPath);

        if (!loaded)
            loaded = avatar.load(":/staff_photos/" + picture);
    }

    if (!loaded) {
        QString initials;
        if (!firstName.isEmpty()) initials += firstName.at(0).toUpper();
        if (!lastName.isEmpty())  initials += lastName.at(0).toUpper();
        if (initials.isEmpty())   initials = "?";

        avatar = QPixmap(AVATAR_DIAMETER, AVATAR_DIAMETER);
        avatar.fill(Qt::transparent);

        QPainter p(&avatar);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#4A9EE8"));
        p.drawEllipse(0, 0, AVATAR_DIAMETER, AVATAR_DIAMETER);

        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(AVATAR_DIAMETER / 2);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(avatar.rect(), Qt::AlignCenter, initials);
    }

    ui->avatarLabel->setPixmap(makeCircularPixmap(avatar, AVATAR_DIAMETER));
}

QPixmap Profile::makeCircularPixmap(const QPixmap &source, int diameter)
{
    QPixmap scaled = source.scaled(diameter, diameter,
                                    Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation);

    QPixmap circular(diameter, diameter);
    circular.fill(Qt::transparent);

    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath clipPath;
    clipPath.addEllipse(0, 0, diameter, diameter);
    painter.setClipPath(clipPath);

    const int x = (diameter - scaled.width())  / 2;
    const int y = (diameter - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    return circular;
}

bool Profile::isValidEmail(const QString &email)
{
    static const QRegularExpression pattern(
        R"(^[\w.+-]+@[\w-]+\.[A-Za-z]{2,}$)");
    return pattern.match(email).hasMatch();
}

bool Profile::isValidPhone(const QString &phone)
{
    static const QRegularExpression pattern(R"(^\d{7,15}$)");
    return pattern.match(phone).hasMatch();
}

void Profile::showStatusMessage(const QString &text, bool isError)
{
    ui->statusMessageLabel->setText(text);
    ui->statusMessageLabel->setStyleSheet(
        isError
            ? "QLabel { color: #C0392B; background-color: #FDEDEC; "
              "border-radius: 6px; padding: 8px 12px; font-size: 12px; }"
            : "QLabel { color: #1E7E34; background-color: #E6F7EA; "
              "border-radius: 6px; padding: 8px 12px; font-size: 12px; }");
    ui->statusMessageLabel->setVisible(true);
}

// ─────────────────────────────────────────────────────────────────
//  Validates + saves email/phone only. Username, name, role and
//  status are never written back from this dialog.
// ─────────────────────────────────────────────────────────────────
void Profile::handleSaveClicked()
{
    const QString newEmail = ui->leEmail->text().trimmed();
    const QString newPhone = ui->lePhone->text().trimmed();

    if (newEmail.isEmpty() || newPhone.isEmpty()) {
        showStatusMessage("Email and phone cannot be empty.", true);
        return;
    }

    if (!isValidEmail(newEmail)) {
        showStatusMessage("Please enter a valid email address.", true);
        return;
    }

    if (!isValidPhone(newPhone)) {
        showStatusMessage("Please enter a valid phone number (digits only).", true);
        return;
    }

    if (newEmail == m_originalEmail && newPhone == m_originalPhone) {
        showStatusMessage("No changes to save.", false);
        return;
    }

    QSqlQuery q;
    q.prepare("UPDATE information SET email = :e, phone = :p WHERE username = :u");
    q.bindValue(":e", newEmail);
    q.bindValue(":p", newPhone);
    q.bindValue(":u", m_username);

    if (!q.exec()) {
        qWarning() << "Profile: update failed -" << q.lastError().text();
        showStatusMessage("Failed to save changes. Please try again.", true);
        return;
    }

    m_originalEmail = newEmail;
    m_originalPhone = newPhone;

    showStatusMessage("Profile updated successfully.", false);
    emit profileUpdated();
}

void Profile::handleCancelClicked()
{
    // Discard any unsaved edits and close.
    ui->leEmail->setText(m_originalEmail);
    ui->lePhone->setText(m_originalPhone);
    reject();
}
