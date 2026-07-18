#include "../include/staffdashboard.h"
#include "../ui/ui_staffdashboard.h"
#include "../include/productstaff.h"

#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

StaffDashboard::StaffDashboard(QWidget *parent)
    : ClassLogout(parent)
    , ui(new Ui::StaffDashboard)
{
    ui->setupUi(this);

    // Products button
    connect(ui->btnProducts,
            &QPushButton::clicked,
            this,
            &StaffDashboard::handleProductsClicked);

    // Logout button
    connect(ui->btnLogout,
            &QPushButton::clicked,
            this,
            &StaffDashboard::handleLogout_clicked);

    // Re-check stats whenever staff clicks back into Dashboard
    connect(ui->btnDashboard,
            &QPushButton::clicked,
            this,
            &StaffDashboard::refreshDashboardStats);

    setupDashboardContent();
    refreshDashboardStats();
}

StaffDashboard::~StaffDashboard()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  Builds the stat cards + low-stock table inside contentFrame's
//  existing QVBoxLayout. Called once from the constructor.
// ─────────────────────────────────────────────────────────────────
void StaffDashboard::setupDashboardContent()
{
    QVBoxLayout *contentLayout =
        qobject_cast<QVBoxLayout *>(ui->contentFrame->layout());
    if (!contentLayout) {
        qWarning() << "StaffDashboard: contentFrame has no QVBoxLayout!";
        return;
    }

    // ── Row of stat cards ───────────────────────────────────────
    auto *cardsRow = new QGridLayout();
    cardsRow->setSpacing(16);

    auto makeCard = [this](const QString &title, QLabel *&valueLabelOut,
                           const QString &accentColor) -> QFrame* {
        auto *card = new QFrame;
        card->setStyleSheet(QString(
                                "QFrame {"
                                "  background-color: white;"
                                "  border-radius: 8px;"
                                "  border-left: 4px solid %1;"
                                "}").arg(accentColor));
        card->setMinimumHeight(90);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);

        auto *titleLbl = new QLabel(title);
        titleLbl->setStyleSheet("color: #6B7280; font-size: 12px; border: none;");

        valueLabelOut = new QLabel("0");
        valueLabelOut->setStyleSheet(
            "color: #1B2A4A; font-size: 24px; font-weight: bold; border: none;");

        cardLayout->addWidget(titleLbl);
        cardLayout->addWidget(valueLabelOut);
        return card;
    };

    QFrame *cardTotal   = makeCard("Total Products",  lblTotalProducts,   "#4A9EE8");
    QFrame *cardLow     = makeCard("Low Stock",        lblLowStockCount,   "#E8A94A");
    QFrame *cardOut     = makeCard("Out of Stock",     lblOutOfStockCount, "#E85D4A");
    QFrame *cardTasks   = makeCard("Pending Tasks",    lblPendingTasks,    "#8A4AE8");

    cardsRow->addWidget(cardTotal, 0, 0);
    cardsRow->addWidget(cardLow,   0, 1);
    cardsRow->addWidget(cardOut,   0, 2);
    cardsRow->addWidget(cardTasks, 0, 3);

    contentLayout->addLayout(cardsRow);

    // ── Low stock table heading ─────────────────────────────────
    auto *lowStockHeading = new QLabel("Low Stock Items");
    lowStockHeading->setStyleSheet(
        "color: #1B2A4A; font-size: 14px; font-weight: bold; margin-top: 10px;");
    contentLayout->addWidget(lowStockHeading);

    // ── Low stock table ──────────────────────────────────────────
    tblLowStock = new QTableWidget;
    tblLowStock->setColumnCount(5);
    tblLowStock->setHorizontalHeaderLabels(
        {"Product", "Category", "Stock", "Unit", "Status"});
    tblLowStock->verticalHeader()->setVisible(false);
    tblLowStock->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblLowStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    tblLowStock->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tblLowStock->setColumnWidth(1, 120);
    tblLowStock->setColumnWidth(2, 70);
    tblLowStock->setColumnWidth(3, 70);
    tblLowStock->setColumnWidth(4, 110);
    tblLowStock->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 6px; }");

    contentLayout->addWidget(tblLowStock);
    contentLayout->addStretch();
}

// ─────────────────────────────────────────────────────────────────
//  Pulls fresh numbers from the DB and repopulates the dashboard.
//  Call this any time stock might have changed (e.g. after coming
//  back from the Update Stock page).
// ─────────────────────────────────────────────────────────────────
void StaffDashboard::refreshDashboardStats()
{
    // Total products
    {
        QSqlQuery q("SELECT COUNT(*) FROM products");
        if (q.next())
            lblTotalProducts->setText(QString::number(q.value(0).toInt()));
    }

    // Low stock (stock > 0 and <= threshold)
    {
        QSqlQuery q;
        q.prepare("SELECT COUNT(*) FROM products WHERE stock > 0 AND stock <= :t");
        q.bindValue(":t", LOW_STOCK_THRESHOLD);
        if (q.exec() && q.next())
            lblLowStockCount->setText(QString::number(q.value(0).toInt()));
    }

    // Out of stock
    {
        QSqlQuery q("SELECT COUNT(*) FROM products WHERE stock <= 0");
        if (q.next())
            lblOutOfStockCount->setText(QString::number(q.value(0).toInt()));
    }

    // TODO: replace with a real query once taskmanagement.h schema is shared
    lblPendingTasks->setText("0");

    // Populate low-stock table
    tblLowStock->setRowCount(0);

    QSqlQuery q;
    q.prepare(
        "SELECT product_name, category, stock, unit, status "
        "FROM products WHERE stock <= :t ORDER BY stock ASC");
    q.bindValue(":t", LOW_STOCK_THRESHOLD);

    if (!q.exec()) {
        qWarning() << "StaffDashboard: low stock query failed -"
                   << q.lastError().text();
        return;
    }

    int row = 0;
    while (q.next()) {
        tblLowStock->insertRow(row);

        auto *itemName = new QTableWidgetItem(q.value(0).toString());
        auto *itemCat  = new QTableWidgetItem(q.value(1).toString());

        int stock = q.value(2).toInt();
        auto *itemStock = new QTableWidgetItem(QString::number(stock));
        itemStock->setTextAlignment(Qt::AlignCenter);
        if (stock <= 0) {
            itemStock->setForeground(QColor("#C0392B"));
        } else {
            itemStock->setForeground(QColor("#E8A94A"));
        }

        auto *itemUnit = new QTableWidgetItem(q.value(3).toString());
        itemUnit->setTextAlignment(Qt::AlignCenter);

        auto *itemStatus = new QTableWidgetItem(q.value(4).toString());
        itemStatus->setTextAlignment(Qt::AlignCenter);
        if (stock <= 0) {
            itemStatus->setForeground(QColor("#C0392B"));
            itemStatus->setBackground(QColor("#FDEDEC"));
        } else {
            itemStatus->setForeground(QColor("#856404"));
            itemStatus->setBackground(QColor("#FFF3CD"));
        }

        tblLowStock->setItem(row, 0, itemName);
        tblLowStock->setItem(row, 1, itemCat);
        tblLowStock->setItem(row, 2, itemStock);
        tblLowStock->setItem(row, 3, itemUnit);
        tblLowStock->setItem(row, 4, itemStatus);
        ++row;
    }
}

void StaffDashboard::handleProductsClicked()
{
    ProductStaff *productPage = new ProductStaff();
    productPage->show();
}