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

signals:
    // Emitted when the "Back to Dashboard" button is clicked. Whatever
    // owns this page (e.g. AdminDashboard's QStackedWidget) should connect
    // to this and switch back to the dashboard page - this class does not
    // know about AdminDashboard directly, keeping it reusable/decoupled.
    void backToDashboardRequested();

private slots:
    void on_addCategoryBtn_clicked();          // Slot for the dashboard button click
    void loadCategoriesTable();                // Fetches all rows from the categories table
    void editCategory();                       // Edit handler — resolves the row via sender()
    void deleteCategory();                     // Delete handler — resolves the row via sender()

private:
    Ui::category *ui;
};

#endif // CATEGORY_H