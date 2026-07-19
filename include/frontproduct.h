#ifndef FRONTPRODUCT_H
#define FRONTPRODUCT_H

#include "productbase.h"

namespace Ui { class FrontProduct; }

// ─────────────────────────────────────────────
//  FrontProduct  —  Front Desk page (VIEW ONLY)
//  Inherits everything shared (fetch, table, search, filter,
//  pagination) from ProductBase — exact same inheritance /
//  encapsulation pattern used by Product and ProductStaff.
//
//  Unlike its siblings:
//    • No "Action" column at all (no Update Stock / Add / Edit /
//      Delete) — front desk staff can look products up, nothing
//      more. addActionButtons() is overridden as a deliberate no-op.
//    • No expiry-warning system — like Product, it just leaves
//      ProductBase's plain-date defaults in place (no override of
//      expiringSoonFilterActive()/formatExpiryText()/decorateExpiryCell()).
// ─────────────────────────────────────────────
class FrontProduct : public ProductBase
{
    Q_OBJECT
public:
    explicit FrontProduct(QWidget *parent = nullptr);
    ~FrontProduct() override;

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

    // No action column → intentionally empty implementation.
    void addActionButtons(int row, const ProductRecord &p) override;
    // expiringSoonFilterActive(), formatExpiryText(), decorateExpiryCell()
    // are left at ProductBase's plain defaults on purpose (same as Product).

private:
    Ui::FrontProduct *ui;
};
#endif // FRONTPRODUCT_H
