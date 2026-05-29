#include "supplier.h"
#include "ui_supplier.h"

Supplier::Supplier(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Supplier)
{
    ui->setupUi(this);
}

Supplier::~Supplier()
{
    delete ui;
}