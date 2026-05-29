#include "product.h"
#include "ui_product.h"
#include <QStringList>
#include <QHeaderView>
#include <QTableWidgetItem>

product::product(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::product)
{
    ui->setupUi(this);

    // Set column headers
    QStringList headers;
    headers << "ID" << "Product Name" << "Category" << "Price (Rs.)" << "Stock" << "Status" << "Actions";
    ui->productTable->setHorizontalHeaderLabels(headers);

    // Set column widths
    ui->productTable->setColumnWidth(0, 50);
    ui->productTable->setColumnWidth(1, 180);
    ui->productTable->setColumnWidth(2, 100);
    ui->productTable->setColumnWidth(3, 90);
    ui->productTable->setColumnWidth(4, 60);
    ui->productTable->setColumnWidth(5, 90);
    ui->productTable->horizontalHeader()->setStretchLastSection(true);
}

product::~product()
{
    delete ui;
}