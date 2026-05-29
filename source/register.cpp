#include "../include/"

#include <QtSql>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>

Register::Register(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Register");
    resize(400,300);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QLabel *title = new QLabel("Register User");

    QLineEdit *usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Username");

    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    QComboBox *roleBox = new QComboBox();
    roleBox->addItem("admin");
    roleBox->addItem("staff");

    QPushButton *createButton = new QPushButton("Create Account");

    QVBoxLayout *layout = new QVBoxLayout();

    layout->addWidget(title);
    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(roleBox);
    layout->addWidget(createButton);

    central->setLayout(layout);

    connect(createButton,&QPushButton::clicked,[=](){

        QString username = usernameEdit->text();
        QString password = passwordEdit->text();
        QString role = roleBox->currentText();

        QSqlQuery query;

        query.prepare("INSERT INTO users(username,password,role)"
                      "VALUES(?,?,?)");

        query.addBindValue(username);
        query.addBindValue(password);
        query.addBindValue(role);

        if(query.exec())
        {
            QMessageBox::information(this,"Success","Account Created");
        }
        else
        {
            QMessageBox::warning(this,"Error","Registration Failed");
        }
    });
}