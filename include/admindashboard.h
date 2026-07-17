#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QVector>
#include <QPair>
#include <QString>

#include "../include/reportdb.h"   // ReportData
#include "../include/classlogout.h"
namespace Ui {
class AdminDashboard;
}

// Forward declarations — these pages are only ever used as pointers here,
// so we don't need their full headers in this file.
class category;
class Product;
class inventory;
class staff;
class pending;
class supplier;
class Login;

class AdminDashboard : public ClassLogout
{
    Q_OBJECT

public:
    explicit AdminDashboard(QWidget *parent = nullptr);
    ~AdminDashboard();

private slots:
    void handleCategories_clicked();
    void handleProducts_clicked();
    void handleInventory_clicked();
    void handleStaff_clicked();
    void handleSuppliers_clicked();
    void handlePending_Request_clicked();

private:
    Ui::AdminDashboard *ui;

    inventory *inventoryPage = nullptr;

    // ---- Reporting ----
    ReportData fetchReportData();
    QVector<QPair<QString, double>> fetchWeeklySales();
    QVector<QPair<QString, double>> fetchMonthlySales();

    void setupReportChart();
    void setupWeeklySalesChart();
    void setupMonthlySalesChart();

    QChartView *reportChartView  = nullptr;
    QChartView *weeklyChartView  = nullptr;
    QChartView *monthlyChartView = nullptr;
};

#endif // ADMINDASHBOARD_H