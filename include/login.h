#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();

private:
    Ui::Login *ui;


    QString hashPassword(const QString &plainText) const;


    void openDashboard(const QString &role,
                       const QString &username,
                       const QString &firstName,
                       const QString &lastName);
};

#endif