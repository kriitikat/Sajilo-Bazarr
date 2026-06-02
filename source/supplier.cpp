#include "supplier.h"
#include "ui_supplier.h"

supplier::supplier(QWidget *parent)
    : QWidget(parent) // 1. Changed from QMainWindow to QWidget
    , ui(new Ui::supplier)
{
    ui->setupUi(this); // 2. Removed the forced setLayout line
}

supplier::~supplier()
{
    delete ui;
}
