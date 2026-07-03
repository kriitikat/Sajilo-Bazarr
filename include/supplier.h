#ifndef SUPPLIER_H
#define SUPPLIER_H

#include <QWidget>

namespace Ui {
class supplier;
}

class supplier : public QWidget
{
    Q_OBJECT

public:
    explicit supplier(QWidget *parent = nullptr);
    ~supplier();

private slots:
    // Loads / reloads all rows from the suppliers table.
    void loadSupplierTable();

    // Filters the visible rows by the search text.
    void onSearchTextChanged(const QString &text);

    // "+ Add Supplier" button handler — opens an inline dialog to insert a new row.
    void on_addSupplierBtn_clicked();

    // Per-row action buttons dispatched via lambda / connect with the supplier id baked in.
    void editSupplier(int supplierId);
    void deleteSupplier(int supplierId);

private:
    Ui::supplier *ui;

    // Builds the ACTIONS cell widget (Edit / Delete buttons) for a given row.
    QWidget *buildActionsWidget(int supplierId);
};

#endif // SUPPLIER_H