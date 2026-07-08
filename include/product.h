#ifndef PRODUCT_H
#define PRODUCT_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QList>

// ─────────────────────────────────────────────
//  Data Transfer Object  (matches bazar1.db)
// ─────────────────────────────────────────────
struct ProductDTO {
    int     id          = 0;
    QString productName;   // product_name
    QString category;
    QString unit;
    double  price       = 0.0;
    int     stock       = 0;
    QString expiryDate;    // expiry_date  "YYYY-MM-DD"
    QString status;
    QString supplier;
    QString sku;
};

// ─────────────────────────────────────────────
//  Main Product Widget  (Admin: view + update stock + delete)
// ─────────────────────────────────────────────
namespace Ui { class Product; }

class Product : public QWidget
{
    Q_OBJECT

public:
    explicit Product(QWidget *parent = nullptr);
    ~Product() override;

    // Reads distinct category names live from the "categories" table.
    // Public + static so it can be reused wherever a category dropdown
    // needs to stay in sync with the Category page.
    static QStringList loadCategoriesFromDb();

private slots:
    void onSearchChanged(const QString &text);
    void onFilterCategoryChanged(int index);
    void onClearSearch();
    void onDeleteProduct();
    void onUpdateStock();
    void onNextPage();
    void onPrevPage();

private:
    // ── DB operations ──────────────────────────────────────────
    bool              deleteProduct(int id);
    bool              updateStock(int id, int newStock);
    QList<ProductDTO> fetchProducts(const QString &search, const QString &category,
                                    int limit, int offset);
    int               countProducts(const QString &search, const QString &category);

    // ── UI helpers ─────────────────────────────────────────────
    void    loadProducts();      // reads current search/filter/page state and refreshes table
    void    populateCategoryFilter();
    void    setRowData(int row, const ProductDTO &p);
    void    addActionButtons(int row, const ProductDTO &p);
    void    updateStatusBar(int shownCount);
    void    updatePagerControls();
    QString currentCategoryFilter() const;

    Ui::Product *ui;

    // ── Pagination state ──────────────────────────────────────
    static const int PAGE_SIZE = 10;
    int m_currentPage = 0;   // zero-indexed
    int m_totalCount  = 0;   // total rows matching current search/filter
};

#endif // PRODUCT_H
