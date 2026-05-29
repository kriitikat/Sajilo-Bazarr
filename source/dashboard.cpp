#include "../include/dashboard.h"

#include <QVBoxLayout>
#include <QLabel>

Dashboard::Dashboard(QString role,QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Dashboard");
    resize(400,300);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QLabel *label = new QLabel();

    if(role == "admin")
    {
        label->setText("Welcome Admin");
    }
    else
    {
        label->setText("Welcome Staff");
    }

    QVBoxLayout *layout = new QVBoxLayout();

    layout->addWidget(label);

    central->setLayout(layout);
}