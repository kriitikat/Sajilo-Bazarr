#ifndef PRODUCT_H
#define PRODUCT_H

#include "productbase.h"

namespace Ui { class Product; }

// ─────────────────────────────────────────────
//  Product  —  Admin page (view + update stock + delete)
//  Inherits everything shared (fetch, table, search, filter,
//  pagination, delete) from ProductBase. Does NOT provide the
//  expiry-warning system or "Add Product" — those live on the
//  ProductStaff page.
// ─────────────────────────────────────────────
class Product : public ProductBase
{
    Q_OBJECT
public:
    explicit Product(QWidget *parent = nullptr);
    ~Product() override;

protected:
    // ── ProductBase widget accessors ───────────────────────────
    QTableWidget* tableWidget()    const override;
    QLineEdit*    searchBox()      const override;
    QComboBox*    categoryFilter() const override;
    QPushButton*  clearButton()    const override;
    QPushButton*  prevPageButton() const override;
    QPushButton*  nextPageButton() const override;
    QLabel*       pageInfoLabel()  const override;
    QLabel*       statusBarLabel() const override;
    QLabel*       totalLabel()     const override;

    void addActionButtons(int row, const ProductRecord &p) override;
    // expiringSoonFilterActive(), formatExpiryText(), decorateExpiryCell()
    // are left at ProductBase's plain defaults on purpose.

private slots:
    void onUpdateStock();

private:
    bool updateStock(int id, int newStock);

    Ui::Product *ui;
};
#endif // PRODUCT_H
