#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include <QMainWindow>

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

private:
    Ui::AdminDashboard *ui;
};
#endif // ADMINDASHBOARD_H
