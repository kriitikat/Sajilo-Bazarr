// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Product Module (Admin: view + update stock + delete)
//  product.cpp
//  Reads bazar1.db → products table
//  Columns: id, product_name, category, unit, price, stock,
//           expiry_date, status, supplier, sku
// ═══════════════════════════════════════════════════════════════════

#include "../include/product.h"
#include "../ui/ui_product.h"

#include <QHBoxLayout>
#include <QDateTime>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QColor>
#include <QFont>
#include <QInputDialog>

// ── Static data ───────────────────────────────────────────────────

// Reads category names live from the "categories" table so that any
// category added/renamed/deleted on the Category page is immediately
// reflected here the next time this page is opened.
QStringList Product::loadCategoriesFromDb()
{
    QStringList categories;

    QSqlQuery q;
    if (!q.exec("SELECT category_name FROM categories "
                "WHERE category_name IS NOT NULL AND TRIM(category_name) <> '' "
                "ORDER BY category_name ASC")) {
        qWarning() << "Product: failed to load categories from DB -"
                   << q.lastError().text();
        return { "Other" };
    }

    while (q.next())
        categories << q.value(0).toString();

    if (categories.isEmpty())
        categories << "Other";

    return categories;
}

// ═══════════════════════════════════════════════════════════════════
//  STATIC HELPERS  (file-scope)
// ═══════════════════════════════════════════════════════════════════
static ProductDTO fetchById(int id)
{
    ProductDTO p;
    QSqlQuery q;
    q.prepare(
        "SELECT id, product_name, category, unit, price, stock, "
        "       expiry_date, status, supplier, sku "
        "FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (q.exec() && q.next()) {
        p.id          = q.value(0).toInt();
        p.productName = q.value(1).toString();
        p.category    = q.value(2).toString();
        p.unit        = q.value(3).toString();
        p.price       = q.value(4).toDouble();
        p.stock       = q.value(5).toInt();
        p.expiryDate  = q.value(6).toString();
        p.status      = q.value(7).toString();
        p.supplier    = q.value(8).toString();
        p.sku         = q.value(9).toString();
    }
    return p;
}

// ═══════════════════════════════════════════════════════════════════
//  PRODUCT WIDGET  –  Main Screen (Admin: view + update stock + delete)
// ═══════════════════════════════════════════════════════════════════
Product::Product(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Product)
{
    ui->setupUi(this);

    // ── Table setup ──────────────────────────────────────────────
    // Columns: 0=Id, 1=Name, 2=Category, 3=Unit, 4=Price,
    //          5=Stock, 6=Expiry, 7=Status, 8=Supplier, 9=SKU, 10=Action
    QTableWidget *tbl = ui->tblProducts;
    tbl->verticalHeader()->setVisible(false);
    tbl->setWordWrap(false);
    tbl->setColumnWidth(0,  50);   // Id
    tbl->setColumnWidth(1, 160);   // Name
    tbl->setColumnWidth(2, 110);   // Category
    tbl->setColumnWidth(3,  60);   // Unit
    tbl->setColumnWidth(4,  90);   // Price
    tbl->setColumnWidth(5,  60);   // Stock
    tbl->setColumnWidth(6,  95);   // Expiry
    tbl->setColumnWidth(7,  90);   // Status
    tbl->setColumnWidth(8, 120);   // Supplier
    tbl->setColumnWidth(9,  90);   // SKU
    tbl->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Stretch); // Action

    populateCategoryFilter();

    connect(ui->txtSearch,         &QLineEdit::textChanged,
            this,                  &Product::onSearchChanged);
    connect(ui->cmbFilterCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,                  &Product::onFilterCategoryChanged);
    connect(ui->btnClearSearch,    &QPushButton::clicked,
            this,                  &Product::onClearSearch);
    connect(ui->btnPrevPage,       &QPushButton::clicked,
            this,                  &Product::onPrevPage);
    connect(ui->btnNextPage,       &QPushButton::clicked,
            this,                  &Product::onNextPage);

    loadProducts();
}

Product::~Product()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  DATABASE OPERATIONS
// ─────────────────────────────────────────────────────────────────
bool Product::deleteProduct(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

bool Product::updateStock(int id, int newStock)
{
    QSqlQuery q;
    q.prepare("UPDATE products SET stock = :stock WHERE id = :id");
    q.bindValue(":stock", newStock);
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

QList<ProductDTO> Product::fetchProducts(const QString &search,
                                         const QString &category,
                                         int limit, int offset)
{
    QList<ProductDTO> list;

    QString sql =
        "SELECT id, product_name, category, unit, price, stock, "
        "       expiry_date, status, supplier, sku "
        "FROM products WHERE 1=1";

    if (!search.isEmpty())
        sql += " AND (product_name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";
    sql += " ORDER BY product_name ASC LIMIT :limit OFFSET :offset";

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty())
        q.bindValue(":search", "%" + search + "%");
    if (!category.isEmpty())
        q.bindValue(":category", category);
    q.bindValue(":limit", limit);
    q.bindValue(":offset", offset);

    if (!q.exec()) {
        qWarning() << "[DB] fetchProducts error:" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        ProductDTO p;
        p.id          = q.value(0).toInt();
        p.productName = q.value(1).toString();
        p.category    = q.value(2).toString();
        p.unit        = q.value(3).toString();
        p.price       = q.value(4).toDouble();
        p.stock       = q.value(5).toInt();
        p.expiryDate  = q.value(6).toString();
        p.status      = q.value(7).toString();
        p.supplier    = q.value(8).toString();
        p.sku         = q.value(9).toString();
        list.append(p);
    }
    return list;
}

int Product::countProducts(const QString &search, const QString &category)
{
    QString sql = "SELECT COUNT(*) FROM products WHERE 1=1";

    if (!search.isEmpty())
        sql += " AND (product_name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty())
        q.bindValue(":search", "%" + search + "%");
    if (!category.isEmpty())
        q.bindValue(":category", category);

    if (!q.exec() || !q.next()) {
        qWarning() << "[DB] countProducts error:" << q.lastError().text();
        return 0;
    }
    return q.value(0).toInt();
}

// ─────────────────────────────────────────────────────────────────
//  UI HELPERS
// ─────────────────────────────────────────────────────────────────
void Product::populateCategoryFilter()
{
    ui->cmbFilterCategory->blockSignals(true);
    ui->cmbFilterCategory->clear();
    ui->cmbFilterCategory->addItem("All Categories");
    for (const auto &c : loadCategoriesFromDb())
        ui->cmbFilterCategory->addItem(c);
    ui->cmbFilterCategory->blockSignals(false);
}

QString Product::currentCategoryFilter() const
{
    return ui->cmbFilterCategory->currentIndex() == 0
               ? QString()
               : ui->cmbFilterCategory->currentText();
}

void Product::loadProducts()
{
    const QString search   = ui->txtSearch->text().trimmed();
    const QString category = currentCategoryFilter();

    // Recompute total + clamp current page (handles deletes shrinking
    // the result set, or a fresh search/filter resetting things).
    m_totalCount = countProducts(search, category);
    const int maxPage = m_totalCount > 0 ? (m_totalCount - 1) / PAGE_SIZE : 0;
    if (m_currentPage > maxPage) m_currentPage = maxPage;
    if (m_currentPage < 0)       m_currentPage = 0;

    const int offset = m_currentPage * PAGE_SIZE;
    const QList<ProductDTO> products = fetchProducts(search, category, PAGE_SIZE, offset);

    QTableWidget *tbl = ui->tblProducts;
    tbl->setUpdatesEnabled(false);
    tbl->setSortingEnabled(false);
    tbl->setRowCount(0);

    for (int row = 0; row < products.size(); ++row) {
        tbl->insertRow(row);
        setRowData(row, products[row]);
        addActionButtons(row, products[row]);
    }

    tbl->setSortingEnabled(true);
    tbl->setUpdatesEnabled(true);

    updateStatusBar(products.size());
    updatePagerControls();
}

void Product::updatePagerControls()
{
    const int maxPage = m_totalCount > 0 ? (m_totalCount - 1) / PAGE_SIZE : 0;
    ui->btnPrevPage->setEnabled(m_currentPage > 0);
    ui->btnNextPage->setEnabled(m_currentPage < maxPage);
    ui->lblPageInfo->setText(
        QString("Page %1 of %2").arg(m_currentPage + 1).arg(maxPage + 1));
}

void Product::setRowData(int row, const ProductDTO &p)
{
    QTableWidget *tbl = ui->tblProducts;

    // Col 0 – Id  (store real DB id as UserRole for later retrieval)
    auto *itemId = new QTableWidgetItem(QString::number(p.id));
    itemId->setData(Qt::UserRole, p.id);
    itemId->setTextAlignment(Qt::AlignCenter);

    // Col 1 – Product Name
    auto *itemName = new QTableWidgetItem(p.productName);

    // Col 2 – Category
    auto *itemCat = new QTableWidgetItem(p.category);

    // Col 3 – Unit
    auto *itemUnit = new QTableWidgetItem(p.unit);
    itemUnit->setTextAlignment(Qt::AlignCenter);

    // Col 4 – Price
    auto *itemPrice = new QTableWidgetItem(
        QString("Rs. %1").arg(p.price, 0, 'f', 2));
    itemPrice->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Col 5 – Stock  (red + bold when low)
    auto *itemStock = new QTableWidgetItem(QString::number(p.stock));
    itemStock->setTextAlignment(Qt::AlignCenter);
    if (p.stock <= 10) {
        itemStock->setForeground(QColor("#C0392B"));
        QFont f; f.setBold(true);
        itemStock->setFont(f);
    }

    // Col 6 – Expiry
    auto *itemExp = new QTableWidgetItem(
        p.expiryDate.isEmpty() ? "—" : p.expiryDate);
    itemExp->setTextAlignment(Qt::AlignCenter);

    // Col 7 – Status  (colour-coded)
    auto *itemStatus = new QTableWidgetItem(p.status);
    itemStatus->setTextAlignment(Qt::AlignCenter);
    if (p.status == "In Stock" || p.status == "High Stock") {
        itemStatus->setForeground(QColor("#1B8A44"));
        itemStatus->setBackground(QColor("#E6F4EA"));
    } else if (p.status == "Low Stock") {
        itemStatus->setForeground(QColor("#856404"));
        itemStatus->setBackground(QColor("#FFF3CD"));
    } else {
        itemStatus->setForeground(QColor("#C0392B"));
        itemStatus->setBackground(QColor("#FDEDEC"));
    }

    // Col 8 – Supplier
    auto *itemSupplier = new QTableWidgetItem(p.supplier);

    // Col 9 – SKU
    auto *itemSku = new QTableWidgetItem(p.sku);
    itemSku->setTextAlignment(Qt::AlignCenter);

    tbl->setItem(row, 0, itemId);
    tbl->setItem(row, 1, itemName);
    tbl->setItem(row, 2, itemCat);
    tbl->setItem(row, 3, itemUnit);
    tbl->setItem(row, 4, itemPrice);
    tbl->setItem(row, 5, itemStock);
    tbl->setItem(row, 6, itemExp);
    tbl->setItem(row, 7, itemStatus);
    tbl->setItem(row, 8, itemSupplier);
    tbl->setItem(row, 9, itemSku);
    // Col 10 = action buttons (set by addActionButtons)
}

void Product::addActionButtons(int row, const ProductDTO &p)
{
    auto *cell   = new QWidget;
    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    auto *btnStock = new QPushButton("📦 Update Stock");
    btnStock->setProperty("productId", p.id);
    btnStock->setStyleSheet(
        "QPushButton{background:#EAF7EF;color:#1E8E3E;border:1px solid #A9DFBF;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#D5F2E0;}");
    connect(btnStock, &QPushButton::clicked, this, &Product::onUpdateStock);

    auto *btnDel = new QPushButton("🗑 Delete");
    btnDel->setProperty("productId", p.id);
    btnDel->setStyleSheet(
        "QPushButton{background:#FDEDEC;color:#C0392B;border:1px solid #F5B7B1;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#FADBD8;}");
    connect(btnDel, &QPushButton::clicked, this, &Product::onDeleteProduct);

    layout->addStretch();
    layout->addWidget(btnStock);
    layout->addWidget(btnDel);
    layout->addStretch();

    ui->tblProducts->setCellWidget(row, 10, cell);
    ui->tblProducts->setRowHeight(row, 42);
}

void Product::updateStatusBar(int shownCount)
{
    ui->lblTotalProducts->setText(
        QString("Total: %1 product%2").arg(m_totalCount).arg(m_totalCount == 1 ? "" : "s"));
    ui->lblStatusBar->setText(
        QString("Showing %1 of %2  ·  Last refreshed: %3")
            .arg(shownCount)
            .arg(m_totalCount)
            .arg(QDateTime::currentDateTime().toString("dd-MMM-yyyy  hh:mm:ss")));
}

// ─────────────────────────────────────────────────────────────────
//  SLOTS
// ─────────────────────────────────────────────────────────────────
void Product::onSearchChanged(const QString & /*text*/)
{
    m_currentPage = 0;   // any new search restarts at page 1
    loadProducts();
}

void Product::onFilterCategoryChanged(int /*index*/)
{
    m_currentPage = 0;   // any new filter restarts at page 1
    loadProducts();
}

void Product::onClearSearch()
{
    ui->txtSearch->clear();
    ui->cmbFilterCategory->setCurrentIndex(0);
    m_currentPage = 0;
    loadProducts();
}

void Product::onNextPage()
{
    ++m_currentPage;
    loadProducts();
}

void Product::onPrevPage()
{
    if (m_currentPage > 0) {
        --m_currentPage;
        loadProducts();
    }
}

void Product::onDeleteProduct()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    ProductDTO p = fetchById(id);

    auto ans = QMessageBox::question(
        this, "Confirm Delete",
        QString("Delete <b>%1</b> (SKU: %2)?<br>"
                "<span style='color:red;'>This action cannot be undone.</span>")
            .arg(p.productName, p.sku),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ans == QMessageBox::Yes && deleteProduct(id)) {
        ui->lblStatusBar->setText(
            QString("🗑  '%1' deleted.").arg(p.productName));
        loadProducts();   // loadProducts() clamps the page if this was the last row on the page
    }
}

void Product::onUpdateStock()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    ProductDTO p = fetchById(id);

    bool ok = false;
    int newStock = QInputDialog::getInt(
        this, "Update Stock",
        QString("New stock quantity for \"%1\" (SKU: %2):").arg(p.productName, p.sku),
        p.stock,   // current value
        0,         // min
        1000000,   // max
        1,         // step
        &ok);

    if (ok && newStock != p.stock && updateStock(id, newStock)) {
        ui->lblStatusBar->setText(
            QString("📦  Stock for '%1' updated to %2.").arg(p.productName).arg(newStock));
        loadProducts();
    }
}
