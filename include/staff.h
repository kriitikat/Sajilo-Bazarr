#ifndef STAFF_H
#define STAFF_H

#include <QWidget>

namespace Ui {
class staff;
}

class staff : public QWidget
{
    Q_OBJECT

public:
    explicit staff(QWidget *parent = nullptr);
    ~staff();

private:
    Ui::staff *ui;
};

#endif // STAFF_H
