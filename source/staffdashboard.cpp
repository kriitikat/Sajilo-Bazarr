// staffdashboard.cpp

#include "staffdashboard.h"
#include "ui_staffdashboard.h"

StaffDashboard::StaffDashboard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::StaffDashboard)
{
    ui->setupUi(this);
}

StaffDashboard::~StaffDashboard()
{
    delete ui;
}