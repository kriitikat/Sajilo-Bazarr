#include "satffpage.h"
#include "ui_satffpage.h"

staff::staff(QWidget *parent)
    : QWidget(parent) // 1. Changed from QMainWindow to QWidget
    , ui(new Ui::staff)
{
    ui->setupUi(this); // 2. Removed the forced setLayout line
}

staff::~staff()
{
    delete ui;
}