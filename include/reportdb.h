#ifndef REPORTDB_H
#define REPORTDB_H

// Holds the numbers shown in the Dashboard Overview pie chart.
struct ReportData {
    double revenue = 0.0;
    double profit  = 0.0;
    double loss    = 0.0;
};

// Creates the 'sales' table if it doesn't exist, and ensures the
// 'products' table has a buying_price column (seeded at 70% of the
// current selling price for any row missing it, as a fallback default
// until real costs are entered). Safe to call on every app startup —
// never touches existing product data otherwise.
void initReportTables();

#endif // REPORTDB_H