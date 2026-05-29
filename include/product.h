#ifndef PRODUCT_H
#define PRODUCT_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class product;
}
QT_END_NAMESPACE

class product : public QMainWindow
{
    Q_OBJECT

public:
    explicit product(QWidget *parent = nullptr);
    ~product() override;

private:
    Ui::product *ui;
};
#endif // PRODUCT_H
