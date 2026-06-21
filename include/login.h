#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include <QtSql>

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private:
    bool connectDatabase();
};

#endif