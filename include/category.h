#ifndef CATEGORY_H
#define CATEGORY_H

#include <QMainWindow>

namespace Ui {
class category;
}

class category : public QMainWindow
{
    Q_OBJECT

public:
    explicit category(QWidget *parent = nullptr);
    ~category();

private:
    Ui::category *ui;
};

#endif