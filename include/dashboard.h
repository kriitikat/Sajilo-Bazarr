#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    Dashboard(QString role,QWidget *parent = nullptr);
};

#endif