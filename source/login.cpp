#include "../include/login.h"
#include "../include/register.h"
#include "../include/dashboard.h"

#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

Login::Login(QWidget *parent)
    : QMainWindow(parent)
{
    // WINDOW
    setWindowTitle("Inventory Login");
    resize(400,300);

    // WIDGETS
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QLabel *title = new QLabel("Login System");

    QLineEdit *usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Username");

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    QPushButton *loginButton = new QPushButton("Login");
    QPushButton *registerButton = new QPushButton("Register");

    // LAYOUT
    QVBoxLayout *layout = new QVBoxLayout();

    layout->addWidget(title);
    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(loginButton);
    layout->addWidget(registerButton);

    central->setLayout(layout);

    // DATABASE CONNECTION
    connectDatabase();

    // LOGIN BUTTON
    connect(loginButton,&QPushButton::clicked,[=](){

        QString username = usernameEdit->text();
        QString password = passwordEdit->text();

        QSqlQuery query;

        query.prepare("SELECT role FROM users WHERE username=? AND password=?");

        query.addBindValue(username);
        query.addBindValue(password);

        if(query.exec() && query.next())
        {
            QString role = query.value(0).toString();

            Dashboard *dashboard = new Dashboard(role);
            dashboard->show();

            this->hide();
        }
        else
        {
            QMessageBox::warning(this,"Error","Invalid Username or Password");
        }
    });

    // REGISTER BUTTON
    connect(registerButton,&QPushButton::clicked,[=](){

        Register *reg = new Register();
        reg->show();
    });
}

bool Login::connectDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

    db.setHostName("localhost");
    db.setDatabaseName("inventory_system");
    db.setUserName("root");
    db.setPassword("root");

    if(db.open())
    {
        qDebug() << "Database Connected";
        return true;
    }
    else
    {
        QMessageBox::critical(this,"Error","Database Failed");
        return false;
    }
}

Login::~Login()
{
}