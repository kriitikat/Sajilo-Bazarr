#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include <QMainWindow>
#include "../include/category.h"
#include "../include/product.h"
#include "../include/inventory.h"
#include "../include/staff.h"
#include "../include/pending.h"
#include "../include/login.h"
#include "../include/supplier.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class AdminDashboard;
}
QT_END_NAMESPACE

class AdminDashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminDashboard(QWidget *parent = nullptr);
    ~AdminDashboard() override;

private slots:
    void handleCategories_clicked();
    void handleProducts_clicked();
    void handleInventory_clicked();
    void handleStaff_clicked();
    void handleSuppliers_clicked();
    void handlePending_Request_clicked(); // matches the UI button name "btnPending Request"
    void handleLogout_clicked();

private:
    Ui::AdminDashboard *ui;
    inventory *inventoryPage = nullptr;
};

#endif // ADMINDASHBOARD_H