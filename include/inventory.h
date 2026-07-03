#ifndef INVENTORY_H
#define INVENTORY_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class inventory;
}
QT_END_NAMESPACE

class inventory : public QMainWindow
{
    Q_OBJECT

public:
    explicit inventory(QWidget *parent = nullptr);
    ~inventory() override;

private:
    void loadInventoryData();
    void handleSortChanged(int index);

    Ui::inventory *ui;
};

#endif // INVENTORY_H