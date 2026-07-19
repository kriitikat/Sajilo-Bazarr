#ifndef FRONTPROFILE_H
#define FRONTPROFILE_H

#include <QDialog>
#include <QPixmap>
#include <QString>

class QLineEdit;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class FrontProfile;
}
QT_END_NAMESPACE

// Read-only profile view for a single staff/admin/frontdesk account,
// pulled from 'information' by username. Everything is shown except
// the password. Username cannot be edited (it's the login key); the
// person can update their email and phone number — each unlocked
// individually via its own Edit button — and save the change back
// to the database.
class FrontProfile : public QDialog
{
    Q_OBJECT
public:
    explicit FrontProfile(const QString &username, QWidget *parent = nullptr);
    ~FrontProfile() override;

signals:
    // Fired after a successful save, in case the caller (e.g. the
    // dashboard) wants to refresh anything that depends on this data.
    void profileUpdated();

private slots:
    void handleSaveClicked();
    void handleCancelClicked();
    void handleEditEmailClicked();
    void handleEditPhoneClicked();
    void handleChangePhotoClicked();

private:
    void loadUserData();
    static QPixmap makeCircularPixmap(const QPixmap &source, int diameter);
    static bool isValidEmail(const QString &email);
    static bool isValidPhone(const QString &phone);
    void showStatusMessage(const QString &text, bool isError);

    // Toggles a field between its locked (read-only) and unlocked
    // (editable) visual/interactive state. The actual colors for
    // each state live in frontprofile.ui via the "fieldEditable"
    // dynamic property selectors — this just flips the property and
    // re-polishes.
    void setFieldEditable(QLineEdit *field, QPushButton *editButton, bool editable);

    // Copies a newly-picked photo into the shared staff photo folder as
    // "<username>.<ext>" -- same convention/location register.cpp's
    // copyPictureToStorage() already uses -- and returns that bare
    // filename for storing in information.picture. Empty on failure.
    QString copyPictureToStorage(const QString &sourceFilePath) const;

    Ui::FrontProfile *ui;

    QString m_username;
    QString m_originalEmail;
    QString m_originalPhone;

    // Path (on disk) of a photo picked via Change Photo but not yet
    // saved. The preview updates immediately on pick; the actual file
    // copy + DB write only happens on Save. Cleared on Save or Cancel.
    QString m_pendingPicturePath;

    // The circular avatar as it looked before any pending photo pick,
    // so Cancel can restore it without re-hitting the DB.
    QPixmap m_originalAvatarPixmap;

    static constexpr int AVATAR_DIAMETER = 96;
};

#endif // FRONTPROFILE_H
