#include "inventory.h"
#include "ui_inventory.h"
#include <QStringList>

inventory::inventory(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::inventory)
{
    ui->setupUi(this);

    // Set column headers matching "Manage Products" table
    QStringList headers;
    headers << "ID" << "Name" << "Category" << "Unit" << "Price" << "Stock" << "Expiry" << "Status" << "Supplier" << "SKU";
    ui->inventoryTable->setHorizontalHeaderLabels(headers);

    // Set column widths
    ui->inventoryTable->setColumnWidth(0, 40);
    ui->inventoryTable->setColumnWidth(1, 100);
    ui->inventoryTable->setColumnWidth(2, 90);
    ui->inventoryTable->setColumnWidth(3, 60);
    ui->inventoryTable->setColumnWidth(4, 70);
    ui->inventoryTable->setColumnWidth(5, 60);
    ui->inventoryTable->setColumnWidth(6, 90);
    ui->inventoryTable->setColumnWidth(7, 90);
    ui->inventoryTable->setColumnWidth(8, 100);
    ui->inventoryTable->horizontalHeader()->setStretchLastSection(true);
}

inventory::~inventory()
{
    delete ui;
}
