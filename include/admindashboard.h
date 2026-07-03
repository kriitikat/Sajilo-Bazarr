#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include <QMainWindow>
#include "category.h"
#include "product.h"
#include "inventory.h"
#include "staff.h"
#include "pending.h"
#include "login.h"
#include"supplier.h"
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
    void on_btnCategories_clicked();
    void on_btnProducts_clicked();
    void on_btnInventory_clicked();
    void on_btnStaff_clicked();
    void on_btnSuppliers_clicked();
    void on_btnPending_Request_clicked(); // matches the UI button name "btnPending Request"
    void on_btnLogout_clicked();

private:
    Ui::AdminDashboard *ui;
};

#endif // ADMINDASHBOARD_H