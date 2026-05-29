#include "../include/"
#include "../ui/"

Supplier::Supplier(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Supplier)
{
    ui->setupUi(this);
}

Supplier::~Supplier()
{
    delete ui;
}