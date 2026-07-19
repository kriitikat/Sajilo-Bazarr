#ifndef REGISTER_H
#define REGISTER_H

#include <QMainWindow>
#include <QPixmap>
#include <QSet>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class Register; }
QT_END_NAMESPACE

class QAction;
class QLabel;

class Register : public QMainWindow
{
    Q_OBJECT

public:
    // `loginWindow` is the Login window that opened this form. It is shown
    // again (and this window closed) once the user goes back or finishes
    // registering. It is NOT set as this window's Qt parent -- Register
    // still opens as its own independent top-level window, the same way
    // AdminDashboard / category / Product are opened elsewhere in the app.
    explicit Register(QWidget *loginWindow = nullptr);
    ~Register();

private slots:
    void on_submitButton_clicked();
    void on_backToLoginButton_clicked();
    void on_regenerateUsernameButton_clicked();
    void updateGeneratedUsername();

    // Live, as-you-type feedback (no need to wait for Submit to find out
    // an email/phone/password is invalid).
    void validateEmailLive();
    void validatePhoneLive();
    void updatePasswordStrength(const QString &text);
    void togglePasswordVisibility();
    void toggleConfirmPasswordVisibility();
    void on_uploadPhotoButton_clicked();

private:
    Ui::Register *ui;
    QWidget *m_loginWindow;
    QAction *m_togglePasswordAction        = nullptr;
    QAction *m_toggleConfirmPasswordAction = nullptr;

    // Regeneration session state: which base name/role we last generated
    // from (so a new person starts fresh), and which candidate usernames
    // the user has already seen/rejected via the Regenerate button (so it
    // never shows the same suggestion twice in a row).
    QString      m_lastBaseUsername;
    QSet<QString> m_rejectedUsernames;

    // Full path (on disk, wherever the user picked it from) of the photo
    // they selected via the Upload Photo button, if any. Empty means no
    // photo was chosen -- pending_requests.picture is left NULL, same as
    // the existing seed rows that have no picture.
    QString m_selectedPicturePath;

    QString hashPassword(const QString &plainText) const;
    QString sanitizeNamePart(const QString &part) const;
    QString generateUsername(const QString &firstName,
                             const QString &lastName,
                             const QString &role) const;
    QString pickUniqueCandidate(const QString &baseUsername);
    bool usernameExistsInDb(const QString &username) const;
    bool nameRoleExists(const QString &firstName,
                        const QString &lastName,
                        const QString &role) const;
    bool isValidEmail(const QString &email) const;
    bool isValidPhone(const QString &phone) const;
    int  passwordStrength(const QString &password) const;

    void setStatus(const QString &message, bool isError);

    // Renders `source` into avatarPreviewLabel as a circular 88x88 crop,
    // matching the avatar treatment already used in profile.ui /
    // frontprofile.ui's header.
    void updateAvatarPreview(const QPixmap &source);

    // Copies the picked photo into
    // C:\Users\ASUS\Desktop\Sajilo-Bazarr\resources\staff_photos as
    // "<username>.<ext>" and returns that filename -- stored in
    // the DB the same way information.picture already stores bare
    // filenames like "anushka.jpg" (not a full path). Returns an empty
    // string on failure.
    QString copyPictureToStorage(const QString &sourceFilePath,
                                 const QString &username) const;

    // ── State-only helpers (no colors/fonts touched here) ─────────────────
    // These just set text + a "state" dynamic property (e.g. "success",
    // "error", "warning", "info", "neutral") and ask Qt to re-evaluate the
    // widget's stylesheet. Every actual color/font lives in register.ui's
    // styleSheet property, matched via [state="..."] selectors.
    void setLabelState(QLabel *label, const QString &message, const QString &state);

    // score in [0,4] maps to a "strength" property ("weak"/"fair"/"good"/
    // "strong") read by register.ui's QProgressBar#passwordStrengthBar
    // selectors. score < 0 clears it back to the empty/default look.
    void setProgressStrength(int score);
};

#endif // REGISTER_H