#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
class Login;
}
QT_END_NAMESPACE

class Login : public QMainWindow
{
    Q_OBJECT

public:
    Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();

private:
    Ui::Login *ui;

<<<<<<< HEAD

    QString hashPassword(const QString &plainText) const;


    void openDashboard(const QString &role,
                       const QString &username,
                       const QString &firstName,
                       const QString &lastName);
=======
    bool connectDatabase();
>>>>>>> 8364bbfabed1b7d2cc57b69230f1e1f88628ef97
};

#endif