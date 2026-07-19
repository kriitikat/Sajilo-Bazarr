#ifndef INVENTORY_H
#define INVENTORY_H

#include <QMainWindow>
#include <QEvent>
#include "../include/backbase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class inventory;
}
QT_END_NAMESPACE

// Inherits BackBase<QMainWindow> instead of QMainWindow directly — this
// is the only change needed to pick up wireBackButton()/goBackToDashboard()
// (see backbase.h for why it's a template). Everything else about this
// class is exactly as before.
class inventory : public BackBase<QMainWindow>
{
    Q_OBJECT

public:
    explicit inventory(QWidget *parent = nullptr);
    ~inventory() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;   // ADD THIS LINE
private:
    Ui::inventory *ui;

    int currentPage = 1;
    int pageSize = 15;
    QString currentFilter; // empty = show all; else "Low Stock" / "High Stock" / "Out Of Stock"

    void loadInventoryData(int page = 1);
    void updateStatCards();          // NEW: always counts ALL products, unaffected by filter
    void filterByStatus(const QString &status);

private slots:
    void handleSortChanged(int index);
    void goToNextPage();
    void goToPreviousPage();
};

#endif // INVENTORY_H
