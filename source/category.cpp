#include "category.h"
#include "../ui/ui_category.h"

category::category(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CATEGORY)
{
    ui->setupUi(this);
}

category::~category()
{
    delete ui;
}