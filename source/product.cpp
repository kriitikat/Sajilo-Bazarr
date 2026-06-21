#include "../include/product.h"
#include "../ui/ui_product.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDate>


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


ProductDialog::ProductDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Product");
    setMinimumWidth(480);
    setModal(true);
    setupUi();
}

void ProductDialog::setupUi()
{
    auto *lblTitle = new QLabel("➕  Add New Product", this);
    lblTitle->setStyleSheet(
        "font-size:16px; font-weight:bold; color:#1A73E8; padding:6px 0;");

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#DDE3EC;");

    // SKU row
    txtSku = new QLineEdit(this);
    txtSku->setPlaceholderText("e.g. SB-FD-00123");
    txtSku->setMaxLength(30);

    btnGenSku = new QPushButton("⟳ Generate", this);
    btnGenSku->setFixedWidth(100);
    btnGenSku->setStyleSheet(
        "QPushButton{background:#E8F0FE;color:#1A73E8;"
        "border:1px solid #ADC6FF;border-radius:4px;padding:5px;}"
        "QPushButton:hover{background:#D2E3FC;}");
    connect(btnGenSku, &QPushButton::clicked,
            this, &ProductDialog::onGenerateSku);

    auto *skuRow = new QHBoxLayout;
    skuRow->addWidget(txtSku);
    skuRow->addWidget(btnGenSku);

    // Other fields
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

    // Form layout
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

    // Buttons
    btnSave   = new QPushButton("✔  Add Product", this);
    btnCancel = new QPushButton("✕  Cancel", this);

    btnSave->setStyleSheet(
        "QPushButton{background:#1A73E8;color:white;border:none;"
        "border-radius:6px;padding:8px 22px;font-weight:bold;}"
        "QPushButton:hover{background:#1558C0;}");
    btnCancel->setStyleSheet(
        "QPushButton{background:#E8EAED;color:#444;border:none;"
        "border-radius:6px;padding:8px 22px;}"
        "QPushButton:hover{background:#D2D5DB;}");

    connect(btnSave,   &QPushButton::clicked, this, &ProductDialog::onAccept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnSave);

    // Master layout
    auto *master = new QVBoxLayout(this);
    master->setSpacing(8);
    master->setContentsMargins(0, 12, 0, 12);
    master->addWidget(lblTitle, 0, Qt::AlignCenter);
    master->addWidget(separator);
    master->addLayout(form);
    master->addSpacing(8);
    master->addLayout(btnRow);

    setStyleSheet(R"(
        QDialog { background:#FFFFFF; }
        QLineEdit, QComboBox, QSpinBox, QDateEdit {
            background:#FFFFFF; border:1px solid #C8D0DC;
            border-radius:5px; padding:5px 8px; min-height:28px;
        }
        QLineEdit:focus, QComboBox:focus,
        QSpinBox:focus, QDateEdit:focus {
            border:1px solid #1A73E8;
        }
    )");

    // Auto-generate SKU on open
    onGenerateSku();
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
    if (txtPrice->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Price is required.");
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

Product::Product(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Product)
{
    ui->setupUi(this);

    // Column widths
    ui->tblProducts->setColumnWidth(0, 45);   // Id
    ui->tblProducts->setColumnWidth(1, 180);  // Name
    ui->tblProducts->setColumnWidth(2, 140);  // Category
    ui->tblProducts->setColumnWidth(3, 60);   // Unit
    ui->tblProducts->setColumnWidth(4, 90);   // Price
    ui->tblProducts->setColumnWidth(5, 60);   // Stock
    ui->tblProducts->setColumnWidth(6, 95);   // Expiry
    ui->tblProducts->setColumnWidth(7, 70);   // Status
    ui->tblProducts->setColumnWidth(8, 90);   // Supplier
    ui->tblProducts->setColumnWidth(9, 110);  // SKU
    ui->tblProducts->horizontalHeader()
        ->setSectionResizeMode(10, QHeaderView::Stretch); // Action

    ui->tblProducts->verticalHeader()->setVisible(false);

    connect(ui->btnAddProduct, &QPushButton::clicked,
            this, &Product::onAddProduct);
}

Product::~Product()
{
    delete ui;
}

void Product::onAddProduct()
{
    ProductDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        ProductDTO p = dlg.getProduct();
        p.id = m_nextId++;
        m_products.append(p);
        addRowToTable(p);
        ui->lblTotalProducts->setText(
            QString("Total: %1 product%2")
                .arg(m_products.size())
                .arg(m_products.size() == 1 ? "" : "s"));
        ui->lblStatusBar->setText(
            QString("✔  Product '%1' added.").arg(p.name));
    }
}

void Product::addRowToTable(const ProductDTO &p)
{
    QTableWidget *tbl = ui->tblProducts;
    int row = tbl->rowCount();
    tbl->insertRow(row);

    tbl->setItem(row, 0,  new QTableWidgetItem(QString::number(p.id)));
    tbl->setItem(row, 1,  new QTableWidgetItem(p.name));
    tbl->setItem(row, 2,  new QTableWidgetItem(p.category));
    tbl->setItem(row, 3,  new QTableWidgetItem(p.unit));
    tbl->setItem(row, 4,  new QTableWidgetItem(
                             QString("Rs. %1").arg(p.price, 0, 'f', 2)));
    tbl->setItem(row, 5,  new QTableWidgetItem(QString::number(p.stock)));
    tbl->setItem(row, 6,  new QTableWidgetItem(p.expiryDate));
    tbl->setItem(row, 7,  new QTableWidgetItem(p.status));
    tbl->setItem(row, 8,  new QTableWidgetItem("—"));
    tbl->setItem(row, 9,  new QTableWidgetItem(p.sku));
    tbl->setItem(row, 10, new QTableWidgetItem(""));

    tbl->setRowHeight(row, 38);
}