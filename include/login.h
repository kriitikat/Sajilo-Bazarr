#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

class QAction;

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();
    void togglePasswordVisibility();

private:
    Ui::Login *ui;
    QAction   *m_togglePasswordAction = nullptr;

    QString hashPassword(const QString &plainText) const;

    // userId = information.id for the row that just matched - needed so
    // StaffDashboard (and in turn MyTasks) can filter tasks by staff_id
    // instead of by name.
    void openDashboard(int userId,
                       const QString &role,
                       const QString &username,
                       const QString &firstName,
                       const QString &lastName);

    // Called when `username`/`hashedPassword` don't match any row in
    // `information`. Checks `pending_requests` for the same pair so a
    // freshly self-registered, not-yet-approved user sees "pending
    // approval" instead of a generic "invalid credentials" message.
    void checkPendingStatus(const QString &username, const QString &hashedPassword);

    // Inline feedback shown under the password field instead of a modal
    // popup for the common/expected cases (empty fields, wrong credentials,
    // disabled/expired accounts). QMessageBox is reserved for genuine,
    // unexpected failures (DB errors, unknown role).
    void setStatus(const QString &message, bool isError);
    void setBusy(bool busy);
};

#endif // LOGIN_H
