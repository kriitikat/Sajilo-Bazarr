// staffdashboard.h

#ifndef STAFFDASHBOARD_H
#define STAFFDASHBOARD_H

#include <QMainWindow>

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

private:
    Ui::StaffDashboard *ui;
};

#endif // STAFFDASHBOARD_H