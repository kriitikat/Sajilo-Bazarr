#include "../include/admindashboard.h"
#include "../ui/ui_admindashboard.h"
#include "../include/product.h"
#include "../include/inventory.h"

AdminDashboard::AdminDashboard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminDashboard)
{
    ui->setupUi(this);

    // Create product page and add as a new page in stackedWidget
    productPage = new Product();
    ui->stackedWidget->addWidget(productPage);

    // Create inventory page and add as a new page in stackedWidget
    inventoryPage = new inventory();
    ui->stackedWidget->addWidget(inventoryPage);
}

void AdminDashboard::on_btnDashboard_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageDashboard);
}

void AdminDashboard::on_btnProducts_clicked()
{
    ui->stackedWidget->setCurrentWidget(productPage);
}

void AdminDashboard::on_btnInventory_clicked()
{
    ui->stackedWidget->setCurrentWidget(inventoryPage);
}

AdminDashboard::~AdminDashboard()
{
    delete ui;
}