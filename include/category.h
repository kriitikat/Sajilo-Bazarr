#ifndef CATEGORY_H
#define CATEGORY_H

#include <QMainWindow>

namespace Ui {
class SUPPLIER;
}

class category : public QMainWindow
{
    Q_OBJECT

public:
    explicit category(QWidget *parent = nullptr);
    ~category();

private:
    Ui::SUPPLIER *ui;
};

#endif