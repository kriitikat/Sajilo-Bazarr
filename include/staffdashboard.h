#ifndef STAFFDASHBOARD_H
#define STAFFDASHBOARD_H

#include <QMainWindow>
#include "../include/productstaff.h"
#include "../include/classlogout.h"

class QLabel;
class QTableWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class StaffDashboard;
}
QT_END_NAMESPACE

class StaffDashboard : public ClassLogout
{
    Q_OBJECT
public:
    explicit StaffDashboard(QWidget *parent = nullptr);
    ~StaffDashboard() override;

private slots:
    void handleProductsClicked();   // renamed to avoid Qt auto-connect double-firing
    void refreshDashboardStats();   // re-pulls counts + low stock rows from DB

private:
    void setupDashboardContent();   // builds the cards + table once, in the ctor

    Ui::StaffDashboard *ui;

    // Stat cards
    QLabel *lblTotalProducts   = nullptr;
    QLabel *lblLowStockCount   = nullptr;
    QLabel *lblOutOfStockCount = nullptr;
    QLabel *lblPendingTasks    = nullptr; // TODO: wire to taskmanagement.h later

    // Low stock table
    QTableWidget *tblLowStock  = nullptr;

    static constexpr int LOW_STOCK_THRESHOLD = 10;
};

#endif // STAFFDASHBOARD_H