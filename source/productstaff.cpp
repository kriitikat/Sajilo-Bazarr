// ═══════════════════════════════════════════════════════════════════
//  Sajilo Bazar – Product Staff Module
//  productstaff.cpp
//  Reads/writes bazar.db → products table
//  Columns: id, product_name, category, unit, price, stock,
//           expiry_date, status, supplier, sku
// ═══════════════════════════════════════════════════════════════════

#include "../include/productstaff.h"
#include "../ui/ui_productstaff.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QDateTime>
#include <QDate>
#include <QMap>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QColor>
#include <QFont>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPushButton>
#include <QMessageBox>

// ── Static data ───────────────────────────────────────────────────
QStringList ProductStaff::s_units = {
    "pcs", "kg", "g", "litre", "ml",
    "dozen", "box", "packet", "bottle", "can",
    "meter", "pair", "set", "piece", "pieces"
};

QStringList ProductStaff::loadCategoriesFromDb()
{
    QStringList categories;

    QSqlQuery q;
    if (!q.exec("SELECT category_name FROM categories "
                "WHERE category_name IS NOT NULL AND TRIM(category_name) <> '' "
                "ORDER BY category_name ASC")) {
        qWarning() << "ProductStaff: failed to load categories from DB -"
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
static int idFromRow(QTableWidget *tbl, int row)
{
    auto *item = tbl->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

static StaffProductDTO fetchById(int id)
{
    StaffProductDTO p;
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
//  PRODUCT STAFF DIALOG  –  Add / Edit
// ═══════════════════════════════════════════════════════════════════
ProductStaffDialog::ProductStaffDialog(QWidget *parent, const StaffProductDTO &product)
    : QDialog(parent)
{
    m_editMode = (product.id > 0);
    m_editId   = product.id;

    setWindowTitle(m_editMode ? "Edit Product" : "Add New Product");
    setMinimumWidth(500);
    setModal(true);

    setupUi();

    if (m_editMode)
        populateFields(product);
    else
        onGenerateSku();
}

ProductStaffDialog::~ProductStaffDialog() {}

void ProductStaffDialog::setupUi()
{
    auto *lblTitle = new QLabel(
        m_editMode ? "✏  Edit Product" : "➕  Add New Product", this);
    lblTitle->setStyleSheet(
        "font-size:16px; font-weight:bold; color:#1B5E3C; padding:6px 0;");

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#DDE3EC;");

    // ── Fields ──────────────────────────────────────────────────
    txtName = new QLineEdit(this);
    txtName->setPlaceholderText("Enter product name");

    cmbCategory = new QComboBox(this);
    for (const auto &c : ProductStaff::loadCategoriesFromDb())
        cmbCategory->addItem(c);

    // ── Unit dropdown (editable so user can still type a custom unit) ──
    cmbUnit = new QComboBox(this);
    cmbUnit->setEditable(true);
    cmbUnit->addItems(ProductStaff::s_units);
    cmbUnit->setCurrentIndex(-1);                 // start empty
    if (cmbUnit->lineEdit())
        cmbUnit->lineEdit()->setPlaceholderText("e.g. kg, pcs, litre");

    txtPrice = new QLineEdit(this);
    txtPrice->setPlaceholderText("0.00");
    txtPrice->setValidator(new QRegularExpressionValidator(
        QRegularExpression(R"(^\d{0,10}(\.\d{0,2})?$)"), txtPrice));

    spnStock = new QSpinBox(this);
    spnStock->setRange(0, 999999);
    spnStock->setSuffix("  units");

    deExpiry = new QDateEdit(this);
    deExpiry->setCalendarPopup(true);
    deExpiry->setDisplayFormat("yyyy-MM-dd");
    deExpiry->setDate(QDate::currentDate().addYears(1));
    deExpiry->setMinimumDate(QDate(2000, 1, 1));

    cmbStatus = new QComboBox(this);
    cmbStatus->addItems({"In Stock", "Low Stock", "High Stock", "Out Of Stock"});
    txtSupplier = new QLineEdit(this);
    txtSupplier->setPlaceholderText("Supplier name");

    // SKU row
    txtSku = new QLineEdit(this);
    txtSku->setPlaceholderText("e.g. SKU001");
    txtSku->setMaxLength(50);

    btnGenSku = new QPushButton("⟳ Generate", this);
    btnGenSku->setFixedWidth(100);
    btnGenSku->setStyleSheet(
        "QPushButton{background:#E6F4EA;color:#1B5E3C;border:1px solid #1B5E3C;"
        "border-radius:4px;padding:5px;}"
        "QPushButton:hover{background:#C8E6D0;}");
    connect(btnGenSku, &QPushButton::clicked, this, &ProductStaffDialog::onGenerateSku);

    auto *skuRow = new QHBoxLayout;
    skuRow->addWidget(txtSku);
    skuRow->addWidget(btnGenSku);

    // ── Form layout ──────────────────────────────────────────────
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setSpacing(12);
    form->setContentsMargins(20, 8, 20, 8);

    auto makeLabel = [](const QString &txt) {
        auto *l = new QLabel(txt);
        l->setStyleSheet("font-weight:600; color:#2C3E50;");
        return l;
    };

    form->addRow(makeLabel("Product Name *"), txtName);
    form->addRow(makeLabel("Category *"),     cmbCategory);
    form->addRow(makeLabel("Unit"),           cmbUnit);
    form->addRow(makeLabel("Price (Rs.) *"),  txtPrice);
    form->addRow(makeLabel("Stock *"),        spnStock);
    form->addRow(makeLabel("Expiry Date"),    deExpiry);
    form->addRow(makeLabel("Status"),         cmbStatus);
    form->addRow(makeLabel("Supplier"),       txtSupplier);
    form->addRow(makeLabel("SKU"),            skuRow);

    // ── Buttons ──────────────────────────────────────────────────
    btnSave   = new QPushButton(
        m_editMode ? "💾  Save Changes" : "✔  Add Product", this);
    btnCancel = new QPushButton("✕  Cancel", this);

    btnSave->setStyleSheet(
        "QPushButton{background:#1B8A44;color:white;border:none;border-radius:6px;"
        "padding:8px 22px;font-weight:bold;}"
        "QPushButton:hover{background:#146C36;}");
    btnCancel->setStyleSheet(
        "QPushButton{background:#E8EAED;color:#444;border:none;border-radius:6px;"
        "padding:8px 22px;}"
        "QPushButton:hover{background:#D2D5DB;}");

    connect(btnSave,   &QPushButton::clicked, this, &ProductStaffDialog::onAccept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnSave);

    // ── Master layout ────────────────────────────────────────────
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
            border:1px solid #1B8A44;
        }
    )");
}

void ProductStaffDialog::populateFields(const StaffProductDTO &p)
{
    txtName->setText(p.productName);

    int catIdx = cmbCategory->findText(p.category);
    if (catIdx < 0 && !p.category.isEmpty()) {
        cmbCategory->addItem(p.category);
        catIdx = cmbCategory->count() - 1;
    }
    cmbCategory->setCurrentIndex(catIdx >= 0 ? catIdx : 0);

    // ── Unit: select if it exists in the list, otherwise add it ──
    int unitIdx = cmbUnit->findText(p.unit, Qt::MatchFixedString);
    if (unitIdx < 0 && !p.unit.isEmpty()) {
        cmbUnit->addItem(p.unit);
        unitIdx = cmbUnit->count() - 1;
    }
    cmbUnit->setCurrentIndex(unitIdx);
    if (unitIdx < 0)
        cmbUnit->setCurrentText(p.unit);

    txtPrice->setText(QString::number(p.price, 'f', 2));
    spnStock->setValue(p.stock);

    if (!p.expiryDate.isEmpty()) {
        QDate d = QDate::fromString(p.expiryDate, "yyyy-MM-dd");
        if (d.isValid()) deExpiry->setDate(d);
    }

    int stIdx = cmbStatus->findText(p.status);
    cmbStatus->setCurrentIndex(stIdx >= 0 ? stIdx : 0);

    txtSupplier->setText(p.supplier);

    txtSku->setText(p.sku);
    txtSku->setReadOnly(true);
    btnGenSku->setEnabled(false);
}

QString ProductStaffDialog::generateSku()
{
    static QMap<QString,QString> catCode = {
        {"Groceries","GR"}, {"Vegetables","VG"}, {"Fruits","FR"},
        {"Dairy","DY"}, {"Cosmetics","CS"}, {"Cleaning","CL"},
        {"Household","HH"}, {"Snacks","SN"}, {"Beverages","BV"},
        {"Bakery","BK"}, {"Stationery","ST"}, {"Meat","MT"},
        {"Breakfast","BR"}, {"Music","MU"}, {"Other","OT"}
    };

    QString cat  = cmbCategory ? cmbCategory->currentText() : "Other";
    QString code = catCode.value(cat, "OT");
    quint32 num  = QRandomGenerator::global()->bounded(100u, 999u);
    return QString("SKU-%1-%2").arg(code).arg(num, 3, 10, QChar('0'));
}

void ProductStaffDialog::onGenerateSku()
{
    txtSku->setText(generateSku());
}

bool ProductStaffDialog::validateInputs()
{
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

void ProductStaffDialog::onAccept()
{
    if (validateInputs())
        accept();
}

StaffProductDTO ProductStaffDialog::getProduct() const
{
    StaffProductDTO p;
    p.id          = m_editId;
    p.productName = txtName->text().trimmed();
    p.category    = cmbCategory->currentText();
    p.unit        = cmbUnit->currentText().trimmed();
    p.price       = txtPrice->text().toDouble();
    p.stock       = spnStock->value();
    p.expiryDate  = deExpiry->date().toString("yyyy-MM-dd");
    p.status      = cmbStatus->currentText();
    p.supplier    = txtSupplier->text().trimmed();
    p.sku         = txtSku->text().trimmed();
    return p;
}

// ═══════════════════════════════════════════════════════════════════
//  PRODUCT STAFF WIDGET  –  Main Screen
// ═══════════════════════════════════════════════════════════════════
ProductStaff::ProductStaff(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProductStaff)
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

    connect(ui->btnAddProduct,     &QPushButton::clicked,
            this,                  &ProductStaff::onAddProduct);
    connect(ui->txtSearch,         &QLineEdit::textChanged,
            this,                  &ProductStaff::onSearchChanged);
    connect(ui->cmbFilterCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,                  &ProductStaff::onFilterCategoryChanged);
    connect(ui->btnClearSearch,    &QPushButton::clicked,
            this,                  &ProductStaff::onClearSearch);
    connect(ui->tblProducts,       &QTableWidget::cellDoubleClicked,
            this,                  &ProductStaff::onTableDoubleClicked);

    loadProducts();
}

ProductStaff::~ProductStaff()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  DATABASE OPERATIONS
// ─────────────────────────────────────────────────────────────────
bool ProductStaff::saveProduct(const StaffProductDTO &p)
{
    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO products
            (product_name, category, unit, price, stock, expiry_date, status, supplier, sku)
        VALUES
            (:name, :cat, :unit, :price, :stock, :exp, :status, :supplier, :sku)
    )");
    q.bindValue(":name",     p.productName);
    q.bindValue(":cat",      p.category);
    q.bindValue(":unit",     p.unit);
    q.bindValue(":price",    p.price);
    q.bindValue(":stock",    p.stock);
    q.bindValue(":exp",      p.expiryDate);
    q.bindValue(":status",   p.status);
    q.bindValue(":supplier", p.supplier);
    q.bindValue(":sku",      p.sku);

    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

bool ProductStaff::updateProduct(const StaffProductDTO &p)
{
    QSqlQuery q;
    q.prepare(R"(
        UPDATE products
        SET product_name = :name,
            category     = :cat,
            unit         = :unit,
            price        = :price,
            stock        = :stock,
            expiry_date  = :exp,
            status       = :status,
            supplier     = :supplier,
            sku          = :sku
        WHERE id = :id
    )");
    q.bindValue(":name",     p.productName);
    q.bindValue(":cat",      p.category);
    q.bindValue(":unit",     p.unit);
    q.bindValue(":price",    p.price);
    q.bindValue(":stock",    p.stock);
    q.bindValue(":exp",      p.expiryDate);
    q.bindValue(":status",   p.status);
    q.bindValue(":supplier", p.supplier);
    q.bindValue(":sku",      p.sku);
    q.bindValue(":id",       p.id);

    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return false;
    }
    return true;
}

bool ProductStaff::deleteProduct(int id)
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

QList<StaffProductDTO> ProductStaff::fetchProducts(const QString &search,
                                                   const QString &category)
{
    QList<StaffProductDTO> list;

    QString sql =
        "SELECT id, product_name, category, unit, price, stock, "
        "       expiry_date, status, supplier, sku "
        "FROM products WHERE 1=1";

    if (!search.isEmpty())
        sql += " AND (product_name LIKE :search OR sku LIKE :search)";
    if (!category.isEmpty())
        sql += " AND category = :category";
    sql += " ORDER BY product_name ASC";

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
        StaffProductDTO p;
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

// ─────────────────────────────────────────────────────────────────
//  UI HELPERS
// ─────────────────────────────────────────────────────────────────
void ProductStaff::populateCategoryFilter()
{
    ui->cmbFilterCategory->blockSignals(true);
    ui->cmbFilterCategory->clear();
    ui->cmbFilterCategory->addItem("All Categories");
    for (const auto &c : loadCategoriesFromDb())
        ui->cmbFilterCategory->addItem(c);
    ui->cmbFilterCategory->blockSignals(false);
}

void ProductStaff::loadProducts(const QString &search, const QString &category)
{
    QTableWidget *tbl = ui->tblProducts;
    tbl->setUpdatesEnabled(false);
    tbl->setSortingEnabled(false);
    tbl->setRowCount(0);

    const QList<StaffProductDTO> products = fetchProducts(search, category);

    for (int row = 0; row < products.size(); ++row) {
        tbl->insertRow(row);
        setRowData(row, products[row]);
        addActionButtons(row, products[row].id);
    }

    tbl->setSortingEnabled(true);
    tbl->setUpdatesEnabled(true);
    updateStatusBar(products.size());
}

void ProductStaff::setRowData(int row, const StaffProductDTO &p)
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

void ProductStaff::addActionButtons(int row, int productId)
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
        "QPushButton{background:#E6F4EA;color:#1B5E3C;border:1px solid #1B5E3C;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#C8E6D0;}");
    btnDel->setStyleSheet(
        "QPushButton{background:#FDEDEC;color:#C0392B;border:1px solid #F5B7B1;"
        "border-radius:4px;padding:3px 10px;font-size:12px;}"
        "QPushButton:hover{background:#FADBD8;}");

    connect(btnEdit, &QPushButton::clicked, this, &ProductStaff::onEditProduct);
    connect(btnDel,  &QPushButton::clicked, this, &ProductStaff::onDeleteProduct);

    layout->addStretch();
    layout->addWidget(btnEdit);
    layout->addWidget(btnDel);
    layout->addStretch();

    ui->tblProducts->setCellWidget(row, 10, cell);
    ui->tblProducts->setRowHeight(row, 42);
}

void ProductStaff::updateStatusBar(int count)
{
    ui->lblTotalProducts->setText(
        QString("Total: %1 product%2").arg(count).arg(count == 1 ? "" : "s"));
    ui->lblStatusBar->setText(
        QString("Last refreshed: %1")
            .arg(QDateTime::currentDateTime().toString("dd-MMM-yyyy  hh:mm:ss")));
}

// ─────────────────────────────────────────────────────────────────
//  SLOTS
// ─────────────────────────────────────────────────────────────────
void ProductStaff::onAddProduct()
{
    ProductStaffDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const StaffProductDTO p = dlg.getProduct();
        if (saveProduct(p)) {
            ui->lblStatusBar->setText(
                QString("✔  '%1' added successfully.").arg(p.productName));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
    }
}

void ProductStaff::onSearchChanged(const QString &text)
{
    QString cat = ui->cmbFilterCategory->currentIndex() == 0
                      ? QString()
                      : ui->cmbFilterCategory->currentText();
    loadProducts(text.trimmed(), cat);
}

void ProductStaff::onFilterCategoryChanged(int)
{
    QString cat = ui->cmbFilterCategory->currentIndex() == 0
                      ? QString()
                      : ui->cmbFilterCategory->currentText();
    loadProducts(ui->txtSearch->text().trimmed(), cat);
}

void ProductStaff::onClearSearch()
{
    ui->txtSearch->clear();
    ui->cmbFilterCategory->setCurrentIndex(0);
    loadProducts();
}

void ProductStaff::onEditProduct()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    StaffProductDTO p = fetchById(id);
    if (p.id <= 0) {
        QMessageBox::warning(this, "Not Found", "Product record not found.");
        return;
    }

    ProductStaffDialog dlg(this, p);
    if (dlg.exec() == QDialog::Accepted) {
        const StaffProductDTO updated = dlg.getProduct();
        if (updateProduct(updated)) {
            ui->lblStatusBar->setText(
                QString("✔  '%1' updated successfully.").arg(updated.productName));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
    }
}

void ProductStaff::onDeleteProduct()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int id = btn->property("productId").toInt();
    StaffProductDTO p = fetchById(id);

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
        loadProducts(ui->txtSearch->text(),
                     ui->cmbFilterCategory->currentIndex() == 0
                         ? QString()
                         : ui->cmbFilterCategory->currentText());
    }
}

void ProductStaff::onTableDoubleClicked(int row, int)
{
    int id = idFromRow(ui->tblProducts, row);
    if (id <= 0) return;

    StaffProductDTO p = fetchById(id);
    if (p.id <= 0) return;

    ProductStaffDialog dlg(this, p);
    if (dlg.exec() == QDialog::Accepted) {
        const StaffProductDTO updated = dlg.getProduct();
        if (updateProduct(updated)) {
            ui->lblStatusBar->setText(
                QString("✔  '%1' updated.").arg(updated.productName));
            loadProducts(ui->txtSearch->text(),
                         ui->cmbFilterCategory->currentIndex() == 0
                             ? QString()
                             : ui->cmbFilterCategory->currentText());
        }
    }
}
