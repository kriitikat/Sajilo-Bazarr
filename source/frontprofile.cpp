#include "../include/frontprofile.h"
#include "../ui/ui_frontprofile.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QDebug>

FrontProfile::FrontProfile(const QString &username, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FrontProfile)
    , m_username(username)
{
    ui->setupUi(this);
    setWindowTitle("My Profile");

    connect(ui->btnSave,       &QPushButton::clicked, this, &FrontProfile::handleSaveClicked);
    connect(ui->btnCancel,     &QPushButton::clicked, this, &FrontProfile::handleCancelClicked);
    connect(ui->btnEditEmail,  &QPushButton::clicked, this, &FrontProfile::handleEditEmailClicked);
    connect(ui->btnEditPhone,  &QPushButton::clicked, this, &FrontProfile::handleEditPhoneClicked);
    connect(ui->btnChangePhoto, &QPushButton::clicked, this, &FrontProfile::handleChangePhotoClicked);

    ui->statusMessageLabel->setVisible(false);

    loadUserData();
}

FrontProfile::~FrontProfile()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  Pulls every column except 'password' for this username and
//  populates the dialog. Username/first name/last name/role/status
//  are shown read-only; email + phone are editable, but locked
//  behind their own Edit button until the person chooses to change
//  them.
// ─────────────────────────────────────────────────────────────────
void FrontProfile::loadUserData()
{
    if (m_username.isEmpty()) {
        showStatusMessage("No user is currently logged in.", true);
        ui->btnSave->setEnabled(false);
        ui->btnEditEmail->setEnabled(false);
        ui->btnEditPhone->setEnabled(false);
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT first_name, last_name, username, role, email, phone, "
              "picture, status FROM information WHERE username = :u");
    q.bindValue(":u", m_username);

    if (!q.exec() || !q.next()) {
        qWarning() << "FrontProfile: failed to load user -" << q.lastError().text();
        showStatusMessage("Could not load profile data.", true);
        ui->btnSave->setEnabled(false);
        ui->btnEditEmail->setEnabled(false);
        ui->btnEditPhone->setEnabled(false);
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

    // Email/phone always load locked; the person has to explicitly
    // hit Edit to unlock whichever field they want to change.
    setFieldEditable(ui->leEmail, ui->btnEditEmail, false);
    setFieldEditable(ui->lePhone, ui->btnEditPhone, false);

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
        p.setBrush(QColor("#660033"));
        p.drawEllipse(0, 0, AVATAR_DIAMETER, AVATAR_DIAMETER);

        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(AVATAR_DIAMETER / 2);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(avatar.rect(), Qt::AlignCenter, initials);
    }

    ui->avatarLabel->setPixmap(makeCircularPixmap(avatar, AVATAR_DIAMETER));

    // Baseline for Cancel to restore to, and a clean slate for a fresh
    // Change Photo pick each time the dialog reloads.
    m_originalAvatarPixmap = makeCircularPixmap(avatar, AVATAR_DIAMETER);
    m_pendingPicturePath.clear();
}

QPixmap FrontProfile::makeCircularPixmap(const QPixmap &source, int diameter)
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

bool FrontProfile::isValidEmail(const QString &email)
{
    static const QRegularExpression pattern(
        R"(^[\w.+-]+@[\w-]+\.[A-Za-z]{2,}$)");
    return pattern.match(email).hasMatch();
}

bool FrontProfile::isValidPhone(const QString &phone)
{
    static const QRegularExpression pattern(R"(^\d{7,15}$)");
    return pattern.match(phone).hasMatch();
}

void FrontProfile::showStatusMessage(const QString &text, bool isError)
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
//  Flips a field between locked/unlocked. All the actual colors for
//  each state are defined in frontprofile.ui via the "fieldEditable"
//  dynamic-property QSS selectors on the QLineEdit; this just sets
//  the property and asks the style to repaint, plus updates the
//  read-only flag and button label.
// ─────────────────────────────────────────────────────────────────
void FrontProfile::setFieldEditable(QLineEdit *field, QPushButton *editButton, bool editable)
{
    if (!field || !editButton)
        return;

    field->setReadOnly(!editable);
    field->setProperty("fieldEditable", editable);
    field->style()->unpolish(field);
    field->style()->polish(field);
    field->update();

    editButton->setText(editable ? "Lock" : "Edit");

    if (editable) {
        field->setFocus();
        field->selectAll();
    }
}

void FrontProfile::handleEditEmailClicked()
{
    const bool nowEditable = ui->leEmail->isReadOnly();
    setFieldEditable(ui->leEmail, ui->btnEditEmail, nowEditable);

    if (!nowEditable) {
        // Field was just relocked without saving — snap back to the
        // last saved value so nothing dangling gets left in the box.
        ui->leEmail->setText(m_originalEmail);
    }
}

void FrontProfile::handleEditPhoneClicked()
{
    const bool nowEditable = ui->lePhone->isReadOnly();
    setFieldEditable(ui->lePhone, ui->btnEditPhone, nowEditable);

    if (!nowEditable) {
        ui->lePhone->setText(m_originalPhone);
    }
}

// ─────────────────────────────────────────────────────────────────
//  Lets the person pick a new photo and previews it immediately, but
//  doesn't touch disk or the DB yet -- that only happens once they
//  hit Save, same as email/phone. Cancel (or just closing without
//  saving) discards the pick and reverts the preview.
// ─────────────────────────────────────────────────────────────────
void FrontProfile::handleChangePhotoClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Select Profile Photo", QString(),
        "Images (*.png *.jpg *.jpeg)");

    if (filePath.isEmpty())
        return;

    QFileInfo info(filePath);
    if (info.size() > 5 * 1024 * 1024) {
        showStatusMessage("That photo is larger than 5 MB. Please choose a smaller image.", true);
        return;
    }

    QPixmap picked(filePath);
    if (picked.isNull()) {
        showStatusMessage("Could not load that image. Please choose a valid PNG or JPG file.", true);
        return;
    }

    m_pendingPicturePath = filePath;
    ui->avatarLabel->setPixmap(makeCircularPixmap(picked, AVATAR_DIAMETER));
    showStatusMessage("Photo selected — click Save Changes to apply it.", false);
}

QString FrontProfile::copyPictureToStorage(const QString &sourceFilePath) const
{
    // Same folder/naming convention as register.cpp's
    // copyPictureToStorage(): a bare "<username>.<ext>" filename, stored
    // in information.picture the same way the seed data already does
    // (e.g. "anushka.jpg", not a full path).
    QDir dir(QStringLiteral("C:/Users/ASUS/Desktop/Sajilo-Bazarr/resources/staff_photos"));
    if (!dir.exists() && !QDir().mkpath(dir.path()))
        return QString();

    const QString ext = QFileInfo(sourceFilePath).suffix().toLower();
    const QString destFileName = m_username + (ext.isEmpty() ? QString() : "." + ext);
    const QString destPath = dir.filePath(destFileName);

    // Overwrite any previous photo for this username.
    QFile::remove(destPath);

    if (!QFile::copy(sourceFilePath, destPath))
        return QString();

    return destFileName;
}

// ─────────────────────────────────────────────────────────────────
//  Validates + saves email/phone only. Username, name, role and
//  status are never written back from this dialog.
// ─────────────────────────────────────────────────────────────────
void FrontProfile::handleSaveClicked()
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

    if (newEmail == m_originalEmail && newPhone == m_originalPhone && m_pendingPicturePath.isEmpty()) {
        showStatusMessage("No changes to save.", false);
        return;
    }

    QString newPictureFileName;
    const bool photoChanged = !m_pendingPicturePath.isEmpty();
    if (photoChanged) {
        newPictureFileName = copyPictureToStorage(m_pendingPicturePath);
        if (newPictureFileName.isEmpty()) {
            showStatusMessage("Failed to save the selected photo. Please try again.", true);
            return;
        }
    }

    QSqlQuery q;
    if (photoChanged) {
        q.prepare("UPDATE information SET email = :e, phone = :p, picture = :pic WHERE username = :u");
        q.bindValue(":pic", newPictureFileName);
    } else {
        q.prepare("UPDATE information SET email = :e, phone = :p WHERE username = :u");
    }
    q.bindValue(":e", newEmail);
    q.bindValue(":p", newPhone);
    q.bindValue(":u", m_username);

    if (!q.exec()) {
        qWarning() << "FrontProfile: update failed -" << q.lastError().text();
        showStatusMessage("Failed to save changes. Please try again.", true);
        return;
    }

    m_originalEmail = newEmail;
    m_originalPhone = newPhone;

    if (photoChanged) {
        // The preview already shows the new photo (set on pick) -- just
        // make it the new baseline and clear the pending state.
        m_originalAvatarPixmap = ui->avatarLabel->pixmap();
        m_pendingPicturePath.clear();
    }

    // Relock both fields back to their default read-only state now
    // that the change is committed.
    setFieldEditable(ui->leEmail, ui->btnEditEmail, false);
    setFieldEditable(ui->lePhone, ui->btnEditPhone, false);

    showStatusMessage("Profile updated successfully.", false);
    emit profileUpdated();
}

void FrontProfile::handleCancelClicked()
{
    // Discard any unsaved edits, relock both fields, and close.
    ui->leEmail->setText(m_originalEmail);
    ui->lePhone->setText(m_originalPhone);
    setFieldEditable(ui->leEmail, ui->btnEditEmail, false);
    setFieldEditable(ui->lePhone, ui->btnEditPhone, false);

    if (!m_pendingPicturePath.isEmpty()) {
        m_pendingPicturePath.clear();
        ui->avatarLabel->setPixmap(m_originalAvatarPixmap);
    }

    reject();
}