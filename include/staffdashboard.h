#ifndef STAFFDASHBOARD_H
#define STAFFDASHBOARD_H

#include <QMainWindow>
#include "../include/productstaff.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class StaffDashboard;
}
QT_END_NAMESPACE

class StaffDashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit StaffDashboard(QWidget *parent = nullptr);
    ~StaffDashboard() override;

private slots:
    void handleProductsClicked();   // renamed to avoid Qt auto-connect double-firing

private:
    Ui::StaffDashboard *ui;
};

#endif // STAFFDASHBOARD_H