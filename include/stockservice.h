#ifndef STOCKSERVICE_H
#define STOCKSERVICE_H

#include <QSqlQuery>

// Holds computed inventory stats. Nothing but plain data.
struct InventoryStats {
    int total = 0;
    int low   = 0;
    int high  = 0;
    int out   = 0;
};

class StockService {
public:
    static constexpr int LOW_STOCK_THRESHOLD  = 10;
    static constexpr int HIGH_STOCK_THRESHOLD = 50;

    // Every page (admin, staff, anywhere else) calls this ONE function.
    // Always live, always the same rule, everywhere.
    static InventoryStats computeStats();
    static QString statusForStock(int stock);
};

#endif // STOCKSERVICE_H