#include "../include/inventory.h"
#include "../ui/ui_inventory.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>

inventory::inventory(QWidget *parent)
    : BackBase<QMainWindow>(parent)
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

    // Sort dropdown
    connect(ui->sortInventory,
            &QComboBox::currentIndexChanged,
            this,
            &inventory::handleSortChanged);

    // Prev / Next buttons
    connect(ui->btnPrev, &QPushButton::clicked, this, &inventory::goToPreviousPage);
    connect(ui->btnNext, &QPushButton::clicked, this, &inventory::goToNextPage);

    // Back to Dashboard — inherited from BackBase<QMainWindow>. Closing
    // this window is enough: AdminDashboard is already open underneath it
    // (see admindashboard.cpp's handleInventory_clicked()).
    wireBackButton(ui->btnBackToDashboard);

    // Make stat boxes clickable for filtering
    ui->cardTotal->installEventFilter(this);
    ui->cardLowStock->installEventFilter(this);
    ui->cardHighStock->installEventFilter(this);
    ui->cardOutOfStock->installEventFilter(this);

    updateStatCards();              // counts, always based on full table
    loadInventoryData(currentPage); // table rows, based on filter + page
}

inventory::~inventory()
{
    delete ui;
}

// Always counts ALL products in the database, regardless of currentFilter/page.
// This keeps the 4 stat cards stable no matter what filter is applied.
void inventory::updateStatCards()
{
    QSqlQuery query;
    if (!query.exec("SELECT status, COUNT(*) FROM products GROUP BY status")) {
        qWarning() << "Inventory: failed to load stat counts -" << query.lastError().text();
        return;
    }

    int totalCount = 0;
    int lowCount = 0;
    int highCount = 0;
    int outCount = 0;

    while (query.next()) {
        const QString status = query.value(0).toString();
        const int count = query.value(1).toInt();

        totalCount += count;

        if (status.compare("Low Stock", Qt::CaseInsensitive) == 0)
            lowCount = count;
        else if (status.compare("High Stock", Qt::CaseInsensitive) == 0)
            highCount = count;
        else if (status.compare("Out Of Stock", Qt::CaseInsensitive) == 0)
            outCount = count;
    }

    ui->lblTotalValue->setText(QString::number(totalCount));
    ui->lblLowStockValue->setText(QString::number(lowCount));
    ui->lblHighStockValue->setText(QString::number(highCount));
    ui->lblOutOfStockValue->setText(QString::number(outCount));
}

// Loads only the TABLE rows, respecting currentFilter and pagination.
// Does NOT touch the stat card numbers anymore.
void inventory::loadInventoryData(int page)
{
    currentPage = page;
    int offset = (currentPage - 1) * pageSize;

    QSqlQuery query;
    QString sql = "SELECT id, product_name, category, unit, price, stock, "
                  "expiry_date, status, supplier, sku FROM products ";

    if (!currentFilter.isEmpty())
        sql += "WHERE LOWER(status) = LOWER(:status) ";

    sql += "ORDER BY id LIMIT :limit OFFSET :offset";

    query.prepare(sql);

    if (!currentFilter.isEmpty())
        query.bindValue(":status", currentFilter);

    query.bindValue(":limit", pageSize);
    query.bindValue(":offset", offset);

    if (!query.exec()) {
        qWarning() << "Inventory: failed to load products -" << query.lastError().text();
        return;
    }

    ui->inventoryTable->setRowCount(0);
    int row = 0;

    while (query.next()) {
        ui->inventoryTable->insertRow(row);

        for (int col = 0; col < 10; ++col) {
            auto *item = new QTableWidgetItem();

            // Store Stock (col 5) as a real number so sorting works correctly
            if (col == 5) {
                item->setData(Qt::DisplayRole, query.value(col).toInt());
            } else {
                item->setText(query.value(col).toString());
            }

            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->inventoryTable->setItem(row, col, item);
        }

        ++row;
    }
}

void inventory::filterByStatus(const QString &status)
{
    currentFilter = status;
    loadInventoryData(1); // reset to page 1 whenever the filter changes
    // Note: updateStatCards() is NOT called here on purpose -
    // stat card numbers must stay fixed regardless of filter.
}

bool inventory::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == ui->cardLowStock) {
            filterByStatus("Low Stock");
            return true;
        } else if (watched == ui->cardHighStock) {
            filterByStatus("High Stock");
            return true;
        } else if (watched == ui->cardOutOfStock) {
            filterByStatus("Out Of Stock");
            return true;
        } else if (watched == ui->cardTotal) {
            filterByStatus(""); // clicking "Total Products" clears the filter
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
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

void inventory::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Every time this page becomes visible again (e.g. switching back
    // from the Product page after adding/editing/deleting a product),
    // refresh both the stat cards and the table so nothing is stale.
    updateStatCards();
    loadInventoryData(currentPage);
}

void inventory::goToNextPage()
{
    loadInventoryData(currentPage + 1);
}

void inventory::goToPreviousPage()
{
    if (currentPage > 1)
        loadInventoryData(currentPage - 1);
}
