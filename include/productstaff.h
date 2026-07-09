#ifndef PRODUCTSTAFF_H
#define PRODUCTSTAFF_H
#include <QWidget>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>
class QLineEdit;
class QComboBox;
class QSpinBox;
class QDateEdit;
class QPushButton;
namespace Ui { class ProductStaff; }
// ─────────────────────────────────────────────
//  Data Transfer Object  (matches bazar.db "products" table)
// ─────────────────────────────────────────────
struct StaffProductDTO {
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
//  Add / Edit Dialog
// ─────────────────────────────────────────────
class ProductStaffDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProductStaffDialog(QWidget *parent = nullptr,
                                const StaffProductDTO &product = StaffProductDTO());
    ~ProductStaffDialog() override;
    StaffProductDTO getProduct() const;
private slots:
    void onGenerateSku();
    void onAccept();
private:
    void setupUi();
    void populateFields(const StaffProductDTO &p);
    QString generateSku();
    bool validateInputs();
    bool m_editMode = false;
    int  m_editId   = 0;
    QLineEdit   *txtName     = nullptr;
    QComboBox   *cmbCategory = nullptr;
    QComboBox   *cmbUnit     = nullptr;
    QLineEdit   *txtPrice    = nullptr;
    QSpinBox    *spnStock    = nullptr;
    QDateEdit   *deExpiry    = nullptr;
    QComboBox   *cmbStatus   = nullptr;
    QLineEdit   *txtSupplier = nullptr;
    QLineEdit   *txtSku      = nullptr;
    QPushButton *btnGenSku   = nullptr;
    QPushButton *btnSave     = nullptr;
    QPushButton *btnCancel   = nullptr;
};
// ─────────────────────────────────────────────
//  Main Product Staff Widget
// ─────────────────────────────────────────────
class ProductStaff : public QWidget
{
    Q_OBJECT
public:
    explicit ProductStaff(QWidget *parent = nullptr);
    ~ProductStaff() override;
    // Reads distinct category names live from the "categories" table,
    // used to populate category dropdowns.
    static QStringList loadCategoriesFromDb();
    static QStringList s_units;
private slots:
    void onAddProduct();
    void onSearchChanged(const QString &text);
    void onFilterCategoryChanged(int index);
    void onClearSearch();
    void onEditProduct();
    void onDeleteProduct();
    void onTableDoubleClicked(int row, int column);
    void onPrevPage();
    void onNextPage();
private:
    bool saveProduct(const StaffProductDTO &p);
    bool updateProduct(const StaffProductDTO &p);
    bool deleteProduct(int id);

    // limit/offset aware fetch: limit <= 0 means "no limit" (fetch all)
    QList<StaffProductDTO> fetchProducts(const QString &search   = QString(),
                                         const QString &category = QString(),
                                         int limit = -1,
                                         int offset = 0);
    int  fetchProductCount(const QString &search, const QString &category);

    void loadProducts(const QString &search   = QString(),
                      const QString &category = QString());
    void populateCategoryFilter();
    void setRowData(int row, const StaffProductDTO &p);
    void addActionButtons(int row, int productId);
    void updateStatusBar(int rowsShownOnPage);
    void updatePaginationControls();

    // ── Pagination state ────────────────────────────────
    int     m_currentPage   = 1;
    int     m_pageSize      = 10;
    int     m_totalRecords  = 0;
    int     m_totalPages    = 1;
    QString m_lastSearch;
    QString m_lastCategory;

    Ui::ProductStaff *ui;
};
#endif // PRODUCTSTAFF_H