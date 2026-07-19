// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Front Desk Product View (view-only)
//  frontproduct.cpp
//  Reads bazar.db → products table.
//  All logic (fetch, table rendering, search, filter, pagination)
//  lives in ProductBase — this file only wires up its own Designer
//  widgets. There is no Action column and no add/edit/delete/stock
//  update capability: front desk staff can look, not touch.
// ═══════════════════════════════════════════════════════════════════

#include "../include/frontproduct.h"
#include "../ui/ui_frontproduct.h"

#include <QTableWidget>

FrontProduct::FrontProduct(QWidget *parent)
    : ProductBase(parent)
    , ui(new Ui::FrontProduct)
{
    ui->setupUi(this);

    // Common wiring (table columns, search/filter/pagination signals,
    // initial load) happens here, once this object's own widgets exist.
    initializeCommonUi();
}

FrontProduct::~FrontProduct()
{
    delete ui;
}

// ── ProductBase widget accessors ───────────────────────────────────
QTableWidget* FrontProduct::tableWidget()    const { return ui->tblProducts; }
QLineEdit*    FrontProduct::searchBox()      const { return ui->txtSearch; }
QComboBox*    FrontProduct::categoryFilter() const { return ui->cmbFilterCategory; }
QPushButton*  FrontProduct::clearButton()    const { return ui->btnClearSearch; }
QPushButton*  FrontProduct::prevPageButton() const { return ui->btnPrevPage; }
QPushButton*  FrontProduct::nextPageButton() const { return ui->btnNextPage; }
QLabel*       FrontProduct::pageInfoLabel()  const { return ui->lblPageInfo; }
QLabel*       FrontProduct::statusBarLabel() const { return ui->lblStatusBar; }
QLabel*       FrontProduct::totalLabel()     const { return ui->lblTotalProducts; }

// ── No row actions on this page: view-only, so there is nothing to
//    wire up here. There is no 11th "Action" column in frontproduct.ui
//    at all, so this override is just a required, empty implementation
//    of ProductBase's pure-virtual hook.
void FrontProduct::addActionButtons(int /*row*/, const ProductRecord & /*p*/)
{
    // Intentionally empty.
}