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
    QLineEdit   *txtUnit     = nullptr;
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

private:
    bool saveProduct(const StaffProductDTO &p);
    bool updateProduct(const StaffProductDTO &p);
    bool deleteProduct(int id);

    QList<StaffProductDTO> fetchProducts(const QString &search   = QString(),
                                         const QString &category = QString());

    void loadProducts(const QString &search   = QString(),
                      const QString &category = QString());
    void populateCategoryFilter();
    void setRowData(int row, const StaffProductDTO &p);
    void addActionButtons(int row, int productId);
    void updateStatusBar(int count);

    Ui::ProductStaff *ui;
};

#endif // PRODUCTSTAFF_H