#ifndef SUPPLIER_H
#define SUPPLIER_H

#include <QMainWindow

namespace Ui { class Supplier; }


class Supplier : public QMainWindow
{
    Q_OBJECT

public:
    explicit Supplier(QWidget *parent = nullptr);
    ~Supplier();

private:
    Ui::Supplier *ui;
};

#endif //SUPPLIER_H