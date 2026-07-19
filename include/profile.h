#ifndef PROFILE_H
#define PROFILE_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class Profile;
}
QT_END_NAMESPACE

// Read-only profile view for a single staff/admin/frontdesk account,
// pulled from 'information' by username. Everything is shown except
// the password. Username cannot be edited (it's the login key); the
// person can update their email and phone number and save the change
// back to the database.
class Profile : public QDialog
{
    Q_OBJECT
public:
    explicit Profile(const QString &username, QWidget *parent = nullptr);
    ~Profile() override;

signals:
    // Fired after a successful save, in case the caller (e.g. the
    // dashboard) wants to refresh anything that depends on this data.
    void profileUpdated();

private slots:
    void handleSaveClicked();
    void handleCancelClicked();

private:
    void loadUserData();
    static QPixmap makeCircularPixmap(const QPixmap &source, int diameter);
    static bool isValidEmail(const QString &email);
    static bool isValidPhone(const QString &phone);
    void showStatusMessage(const QString &text, bool isError);

    Ui::Profile *ui;

    QString m_username;
    QString m_originalEmail;
    QString m_originalPhone;

    static constexpr int AVATAR_DIAMETER = 96;
};

#endif // PROFILE_H
