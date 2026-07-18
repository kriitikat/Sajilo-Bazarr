#ifndef FRONTDESK_H
#define FRONTDESK_H

#include <QMainWindow>
#include "../include/classlogout.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class frontdesk;
}
QT_END_NAMESPACE

class QFrame;
class QLabel;

class frontdesk : public ClassLogout
{
    Q_OBJECT

public:
    explicit frontdesk(QWidget *parent = nullptr);
    ~frontdesk();

private slots:
    void openBillingWindow();

private:
    Ui::frontdesk *ui;

    // Dashboard population
    void loadDashboardStats();
    void setupProductTable();
    void loadRecentSales();
    QFrame *createStatCard(QWidget *parent, int x, int y, int w, int h,
                           const QString &title, const QString &value,
                           const QString &accentColor);

    // Keep pointers so we can refresh the values later without rebuilding cards
    QLabel *lblItemsSoldValue   = nullptr;
    QLabel *lblSalesTotalValue  = nullptr;
    QLabel *lblProductsSoldValue = nullptr;
    QLabel *lblLowStockValue   = nullptr;

    // Low stock threshold - adjust as needed, or wire this to a real
    // reorder-level column later if you add one to the products table.
    static const int LOW_STOCK_THRESHOLD = 10;
};

#endif // FRONTDESK_H
