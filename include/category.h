#ifndef CATEGORY_H
#define CATEGORY_H

#include <QWidget>

namespace Ui {
class category;
}

class category : public QWidget
{
    Q_OBJECT

public:
    explicit category(QWidget *parent = nullptr);
    ~category();

private slots:
    void on_addCategoryBtn_clicked(); // Slot for the dashboard button click

private:
    Ui::category *ui;
};

#endif // CATEGORY_H