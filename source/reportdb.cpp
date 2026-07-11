#include "../include/reportdb.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace {

// Estimated cost as a fraction of selling price, used only to seed
// buying_price for rows that don't have a real one entered yet.
// This is a fallback default — ideally every product should have a
// real buying_price entered manually when added/edited.
constexpr double kEstimatedCostRatio = 0.70;

// Adds buying_price to 'products' if it doesn't already exist.
// ALTER TABLE ADD COLUMN in SQLite never touches existing rows/data —
// it just adds a new column (defaulted to 0) to every row.
void ensureBuyingPriceColumn()
{
    QSqlQuery pragma("PRAGMA table_info(products)");
    bool hasColumn = false;
    while (pragma.next()) {
        if (pragma.value(1).toString() == "buying_price") {
            hasColumn = true;
            break;
        }
    }
    if (!hasColumn) {
        QSqlQuery alter;
        if (!alter.exec("ALTER TABLE products ADD COLUMN buying_price REAL DEFAULT 0"))
            qWarning() << "ensureBuyingPriceColumn: ALTER TABLE failed -"
                       << alter.lastError().text();
    }

    // Seed an estimated cost for any row still at 0 (new column or new product).
    QSqlQuery seed;
    seed.prepare("UPDATE products SET buying_price = ROUND(price * :ratio, 2) "
                 "WHERE buying_price = 0 OR buying_price IS NULL");
    seed.bindValue(":ratio", kEstimatedCostRatio);
    if (!seed.exec())
        qWarning() << "ensureBuyingPriceColumn: seed failed -" << seed.lastError().text();
}

} // namespace

void initReportTables()
{
    QSqlQuery createSales;
    createSales.exec(R"sql(
        CREATE TABLE IF NOT EXISTS sales (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id    INTEGER NOT NULL,
            sku           TEXT,
            product_name  TEXT,
            quantity_sold REAL NOT NULL,
            unit_price    REAL NOT NULL,
            sale_date     TEXT NOT NULL
        );
    )sql");
    if (createSales.lastError().isValid())
        qWarning() << "initReportTables: create sales failed -"
                   << createSales.lastError().text();

    ensureBuyingPriceColumn();
}