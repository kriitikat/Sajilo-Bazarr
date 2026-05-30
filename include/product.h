#ifndef PRODUCT_H
#define PRODUCT_H

#include <QWidget>
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>

// ─────────────────────────────────────────────
//  Data Transfer Object
// ─────────────────────────────────────────────
struct ProductDTO {
    int     id          = 0;
    QString sku;
    QString name;
    QString category;
    double  price       = 0.0;
    int     stock       = 0;
    QString unit;
    QString expiryDate;   // stored as "YYYY-MM-DD"
    QString status;       // "Active" | "Inactive"
};

// ─────────────────────────────────────────────
//  Add / Edit Product Dialog
// ─────────────────────────────────────────────
namespace Ui { class ProductDialog; }

class ProductDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductDialog(QWidget *parent = nullptr,
                           const ProductDTO &product = ProductDTO{});
    ~ProductDialog();

    ProductDTO  getProduct() const;

private slots:
    void onGenerateSku();
    void onAccept();

private:
    void setupUi();
    void populateFields(const ProductDTO &p);
    bool validateInputs();
    QString generateSku();

    class QLineEdit   *txtSku       = nullptr;
    class QLineEdit   *txtName      = nullptr;
    class QComboBox   *cmbCategory  = nullptr;
    class QLineEdit   *txtPrice     = nullptr;
    class QSpinBox    *spnStock     = nullptr;
    class QComboBox   *cmbUnit      = nullptr;
    class QDateEdit   *deExpiry     = nullptr;
    class QComboBox   *cmbStatus    = nullptr;
    class QPushButton *btnGenSku    = nullptr;
    class QPushButton *btnSave      = nullptr;
    class QPushButton *btnCancel    = nullptr;

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
    ~Product();

    static bool initDatabase(const QString &dbPath = "sajilo_bazar.db");

private slots:
    void onAddProduct();
    void onSearchChanged(const QString &text);
    void onFilterCategoryChanged(int index);
    void onClearSearch();
    void onEditProduct();
    void onDeleteProduct();
    void onTableDoubleClicked(int row, int column);

    void on_btnAddProduct_clicked();

private:
    bool        saveProduct(const ProductDTO &p);
    bool        updateProduct(const ProductDTO &p);
    bool        deleteProduct(int id);
    QList<ProductDTO> fetchProducts(const QString &search   = QString(),
                                    const QString &category = QString());

    void        loadProducts(const QString &search   = QString(),
                      const QString &category = QString());
    void        populateCategoryFilter();
    void        setRowData(int row, const ProductDTO &p);
    void        addActionButtons(int row, int productId);
    void        updateStatusBar(int count);
    ProductDTO  productFromRow(int row) const;

    Ui::Product *ui;

    static QStringList s_categories;
    static QStringList s_units;
};

#endif // PRODUCT_H