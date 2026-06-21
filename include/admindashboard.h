#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include <QMainWindow>
#include <QVBoxLayout>

class Product;
class inventory;

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
    ~AdminDashboard();

private slots:
    void on_btnDashboard_clicked();
    void on_btnProducts_clicked();
    void on_btnInventory_clicked();

private:
    Ui::AdminDashboard *ui;
    Product *productPage;
    inventory *inventoryPage;
};

#endif // ADMINDASHBOARD_H