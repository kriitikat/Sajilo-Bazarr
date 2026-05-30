// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Product Module
//  product.cpp
//
//  Requires Qt modules:  widgets  sql
//  Add to .pro:  QT += widgets sql
// ═══════════════════════════════════════════════════════════════════

#include "../include/product.h"
#include "../ui/ui_product.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFrame>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// ── Static data ───────────────────────────────────────────────────
QStringList Product::s_categories = {
    "Food & Beverages", "Dairy Products", "Bakery",
    "Fruits & Vegetables", "Meat & Fish",
    "Personal Care", "Household", "Electronics",
    "Clothing", "Stationery", "Medicines", "Other"
};

QStringList Product::s_units = {
    "pcs", "kg", "g", "litre", "ml",
    "dozen", "box", "packet", "bottle", "can",
    "meter", "pair", "set"
};

// ═══════════════════════════════════════════════════════════════════
//  DATABASE INITIALISATION
// ═══════════════════════════════════════════════════════════════════
bool Product::initDatabase(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qCritical() << "[DB] Cannot open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery q;
    const QString createSQL = R"(
        CREATE TABLE IF NOT EXISTS products (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            sku         TEXT    NOT NULL UNIQUE,
            name        TEXT    NOT NULL,
            category    TEXT    NOT NULL DEFAULT '',
            price       REAL    NOT NULL DEFAULT 0.0,
            stock       INTEGER NOT NULL DEFAULT 0,
            unit        TEXT    NOT NULL DEFAULT 'pcs',
            expiry_date TEXT    NOT NULL DEFAULT '',
            status      TEXT    NOT NULL DEFAULT 'Active'
        );
    )";

    if (!q.exec(createSQL)) {
        qCritical() << "[DB] Table creation failed:" << q.lastError().text();
        return false;
    }

    qDebug() << "[DB] Database ready at" << dbPath;
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  PRODUCT DIALOG  –  Add / Edit
// ═══════════════════════════════════════════════════════════════════
ProductDialog::ProductDialog(QWidget *parent, const ProductDTO &product)
    : QDialog(parent)
{
    m_editMode = (product.id > 0);
    m_editId   = product.id;

    setWindowTitle(m_editMode ? "Edit Product" : "Add New Product");
    setMinimumWidth(480);
    setModal(true);

    setupUi();

    if (m_editMode)
        populateFields(product);
    else
        onGenerateSku();
}

ProductDialog::~ProductDialog() {}

void ProductDialog::setupUi()
{
    auto *lblTitle = new QLabel(m_editMode ? "✏  Edit Product" : "➕  Add New Product", this);
    lblTitle->setStyleSheet("font-size:16px; font-weight:bold; color:#1A73E8; padding:6px 0;");

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#DDE3EC;");

    txtSku = new QLineEdit(this);
    txtSku->setPlaceholderText("e.g. SB-FD-00123");
    txtSku->setMaxLength(30);

    btnGenSku = new QPushButton("⟳ Generate", this);
    btnGenSku->setFixedWidth(100);
    btnGenSku->setStyleSheet(
        "QPushButton{background:#E8F0FE;color:#1A73E8;border:1px solid #ADC6FF;"
        "border-radius:4px;padding:5px;}"
        "QPushButton:hover{background:#D2E3FC;}");
    connect(btnGenSku, &QPushButton::clicked, this, &ProductDialog::onGenerateSku);

    auto *skuRow = new QHBoxLayout;
    skuRow->addWidget(txtSku);
    skuRow->addWidget(btnGenSku);

    txtName = new QLineEdit(this);
    txtName->setPlaceholderText("Enter product name");

    cmbCategory = new QComboBox(this);
    for (const auto &c : Product::s_categories)
        cmbCategory->addItem(c);

    txtPrice = new QLineEdit(this);
    txtPrice->setPlaceholderText("0.00");
    txtPrice->setValidator(new QRegularExpressionValidator(
        QRegularExpression(R"(^\d{0,10}(\.\d{0,2})?$)"), txtPrice));

    spnStock = new QSpinBox(this);
    spnStock->setRange(0, 999999);
    spnStock->setSuffix("  units");

    cmbUnit = new QComboBox(this);
    for (const auto &u : Product::s_units)
        cmbUnit->addItem(u);

    deExpiry = new QDateEdit(this);
    deExpiry->setCalendarPopup(true);
    deExpiry->setDisplayFormat("yyyy-MM-dd");
    deExpiry->setDate(QDate::currentDate().addYears(1));
    deExpiry->setMinimumDate(QDate::currentDate());

    cmbStatus = new QComboBox(this);
    cmbStatus->addItems({"Active", "Inactive"});

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setSpacing(12);
    form->setContentsMargins(16, 8, 16, 8);

    auto makeLabel = [](const QString &txt) {
        auto *l = new QLabel(txt);
        l->setStyleSheet("font-weight:600; color:#2C3E50;");
        return l;
    };

    form->addRow(makeLabel("SKU *"),          skuRow);
    form->addRow(makeLabel("Product Name *"), txtName);
    form->addRow(makeLabel("Category *"),     cmbCategory);
    form->addRow(makeLabel("Price (Rs.) *"),  txtPrice);
    form->addRow(makeLabel("Stock *"),        spnStock);
    form->addRow(makeLabel("Unit"),           cmbUnit);
    form->addRow(makeLabel("Expiry Date"),    deExpiry);
    form->addRow(makeLabel("Status"),         cmbStatus);

    btnSave   = new QPushButton(m_editMode ? "💾  Save Changes" : "✔  Add Product", this);
    btnCancel = new QPushButton("✕  Cancel", this);

    btnSave->setStyleSheet(
        "QPushButton{background:#1A73E8;color:white;border:none;border-radius:6px;"
        "padding:8px 22px;font-weight:bold;}"
        "QPushButton:hover{background:#1558C0;}");
    btnCancel->setStyleSheet(
        "QPushButton{background:#E8EAED;color:#444;border:none;border-radius:6px;"
        "padding:8px 22px;}"
        "QPushButton:hover{background:#D2D5DB;}");

    connect(btnSave,   &QPushButton::clicked, this, &ProductDialog::onAccept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnSave);

    auto *master = new QVBoxLayout(this);
    master->setSpacing(8);
    master->setContentsMargins(0, 12, 0, 12);
    master->addWidget(lblTitle, 0, Qt::AlignCenter);
    master->addWidget(separator);
    master->addLayout(form);
    master->addSpacing(8);
    master->addLayout(btnRow);
    master->addSpacing(4);

    setStyleSheet(R"(
        QDialog { background:#FFFFFF; }
        QLineEdit, QComboBox, QSpinBox, QDateEdit {
            background:#FFFFFF; border:1px solid #C8D0DC;
            border-radius:5px; padding:5px 8px; min-height:28px;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus {
            border:1px solid #1A73E8;
        }
    )");
}

void ProductDialog::populateFields(const ProductDTO &p)
{
    txtSku->setText(p.sku);
    txtSku->setReadOnly(true);
    btnGenSku->setEnabled(false);
    txtName->setText(p.name);

    int catIdx = cmbCategory->findText(p.category);
    if (catIdx >= 0) cmbCategory->setCurrentIndex(catIdx);

    txtPrice->setText(QString::number(p.price, 'f', 2));
    spnStock->setValue(p.stock);

    int unitIdx = cmbUnit->findText(p.unit);
    if (unitIdx >= 0) cmbUnit->setCurrentIndex(unitIdx);

    if (!p.expiryDate.isEmpty())
        deExpiry->setDate(QDate::fromString(p.expiryDate, "yyyy-MM-dd"));

    cmbStatus->setCurrentText(p.status);
}

QString ProductDialog::generateSku()
{
    static QMap<QString,QString> catCode = {
        {"Food & Beverages","FB"}, {"Dairy Products","DP"}, {"Bakery","BK"},
        {"Fruits & Vegetables","FV"}, {"Meat & Fish","MF"},
        {"Personal Care","PC"}, {"Household","HH"}, {"Electronics","EL"},
        {"Clothing","CL"}, {"Stationery","ST"}, {"Medicines","MD"}, {"Other","OT"}
    };

    QString cat  = cmbCategory ? cmbCategory->currentText() : "OT";
    QString code = catCode.value(cat, "OT");
    quint32 num  = QRandomGenerator::global()->bounded(10000u, 99999u);
    return QString("SB-%1-%2").arg(code).arg(num);
}

void ProductDialog::onGenerateSku()
{
    txtSku->setText(generateSku());
}

bool ProductDialog::validateInputs()
{
    if (txtSku->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "SKU is required.");
        txtSku->setFocus();
        return false;
    }
    if (txtName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Product Name is required.");
        txtName->setFocus();
        return false;
    }
    if (txtPrice->text().trimmed().isEmpty() ||
        txtPrice->text().toDouble() < 0) {
        QMessageBox::warning(this, "Validation", "Please enter a valid Price.");
        txtPrice->setFocus();
        return false;
    }
    return true;
}

void ProductDialog::onAccept()
{
    if (validateInputs())
        accept();
}

ProductDTO ProductDialog::getProduct() const
{
    ProductDTO p;
    p.id         = m_editId;
    p.sku        = txtSku->text().trimmed().toUpper();
    p.name       = txtName->text().trimmed();
    p.category   = cmbCategory->currentText();
    p.price      = txtPrice->text().toDouble();
    p.stock      = spnStock->value();
    p.unit       = cmbUnit->currentText();
    p.expiryDate = deExpiry->date().toString("yyyy-MM-dd");
    p.status     = cmbStatus->currentText();
    return p;
}

// ═══════════════════════════════════════════════════════════════════
//  PRODUCT WIDGET  –  Main Screen
// ═══════════════════════════════════════════════════════════════════
Product::Product(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Product)
{
    ui->setupUi(this);

    QTableWidget *tbl = ui->tblProducts;
    tbl->setColumnWidth(0, 45);
    tbl->setColumnWidth(1, 110);
    tbl->setColumnWidth(2, 200);
    tbl->setColumnWidth(3, 140);
    tbl->setColumnWidth(4, 90);
    tbl->setColumnWidth(5, 65);
    tbl->setColumnWidth(6, 60);
    tbl->setColumnWidth(7, 95);
    tbl->setColumnWidth(8, 75);
    tbl->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
    tbl->verticalHeader()->setVisible(false);
    tbl->setWordWrap(false);

    populateCategoryFilter();

    connect(ui->btnAddProduct,     &QPushButton::clicked,
            this,                  &Product::onAddProduct);
    connect(ui->txtSearch,         &QLineEdit::textChanged,
            this,                  &Product::onSearchChanged);
    connect(ui->cmbFilterCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,                  &Product::onFilterCategoryChanged);
    connect(ui->btnClearSearch,    &QPushButton::clicked,
            this,                  &Product::onClearSearch);
    connect(ui->tblProducts,       &QTableWidget::cellDoubleClicked,
            this,                  &Product::onTableDoubleClicked);

    loadProducts();
}

Product::~Product()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  DATABASE OPERATIONS
// ─────────────────────────────────────────────────────────────────
bool Product::saveProduct(const ProductDTO &p)
{
    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO products (sku, name, category, price, stock, unit, expiry_date, status)
        VALUES (:sku, :name, :cat, :price, :stock, :unit, :exp, :status)
    )");
    q.bindValue(":sku",    p.sku);
    q.bindValue(":name",   p.name);
    q.bindValue(":cat",    p.category);
    q.bindValue(":price",  p.price);
    q.bindValue(":stock",  p.stock);
    q.bindValue(":unit",   p.unit);
    q.bindValue(":exp",    p.expiryDate);
    q.bindValue(":status", p.status);

    if (!q.exec()) {
        QString err = q.lastError().text();
        if (err.contains("UNIQUE", Qt::CaseInsensitive))
            QMessageBox::warning(this, "Duplicate SKU",
                                 "A product with this SKU already exists.\nPlease use a unique SKU.");
        else
            QMessageBox::critical(this, "Database Error", err);
        return false;
    }
    return true;
}

bool Product::updateProduct(const ProductDTO &p)
{
    QSqlQuery q;
    q.prepare(R"(
        UPDATE products
        SET name=:name, category=:cat, price=:price,
            stock=:stock, unit=:unit, expiry_date=:exp, status=:status
        WHERE id=:id
    )");
    q.bindValue(":name",   p.name);
    q.bindValue(":cat",    p.category);
    q.bindValue(":price",  p.price);
    q.bindValue(":stock",  p.stock);
    q.bindValue(":unit",   p.unit);
    q.bindValue(":exp",    p.expiryDate);
    q.bindValue(":status", p.status);
    q.bindValue(":id",     p.id);

    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

bool Product::deleteProduct(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM products WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

QList<ProductDTO> Product::fetchProducts(const QString &search,
                                         const QString &category)
{
    QList<ProductDTO> list;

    QString sql = R"(
        SELECT id, sku, name, category, price, stock, unit, expiry_date, status
        FROM products
        WHERE 1=1
    )";
    if (!search.isEmpty())
        sql += " AND (name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";
    sql += " ORDER BY name ASC";

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty())
        q.bindValue(":search", "%" + search + "%");
    if (!category.isEmpty())
        q.bindValue(":category", category);

    if (!q.exec()) {
        qWarning() << "[DB] fetchProducts error:" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        ProductDTO p;
        p.id         = q.value(0).toInt();
        p.sku        = q.value(1).toString();
        p.name       = q.value(2).toString();
        p.category   = q.value(3).toString();
        p.price      = q.value(4).toDouble();
        p.stock      = q.value(5).toInt();
        p.unit       = q.value(6).toString();
        p.expiryDate = q.value(7).toString();
        p.status     = q.value(8).toString();
        list.append(p);
    }
    return list;
}

// ─────────────────────────────────────────────────────────────────
//  UI HELPERS
// ─────────────────────────────────────────────────────────────────
void Product::populateCategoryFilter()
{
    ui->cmbFilterCategory->blockSignals(true);
    ui->cmbFilterCategory->clear();
    ui->cmbFilterCategory->addItem("All Categories");
    for (const auto &c : s_categories)
        ui->cmbFilterCategory->addItem(c);
    ui->cmbFilterCategory->blockSignals(false);
}

void Product::loadProducts(const QString &search, const QString &category)
{
    QTableWidget *tbl = ui->tblProducts;
    tbl->setUpdatesEnabled(false);
    tbl->setSortingEnabled(false);
    tbl->setRowCount(0);

    QList<ProductDTO> products = fetchProducts(search, category);

    for (int row = 0; row < products.size(); ++row) {
        tbl->insertRow(row);
        setRowData(row, products[row]);
        addActionButtons(row, products[row].id);
    }

    tbl->setSortingEnabled(true);
    tbl->setUpdatesEnabled(true);
    updateStatusBar(products.size());
}

void Product::setRowData(int row, const ProductDTO &p)
{
    QTableWidget *tbl = ui->tblProducts;

    auto *itemNo    = new QTableWidgetItem(QString::number(row + 1));
    auto *itemSku   = new QTableWidgetItem(p.sku);
    auto *itemName  = new QTableWidgetItem(p.name);
    auto *itemCat   = new QTableWidgetItem(p.category);
    auto *itemPrice = new QTableWidgetItem(
        QString("Rs. %1").arg(p.price, 0, 'f', 2));
    auto *itemStock = new QTableWidgetItem(QString::number(p.stock));
    auto *itemUnit  = new QTableWidgetItem(p.unit);
    auto *itemExp   = new QTableWidgetItem(
        p.expiryDate.isEmpty() ? "—" : p.expiryDate);

    itemSku->setData(Qt::UserRole, p.id);

    itemNo->setTextAlignment(Qt::AlignCenter);
    itemPrice->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    itemStock->setTextAlignment(Qt::AlignCenter);
    itemUnit->setTextAlignment(Qt::AlignCenter);
    itemExp->setTextAlignment(Qt::AlignCenter);

    auto *itemStatus = new QTableWidgetItem(p.status);
    itemStatus->setTextAlignment(Qt::AlignCenter);
    if (p.status == "Active") {
        itemStatus->setForeground(QColor("#1B8A44"));
        itemStatus->setBackground(QColor("#E6F4EA"));
    } else {
        itemStatus->setForeground(QColor("#C0392B"));
        itemStatus->setBackground(QColor("#FDEDEC"));
    }

    if (p.stock <= 5 && p.stock >= 0) {
        itemStock->setForeground(QColor("#C0392B"));
        QFont f; f.setBold(true);
        itemStock->setFont(f);
    }

    tbl->setItem(row, 0, itemNo);
    tbl->setItem(row, 1, itemSku);
    tbl->setItem(row, 2, itemName);
    tbl->setItem(row, 3, itemCat);
    tbl->setItem(row, 4, itemPrice);
    tbl->setItem(row, 5, itemStock);
    tbl->setItem(row, 6, itemUnit);
    tbl->setItem(row, 7, itemExp);
    tbl->setItem(row, 8, itemStatus);
}

void Product::addActionButtons(int row, int productId)
{
    auto *cell   = new QWidget;
    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    auto *btnEdit = new QPushButton("✏ Edit");
    auto *btnDel  = new QPushButton("🗑 Delete");

    btnEdit->setProperty("productId", productId);
    btnDel ->setProperty("productId", productId);

    btnEdit->setStyleSheet(
        "QPushButton{background:#E8F0FE;color:#1A73E8;border:1px solid #ADC6FF;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#D2E3FC;}");
    btnDel->setStyleSheet(
        "QPushButton{background:#FDEDEC;color:#C0392B;border:1px solid #F5B7B1;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#FADBD8;}");

    connect(btnEdit, &QPushButton::clicked, this, &Product::onEditProduct);
    connect(btnDel,  &QPushButton::clicked, this, &Product::onDeleteProduct);

    layout->addWidget(btnEdit);
    layout->addWidget(btnDel);
    layout->addStretch();

    ui->tblProducts->setCellWidget(row, 9, cell);
    ui->tblProducts->setRowHeight(row, 40);
}

void Product::updateStatusBar(int count)
{
    ui->lblTotalProducts->setText(
        QString("Total: %1 product%2").arg(count).arg(count == 1 ? "" : "s"));
    ui->lblStatusBar->setText(
        QString("Last refreshed: %1")
            .arg(QDateTime::currentDateTime().toString("dd-MMM-yyyy  hh:mm:ss")));
}

// ─────────────────────────────────────────────────────────────────
//  STATIC HELPERS
// ─────────────────────────────────────────────────────────────────
static int idFromRow(QTableWidget *tbl, int row)
{
    auto *item = tbl->item(row, 1);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

static ProductDTO fetchById(int id)
{
    ProductDTO p;
    QSqlQuery q;
    q.prepare("SELECT id,sku,name,category,price,stock,unit,expiry_date,status "
              "FROM products WHERE id=:id");
    q.bindValue(":id", id);
    if (q.exec() && q.next()) {
        p.id         = q.value(0).toInt();
        p.sku        = q.value(1).toString();
        p.name       = q.value(2).toString();
        p.category   = q.value(3).toString();
        p.price      = q.value(4).toDouble();
        p.stock      = q.value(5).toInt();
        p.unit       = q.value(6).toString();
        p.expiryDate = q.value(7).toString();
        p.status     = q.value(8).toString();
    }
    return p;
}

// ─────────────────────────────────────────────────────────────────
//  SLOTS
// ─────────────────────────────────────────────────────────────────
void Product::onAddProduct()
{
    ProductDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        ProductDTO p = dlg.getProduct();
        if (saveProduct(p)) {
            ui->lblStatusBar->setText(
                QString("✔  Product '%1' added successfully.").arg(p.name));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
    }
}

void Product::onSearchChanged(const QString &text)
{
    QString cat = ui->cmbFilterCategory->currentIndex() == 0
                      ? QString()
                      : ui->cmbFilterCategory->currentText();
    loadProducts(text.trimmed(), cat);
}

void Product::onFilterCategoryChanged(int)
{
    QString cat = ui->cmbFilterCategory->currentIndex() == 0
                      ? QString()
                      : ui->cmbFilterCategory->currentText();
    loadProducts(ui->txtSearch->text().trimmed(), cat);
}

void Product::onClearSearch()
{
    ui->txtSearch->clear();
    ui->cmbFilterCategory->setCurrentIndex(0);
    loadProducts();
}

void Product::onEditProduct()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    ProductDTO p = fetchById(id);
    if (p.id <= 0) {
        QMessageBox::warning(this, "Not Found", "Product record not found.");
        return;
    }

    ProductDialog dlg(this, p);
    if (dlg.exec() == QDialog::Accepted) {
        ProductDTO updated = dlg.getProduct();
        if (updateProduct(updated)) {
            ui->lblStatusBar->setText(
                QString("✔  Product '%1' updated successfully.").arg(updated.name));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
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
        QString("Are you sure you want to delete <b>%1</b> (SKU: %2)?<br>"
                "<span style='color:red;'>This action cannot be undone.</span>")
            .arg(p.name, p.sku),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ans == QMessageBox::Yes && deleteProduct(id)) {
        ui->lblStatusBar->setText(
            QString("🗑  Product '%1' deleted.").arg(p.name));
        loadProducts(ui->txtSearch->text(),
                     ui->cmbFilterCategory->currentIndex() == 0
                         ? QString()
                         : ui->cmbFilterCategory->currentText());
    }
}

void Product::onTableDoubleClicked(int row, int)
{
    int id = idFromRow(ui->tblProducts, row);
    if (id <= 0) return;

    ProductDTO p = fetchById(id);
    if (p.id <= 0) return;

    ProductDialog dlg(this, p);
    if (dlg.exec() == QDialog::Accepted) {
        ProductDTO updated = dlg.getProduct();
        if (updateProduct(updated)) {
            ui->lblStatusBar->setText(
                QString("✔  Product '%1' updated.").arg(updated.name));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
    }
}
void Product::on_btnAddProduct_clicked()
{
    void Product::on_addProductButton_clicked()
    {
        QString sku = ui->skuLineEdit->text();
        QString category = ui->categoryComboBox->currentText();
        QString price = ui->priceLineEdit->text();
        QString stock = ui->stockLineEdit->text();

        if(sku.isEmpty() || price.isEmpty()) {
            QMessageBox::warning(this, "Error", "SKU and Price are required!");
            return;
        }

        // Insert into database
        QSqlQuery query;
        query.prepare("INSERT INTO products (sku, category, price, stock) "
                      "VALUES (:sku, :category, :price, :stock)");
        query.bindValue(":sku", sku);
        query.bindValue(":category", category);
        query.bindValue(":price", price.toDouble());
        query.bindValue(":stock", stock.toInt());

        if(query.exec()) {
            QMessageBox::information(this, "Success", "Product added!");
            loadProducts(); // refresh table
        } else {
            QMessageBox::critical(this, "Error", query.lastError().text());
        }
    }
}

