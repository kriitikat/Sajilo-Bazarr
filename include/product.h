#ifndef PRODUCT_H
#define PRODUCT_H

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPushButton>

namespace Ui {
class Product;
}


struct ProductDTO {
    int     id = 0;
    QString sku;
    QString name;
    QString category;
    double  price = 0.0;
    int     stock = 0;
    QString unit;
    QString expiryDate;
    QString status;
};


class ProductDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProductDialog(QWidget *parent = nullptr);
    ProductDTO getProduct() const;

private slots:
    void onGenerateSku();
    void onAccept();

private:
    void setupUi();
    bool validateInputs();
    QString generateSku();

    QLineEdit   *txtSku;
    QLineEdit   *txtName;
    QComboBox   *cmbCategory;
    QLineEdit   *txtPrice;
    QSpinBox    *spnStock;
    QComboBox   *cmbUnit;
    QDateEdit   *deExpiry;
    QComboBox   *cmbStatus;
    QPushButton *btnGenSku;
    QPushButton *btnSave;
    QPushButton *btnCancel;
};


class Product : public QWidget
{
    Q_OBJECT
public:
    explicit Product(QWidget *parent = nullptr);
    ~Product();

    static QStringList s_categories;
    static QStringList s_units;

private slots:
    void onAddProduct();

private:
    void addRowToTable(const ProductDTO &p);

    Ui::Product *ui;
    QList<ProductDTO> m_products;
    int m_nextId = 1;
};

#endif // PRODUCT_H