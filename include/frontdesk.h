#ifndef FRONTDESK_H
#define FRONTDESK_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class frontdesk;
}
QT_END_NAMESPACE

class frontdesk : public QMainWindow
{
    Q_OBJECT

public:
    explicit frontdesk(QWidget *parent = nullptr);
    ~frontdesk();

private slots:
    void openBillingWindow();

private:
    Ui::frontdesk *ui;
};

#endif // FRONTDESK_H