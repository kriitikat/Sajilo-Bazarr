// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Shared Product Base
//  productbase.cpp
//  Common logic for the Product (admin) and ProductStaff pages:
//  reading bazar.db → products table, drawing the table, search,
//  category filter, pagination, delete.
//  Columns: id, product_name, category, unit, price, stock,
//           expiry_date, status, supplier, sku
// ═══════════════════════════════════════════════════════════════════

#include "../include/productbase.h"
#include "../include/stockservice.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QDate>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QColor>
#include <QFont>
#include <climits>

// ═══════════════════════════════════════════════════════════════════
//  ProductRecord
// ═══════════════════════════════════════════════════════════════════
int ProductRecord::daysUntilExpiry() const
{
    if (m_expiryDate.isEmpty())
        return INT_MIN;

    QDate expiry = QDate::fromString(m_expiryDate, "yyyy-MM-dd");
    if (!expiry.isValid())
        return INT_MIN;

    return QDate::currentDate().daysTo(expiry);
}

// ═══════════════════════════════════════════════════════════════════
//  ProductBase
// ═══════════════════════════════════════════════════════════════════
ProductBase::ProductBase(QWidget *parent)
    : BackBase<QWidget>(parent)
{
    // Intentionally empty: subclasses build their own ui->setupUi(this)
    // first, then call initializeCommonUi() explicitly. See header.
    // (BackBase<QWidget> adds no constructor-time behavior of its own —
    // see backbase.h — so this stays exactly as simple as before.)
}

QStringList ProductBase::loadCategoriesFromDb()
{
    QStringList categories;

    QSqlQuery q;
    if (!q.exec("SELECT category_name FROM categories "
                "WHERE category_name IS NOT NULL AND TRIM(category_name) <> '' "
                "ORDER BY category_name ASC")) {
        qWarning() << "ProductBase: failed to load categories from DB -"
                   << q.lastError().text();
        return { "Other" };
    }

    while (q.next())
        categories << q.value(0).toString();

    if (categories.isEmpty())
        categories << "Other";

    return categories;
}

void ProductBase::initializeCommonUi()
{
    QTableWidget *tbl = tableWidget();
    tbl->verticalHeader()->setVisible(false);
    tbl->setWordWrap(false);
    tbl->setColumnWidth(0,  50);   // Id
    tbl->setColumnWidth(1, 150);   // Name
    tbl->setColumnWidth(2,  95);   // Category
    tbl->setColumnWidth(3,  60);   // Unit
    tbl->setColumnWidth(4,  90);   // Price
    tbl->setColumnWidth(5,  60);   // Stock
    tbl->setColumnWidth(6, 110);   // Expiry
    tbl->setColumnWidth(7,  90);   // Status
    tbl->setColumnWidth(8, 110);   // Supplier
    tbl->setColumnWidth(9,  90);   // SKU
    tbl->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Stretch); // Action

    populateCategoryFilter();

    connect(searchBox(),      &QLineEdit::textChanged,
            this,             &ProductBase::onSearchChanged);
    connect(categoryFilter(), QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,             &ProductBase::onFilterCategoryChanged);
    connect(clearButton(),    &QPushButton::clicked,
            this,             &ProductBase::onClearSearch);
    connect(prevPageButton(), &QPushButton::clicked,
            this,             &ProductBase::onPrevPage);
    connect(nextPageButton(), &QPushButton::clicked,
            this,             &ProductBase::onNextPage);

    // Hook for anything subclass-specific (expiry checkbox, Add button...)
    setupExtraUi();
    connectExtraSignals();

    loadProducts();
}

// ─────────────────────────────────────────────────────────────────
//  DB OPERATIONS
// ─────────────────────────────────────────────────────────────────
ProductRecord ProductBase::fetchById(int id)
{
    ProductRecord p;
    QSqlQuery q;
    q.prepare(
        "SELECT id, product_name, category, unit, price, stock, "
        "       expiry_date, status, supplier, sku "
        "FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (q.exec() && q.next()) {
        p.setId(q.value(0).toInt());
        p.setName(q.value(1).toString());
        p.setCategory(q.value(2).toString());
        p.setUnit(q.value(3).toString());
        p.setPrice(q.value(4).toDouble());
        p.setStock(q.value(5).toInt());
        p.setExpiryDate(q.value(6).toString());
        p.setStatus(q.value(7).toString());
        p.setSupplier(q.value(8).toString());
        p.setSku(q.value(9).toString());
    }
    return p;
}

bool ProductBase::deleteProductFromDb(int id) const
{
    QSqlQuery q;
    q.prepare("DELETE FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(const_cast<ProductBase*>(this), "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

QList<ProductRecord> ProductBase::fetchProducts(const QString &search, const QString &category,
                                                 int limit, int offset,
                                                 bool expiringSoonOnly) const
{
    QList<ProductRecord> list;

    QString sql =
        "SELECT id, product_name, category, unit, price, stock, "
        "       expiry_date, status, supplier, sku "
        "FROM products WHERE 1=1";

    if (!search.isEmpty())
        sql += " AND (product_name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";
    if (expiringSoonOnly)
        sql += " AND expiry_date IS NOT NULL AND TRIM(expiry_date) <> '' "
               " AND date(expiry_date) <= date('now', :windowDays)";
    sql += " ORDER BY product_name ASC";
    if (limit > 0)
        sql += " LIMIT :limit OFFSET :offset";

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty())
        q.bindValue(":search", "%" + search + "%");
    if (!category.isEmpty())
        q.bindValue(":category", category);
    if (expiringSoonOnly)
        q.bindValue(":windowDays", QString("+%1 days").arg(expiryWarningWindowDays()));
    if (limit > 0) {
        q.bindValue(":limit", limit);
        q.bindValue(":offset", offset);
    }

    if (!q.exec()) {
        qWarning() << "[DB] fetchProducts error:" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        ProductRecord p;
        p.setId(q.value(0).toInt());
        p.setName(q.value(1).toString());
        p.setCategory(q.value(2).toString());
        p.setUnit(q.value(3).toString());
        p.setPrice(q.value(4).toDouble());
        p.setStock(q.value(5).toInt());
        p.setExpiryDate(q.value(6).toString());
        p.setStatus(q.value(7).toString());
        p.setSupplier(q.value(8).toString());
        p.setSku(q.value(9).toString());
        list.append(p);
    }
    return list;
}

int ProductBase::countProducts(const QString &search, const QString &category,
                                bool expiringSoonOnly) const
{
    QString sql = "SELECT COUNT(*) FROM products WHERE 1=1";

    if (!search.isEmpty())
        sql += " AND (product_name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";
    if (expiringSoonOnly)
        sql += " AND expiry_date IS NOT NULL AND TRIM(expiry_date) <> '' "
               " AND date(expiry_date) <= date('now', :windowDays)";

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty())
        q.bindValue(":search", "%" + search + "%");
    if (!category.isEmpty())
        q.bindValue(":category", category);
    if (expiringSoonOnly)
        q.bindValue(":windowDays", QString("+%1 days").arg(expiryWarningWindowDays()));

    if (!q.exec() || !q.next()) {
        qWarning() << "[DB] countProducts error:" << q.lastError().text();
        return 0;
    }
    return q.value(0).toInt();
}

// ─────────────────────────────────────────────────────────────────
//  UI HELPERS
// ─────────────────────────────────────────────────────────────────
void ProductBase::populateCategoryFilter()
{
    QComboBox *cmb = categoryFilter();
    cmb->blockSignals(true);
    cmb->clear();
    cmb->addItem("All Categories");
    for (const auto &c : loadCategoriesFromDb())
        cmb->addItem(c);
    cmb->blockSignals(false);
}

QString ProductBase::currentCategoryFilter() const
{
    return categoryFilter()->currentIndex() == 0
               ? QString()
               : categoryFilter()->currentText();
}

QString ProductBase::formatExpiryText(const ProductRecord &p, int /*daysLeft*/) const
{
    // Default (Product/admin page): just show the raw date, no warnings.
    return p.expiryDate().isEmpty() ? QStringLiteral("—") : p.expiryDate();
}

void ProductBase::decorateExpiryCell(QTableWidgetItem * /*item*/, int /*daysLeft*/) const
{
    // Default: no colour-coding. Only ProductStaff turns this on.
}

void ProductBase::loadProducts()
{
    const QString search   = searchBox()->text().trimmed();
    const QString category = currentCategoryFilter();
    const bool expiringSoonOnly = expiringSoonFilterActive();

    m_totalCount = countProducts(search, category, expiringSoonOnly);
    const int maxPage = m_totalCount > 0 ? (m_totalCount - 1) / PAGE_SIZE : 0;
    if (m_currentPage > maxPage) m_currentPage = maxPage;
    if (m_currentPage < 0)       m_currentPage = 0;

    const int offset = m_currentPage * PAGE_SIZE;
    const QList<ProductRecord> products =
        fetchProducts(search, category, PAGE_SIZE, offset, expiringSoonOnly);

    QTableWidget *tbl = tableWidget();
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

void ProductBase::updatePagerControls()
{
    const int maxPage = m_totalCount > 0 ? (m_totalCount - 1) / PAGE_SIZE : 0;
    prevPageButton()->setEnabled(m_currentPage > 0);
    nextPageButton()->setEnabled(m_currentPage < maxPage);
    pageInfoLabel()->setText(
        QString("Page %1 of %2").arg(m_currentPage + 1).arg(maxPage + 1));
}

void ProductBase::setRowData(int row, const ProductRecord &p)
{
    QTableWidget *tbl = tableWidget();

    // Col 0 – Id (store real DB id as UserRole for later retrieval)
    auto *itemId = new QTableWidgetItem(QString::number(p.id()));
    itemId->setData(Qt::UserRole, p.id());
    itemId->setTextAlignment(Qt::AlignCenter);

    // Col 1 – Product Name
    auto *itemName = new QTableWidgetItem(p.name());

    // Col 2 – Category
    auto *itemCat = new QTableWidgetItem(p.category());

    // Col 3 – Unit
    auto *itemUnit = new QTableWidgetItem(p.unit());
    itemUnit->setTextAlignment(Qt::AlignCenter);

    // Col 4 – Price
    auto *itemPrice = new QTableWidgetItem(QString("Rs. %1").arg(p.price(), 0, 'f', 2));
    itemPrice->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Col 5 – Stock (red + bold when low)
    auto *itemStock = new QTableWidgetItem(QString::number(p.stock()));
    itemStock->setTextAlignment(Qt::AlignCenter);
    if (p.stock() <= 10) {
        itemStock->setForeground(QColor("#C0392B"));
        QFont f; f.setBold(true);
        itemStock->setFont(f);
    }

    // Col 6 – Expiry. Text formatting and colour-decoration are both
    // virtual hooks: Product (base defaults) shows a plain date,
    // ProductStaff overrides both to run the expiry-warning system.
    const int daysLeft = p.daysUntilExpiry();
    auto *itemExp = new QTableWidgetItem(formatExpiryText(p, daysLeft));
    itemExp->setTextAlignment(Qt::AlignCenter);
    itemExp->setToolTip(p.expiryDate().isEmpty() ? "No expiry set" : "Expiry: " + p.expiryDate());
    decorateExpiryCell(itemExp, daysLeft);

    // Col 7 – Status (computed LIVE from stock — never trust DB status column)
    QString liveStatus = StockService::statusForStock(p.stock());
    auto *itemStatus = new QTableWidgetItem(liveStatus);
    itemStatus->setTextAlignment(Qt::AlignCenter);
    if (liveStatus == "Normal" || liveStatus == "High Stock") {
        itemStatus->setForeground(QColor("#1B8A44"));
        itemStatus->setBackground(QColor("#E6F4EA"));
    } else if (liveStatus == "Low Stock") {
        itemStatus->setForeground(QColor("#856404"));
        itemStatus->setBackground(QColor("#FFF3CD"));
    } else {
        itemStatus->setForeground(QColor("#C0392B"));
        itemStatus->setBackground(QColor("#FDEDEC"));
    }
    // Col 8 – Supplier
    auto *itemSupplier = new QTableWidgetItem(p.supplier());

    // Col 9 – SKU
    auto *itemSku = new QTableWidgetItem(p.sku());
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
    // Col 10 = action buttons (set by subclass::addActionButtons)
}

void ProductBase::updateStatusBar(int shownCount)
{
    totalLabel()->setText(
        QString("Total: %1 product%2").arg(m_totalCount).arg(m_totalCount == 1 ? "" : "s"));
    statusBarLabel()->setText(
        QString("Showing %1 of %2  ·  Last refreshed: %3")
            .arg(shownCount)
            .arg(m_totalCount)
            .arg(QDateTime::currentDateTime().toString("dd-MMM-yyyy  hh:mm:ss")));
}

// ─────────────────────────────────────────────────────────────────
//  SHARED SLOTS
// ─────────────────────────────────────────────────────────────────
void ProductBase::onSearchChanged(const QString & /*text*/)
{
    m_currentPage = 0;
    loadProducts();
}

void ProductBase::onFilterCategoryChanged(int /*index*/)
{
    m_currentPage = 0;
    loadProducts();
}

void ProductBase::onClearSearch()
{
    searchBox()->clear();
    categoryFilter()->setCurrentIndex(0);
    m_currentPage = 0;
    loadProducts();
}

void ProductBase::onNextPage()
{
    ++m_currentPage;
    loadProducts();
}

void ProductBase::onPrevPage()
{
    if (m_currentPage > 0) {
        --m_currentPage;
        loadProducts();
    }
}

void ProductBase::onDeleteProduct()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    ProductRecord p = fetchById(id);

    auto ans = QMessageBox::question(
        this, "Confirm Delete",
        QString("Delete <b>%1</b> (SKU: %2)?<br>"
                "<span style='color:red;'>This action cannot be undone.</span>")
            .arg(p.name(), p.sku()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ans == QMessageBox::Yes && deleteProductFromDb(id)) {
        statusBarLabel()->setText(QString("🗑  '%1' deleted.").arg(p.name()));
        loadProducts();
    }
}
