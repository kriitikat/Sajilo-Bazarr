#ifndef PRODUCT_H
#define PRODUCT_H

#include <QWidget>
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDate>

// ─────────────────────────────────────────────
//  Data Transfer Object  (matches bazar.db)
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
//  Add / Edit Product Dialog
// ─────────────────────────────────────────────
class ProductDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductDialog(QWidget *parent = nullptr,
                           const ProductDTO &product = ProductDTO{});
    ~ProductDialog() override;

    ProductDTO getProduct() const;

private slots:
    void onGenerateSku();
    void onAccept();

private:
    void    setupUi();
    void    populateFields(const ProductDTO &p);
    bool    validateInputs();
    QString generateSku();

    QLineEdit   *txtName      = nullptr;
    QComboBox   *cmbCategory  = nullptr;
    QLineEdit   *txtUnit      = nullptr;
    QLineEdit   *txtPrice     = nullptr;
    QSpinBox    *spnStock     = nullptr;
    QDateEdit   *deExpiry     = nullptr;
    QComboBox   *cmbStatus    = nullptr;
    QLineEdit   *txtSupplier  = nullptr;
    QLineEdit   *txtSku       = nullptr;
    QPushButton *btnGenSku    = nullptr;
    QPushButton *btnSave      = nullptr;
    QPushButton *btnCancel    = nullptr;

    bool m_editMode = false;
    int  m_editId   = 0;
};

// ─────────────────────────────────────────────
//  Main Product Widget
// ─────────────────────────────────────────────
namespace Ui { class Product; }

class Product : public QWidget
{
    Q_OBJECT

public:
    explicit Product(QWidget *parent = nullptr);
    ~Product() override;

    // s_units stays a fixed static list (the "categories" table in the
    // database has no equivalent "units" table to read from).
    static QStringList s_units;

    // Reads distinct category names live from the "categories" table.
    // Public + static so ProductDialog (a separate class) can call it
    // when building its own category dropdown — this is what keeps the
    // product Add/Edit form and the filter bar in sync with whatever was
    // most recently added/edited/deleted on the Category page.
    static QStringList loadCategoriesFromDb();

private slots:
    void onAddProduct();
    void onSearchChanged(const QString &text);
    void onFilterCategoryChanged(int index);
    void onClearSearch();
    void onEditProduct();
    void onDeleteProduct();
    void onTableDoubleClicked(int row, int column);

private:
    bool              saveProduct(const ProductDTO &p);
    bool              updateProduct(const ProductDTO &p);
    bool              deleteProduct(int id);
    QList<ProductDTO> fetchProducts(const QString &search   = QString(),
                                    const QString &category = QString());

    void loadProducts(const QString &search   = QString(),
                      const QString &category = QString());
    void populateCategoryFilter();
    void setRowData(int row, const ProductDTO &p);
    void addActionButtons(int row, int productId);
    void updateStatusBar(int count);

    Ui::Product *ui;
};

#endif // PRODUCT_H