#ifndef INVENTORY_H
#define INVENTORY_H

#include <QMainWindow>
#include <QHeaderView>
#include <QTableWidgetItem>

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
    Ui::inventory *ui;
};

#endif // INVENTORY_H