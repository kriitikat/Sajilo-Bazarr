#include "inventory.h"
#include "ui_inventory.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QDebug>

inventory::inventory(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::inventory)
{
    ui->setupUi(this);

    // Column headers, matching the "products" table column order
    const QStringList headers = {
        "ID", "Product Name", "Category", "Unit", "Price",
        "Stock", "Expiry Date", "Status", "Supplier", "SKU"
    };
    ui->inventoryTable->setColumnCount(headers.size());
    ui->inventoryTable->setHorizontalHeaderLabels(headers);
    ui->inventoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->inventoryTable->verticalHeader()->setVisible(false);

    // Wire up the sort combo box (named manually so it doesn't collide
    // with Qt's automatic on_<object>_<signal> slot connection)
    connect(ui->sortInventory,
            &QComboBox::currentIndexChanged,
            this,
            &inventory::handleSortChanged);

    loadInventoryData();
}

inventory::~inventory()
{
    delete ui;
}

void inventory::loadInventoryData()
{
    QSqlQuery query;
    if (!query.exec("SELECT id, product_name, category, unit, price, stock, "
                    "expiry_date, status, supplier, sku FROM products ORDER BY id")) {
        qWarning() << "Inventory: failed to load products -" << query.lastError().text();
        return;
    }

    ui->inventoryTable->setRowCount(0);

    int totalCount = 0;
    int lowCount = 0;
    int highCount = 0;
    int outCount = 0;
    int row = 0;

    while (query.next()) {
        ui->inventoryTable->insertRow(row);

        for (int col = 0; col < 10; ++col) {
            auto *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->inventoryTable->setItem(row, col, item);
        }

        const QString status = query.value(7).toString(); // "status" column
        if (status.compare("Low Stock", Qt::CaseInsensitive) == 0)
            ++lowCount;
        else if (status.compare("High Stock", Qt::CaseInsensitive) == 0)
            ++highCount;
        else if (status.compare("Out Of Stock", Qt::CaseInsensitive) == 0)
            ++outCount;

        ++totalCount;
        ++row;
    }

    ui->lblTotalValue->setText(QString::number(totalCount));
    ui->lblLowStockValue->setText(QString::number(lowCount));
    ui->lblHighStockValue->setText(QString::number(highCount));
    ui->lblOutOfStockValue->setText(QString::number(outCount));
}

void inventory::handleSortChanged(int index)
{
    switch (index) {
    case 1: // Name (A-Z)
        ui->inventoryTable->sortItems(1, Qt::AscendingOrder);
        break;
    case 2: // Stock (Low to High)
        ui->inventoryTable->sortItems(5, Qt::AscendingOrder);
        break;
    case 3: // Expiry Date
        ui->inventoryTable->sortItems(6, Qt::AscendingOrder);
        break;
    default:
        break; // "Sort Inventory" placeholder - no-op
    }
}