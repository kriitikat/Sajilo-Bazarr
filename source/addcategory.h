#ifndef ADDCATEGORY_H
#define ADDCATEGORY_H

#include <QDialog>
#include <QString>

namespace Ui {
class addcategory;
}

class addcategory : public QDialog
{
    Q_OBJECT

public:
    explicit addcategory(QWidget *parent = nullptr);
    ~addcategory();

    // Data getter methods available to the category dashboard widget.
    // Both are trimmed of leading/trailing whitespace.
    QString getCategoryName() const;
    QString getDescription() const;

private slots:
    void on_addButton_clicked();
    void on_cancelButton_clicked();
    void updateAddButtonState();

private:
    Ui::addcategory *ui;
};

#endif // ADDCATEGORY_H