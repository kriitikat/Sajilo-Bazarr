#ifndef PRODUCTBASE_H
#define PRODUCTBASE_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QList>

class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

// ═══════════════════════════════════════════════════════════════════
//  ProductRecord  —  ENCAPSULATION
//  Single shared DTO for the "products" table. All fields are
//  private; the outside world only ever touches them through the
//  getters/setters below. Both Product and ProductStaff (and the
//  add/edit dialog) work with this exact same class, so there is
//  only one definition of "what a product is" in the whole program.
// ═══════════════════════════════════════════════════════════════════
class ProductRecord
{
public:
    ProductRecord() = default;

    int     id()        const { return m_id; }
    QString name()       const { return m_name; }
    QString category()   const { return m_category; }
    QString unit()       const { return m_unit; }
    double  price()      const { return m_price; }
    int     stock()      const { return m_stock; }
    QString expiryDate() const { return m_expiryDate; }
    QString status()     const { return m_status; }
    QString supplier()   const { return m_supplier; }
    QString sku()        const { return m_sku; }

    void setId(int v)                  { m_id = v; }
    void setName(const QString &v)     { m_name = v; }
    void setCategory(const QString &v) { m_category = v; }
    void setUnit(const QString &v)     { m_unit = v; }
    void setPrice(double v)            { m_price = v; }
    void setStock(int v)               { m_stock = v; }
    void setExpiryDate(const QString &v) { m_expiryDate = v; }
    void setStatus(const QString &v)   { m_status = v; }
    void setSupplier(const QString &v) { m_supplier = v; }
    void setSku(const QString &v)      { m_sku = v; }

    // Days remaining until expiry (negative = already expired).
    // Returns INT_MIN when there is no usable expiry date.
    int daysUntilExpiry() const;

private:
    int     m_id = 0;
    QString m_name;
    QString m_category;
    QString m_unit;
    double  m_price = 0.0;
    int     m_stock = 0;
    QString m_expiryDate;   // "yyyy-MM-dd"
    QString m_status;
    QString m_supplier;
    QString m_sku;
};

// ═══════════════════════════════════════════════════════════════════
//  ProductBase  —  INHERITANCE
//  Abstract base widget holding everything the Product (admin) page
//  and the ProductStaff page have in common: reading from the DB,
//  drawing the table, search box, category filter and pagination.
//
//  A concrete subclass must:
//    1. "plug in" its own Designer widgets by implementing the
//       protected pure-virtual accessors below (tableWidget(),
//       searchBox(), ... ) so this class can drive them.
//    2. implement addActionButtons(), since Product offers
//       "Update Stock" while ProductStaff offers "Edit".
//    3. call initializeCommonUi() at the END of its own constructor
//       (i.e. after ui->setupUi(this) has created its widgets).
//       This two-step construction is required because virtual
//       functions cannot be safely dispatched to a derived class
//       from inside the base class's own constructor.
//
//  Everything that is specific to the expiry-warning system
//  (colours, the "Expiring Soon" checkbox, the warning window) is
//  expressed as virtual hooks with harmless no-op defaults here,
//  and is overridden ONLY by ProductStaff — Product simply never
//  turns it on.
// ═══════════════════════════════════════════════════════════════════
class ProductBase : public QWidget
{
    Q_OBJECT
public:
    explicit ProductBase(QWidget *parent = nullptr);
    ~ProductBase() override = default;

    // Reads distinct category names live from the "categories" table so
    // any change made on the Category page is picked up immediately.
    static QStringList loadCategoriesFromDb();

protected:
    // ── Widgets the subclass must expose (bound to its own .ui) ───────
    virtual QTableWidget* tableWidget()    const = 0;
    virtual QLineEdit*    searchBox()      const = 0;
    virtual QComboBox*    categoryFilter() const = 0;
    virtual QPushButton*  clearButton()    const = 0;
    virtual QPushButton*  prevPageButton() const = 0;
    virtual QPushButton*  nextPageButton() const = 0;
    virtual QLabel*       pageInfoLabel()  const = 0;
    virtual QLabel*       statusBarLabel() const = 0;
    virtual QLabel*       totalLabel()     const = 0;

    // ── Row-action buttons differ per page → each subclass builds them ──
    virtual void addActionButtons(int row, const ProductRecord &p) = 0;

    // ── Expiry-warning system: OFF by default (used only by Product) ──
    // ProductStaff overrides all four of these to turn the system on.
    virtual bool    expiringSoonFilterActive() const { return false; }
    virtual int     expiryWarningWindowDays()  const { return 5; }
    virtual QString formatExpiryText(const ProductRecord &p, int daysLeft) const;
    virtual void    decorateExpiryCell(QTableWidgetItem *item, int daysLeft) const;

    // ── Optional extension point for subclass-only widgets/signals ────
    // (e.g. ProductStaff's "Add Product" button, the expiry checkbox)
    virtual void setupExtraUi() {}
    virtual void connectExtraSignals() {}

    // ── Call once, at the end of the subclass constructor ─────────────
    void initializeCommonUi();

    // ── Shared building blocks, reusable by subclasses ─────────────────
    void    loadProducts();
    void    populateCategoryFilter();
    QString currentCategoryFilter() const;

    QList<ProductRecord> fetchProducts(const QString &search, const QString &category,
                                        int limit, int offset,
                                        bool expiringSoonOnly) const;
    int  countProducts(const QString &search, const QString &category,
                        bool expiringSoonOnly) const;
    bool deleteProductFromDb(int id) const;
    static ProductRecord fetchById(int id);

    void setRowData(int row, const ProductRecord &p);
    void updateStatusBar(int shownCount);
    void updatePagerControls();

    // Pagination state — protected (encapsulated: not exposed publicly),
    // shared by both subclasses so paging behaves identically everywhere.
    static const int PAGE_SIZE = 10;
    int m_currentPage = 0;   // zero-indexed
    int m_totalCount  = 0;

protected slots:
    void onSearchChanged(const QString &text);
    void onFilterCategoryChanged(int index);
    void onClearSearch();
    void onNextPage();
    void onPrevPage();
    void onDeleteProduct();
};

#endif // PRODUCTBASE_H
