#include "../include/admindashboard.h"
#include "../ui/ui_admindashboard.h"
#include "../include/category.h"
#include "../include/product.h"
#include "../include/inventory.h"
#include "../include/staff.h"
#include "../include/pending.h"
#include "../include/supplier.h"
#include "../include/login.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDate>
#include <QMap>
#include <QDebug>

#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

AdminDashboard::AdminDashboard(QWidget *parent)
    : ClassLogout(parent)
    , ui(new Ui::AdminDashboard)
{
    ui->setupUi(this);

    connect(ui->btnCategories, &QPushButton::clicked, this, &AdminDashboard::handleCategories_clicked);
    connect(ui->btnProducts, &QPushButton::clicked, this, &AdminDashboard::handleProducts_clicked);
    connect(ui->btnInventory, &QPushButton::clicked, this, &AdminDashboard::handleInventory_clicked);
    connect(ui->btnStaff, &QPushButton::clicked, this, &AdminDashboard::handleStaff_clicked);
    connect(ui->btnSuppliers, &QPushButton::clicked, this, &AdminDashboard::handleSuppliers_clicked);
    connect(ui->btnPending_Request, &QPushButton::clicked, this, &AdminDashboard::handlePending_Request_clicked);
    connect(ui->btnLogout,
            &QPushButton::clicked,
            this,
            &AdminDashboard::handleLogout_clicked);
    setupReportChart();
    setupWeeklySalesChart();
    setupMonthlySalesChart();
}

AdminDashboard::~AdminDashboard()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────
//  DATA FETCHING
// ─────────────────────────────────────────────────────────────────

ReportData AdminDashboard::fetchReportData()
{
    ReportData data;

    // --- Revenue: total of everything sold ---
    QSqlQuery revenueQuery;
    revenueQuery.prepare(
        "SELECT COALESCE(SUM(quantity_sold * unit_price), 0) FROM sales"
        );
    if (revenueQuery.exec() && revenueQuery.next()) {
        data.revenue = revenueQuery.value(0).toDouble();
    } else {
        qWarning() << "Revenue query failed:" << revenueQuery.lastError().text();
    }

    // --- Profit: Revenue minus real Cost of Goods Sold ---
    // Cost = for every sale, quantity_sold * that product's buying_price
    QSqlQuery costQuery;
    costQuery.prepare(
        "SELECT COALESCE(SUM(s.quantity_sold * p.buying_price), 0) "
        "FROM sales s "
        "JOIN products p ON p.id = s.product_id"
        );
    double cost = 0.0;
    if (costQuery.exec() && costQuery.next()) {
        cost = costQuery.value(0).toDouble();
    } else {
        qWarning() << "Cost query failed:" << costQuery.lastError().text();
    }
    data.profit = data.revenue - cost;

    // --- Loss: value of unsold stock that has already expired ---
    QSqlQuery lossQuery;
    lossQuery.prepare(
        "SELECT COALESCE(SUM(p.stock * p.buying_price), 0) "
        "FROM products p "
        "WHERE p.expiry_date IS NOT NULL AND p.expiry_date <> '' "
        "AND date(p.expiry_date) < date('now')"
        );
    if (lossQuery.exec() && lossQuery.next()) {
        data.loss = lossQuery.value(0).toDouble();
    } else {
        qWarning() << "Loss query failed:" << lossQuery.lastError().text();
    }

    return data;
}

QVector<QPair<QString, double>> AdminDashboard::fetchWeeklySales()
{
    QVector<QPair<QString, double>> result;

    QSqlQuery q;
    q.prepare(
        "SELECT date(sale_date) AS d, COALESCE(SUM(quantity_sold * unit_price), 0) "
        "FROM sales "
        "WHERE date(sale_date) >= date('now', '-6 days') "
        "GROUP BY d ORDER BY d"
        );
    if (!q.exec()) {
        qWarning() << "Weekly sales query failed:" << q.lastError().text();
        return result;
    }

    QMap<QString, double> byDate;
    while (q.next())
        byDate[q.value(0).toString()] = q.value(1).toDouble();

    for (int i = 6; i >= 0; --i) {
        QDate d = QDate::currentDate().addDays(-i);
        result.append({ d.toString("ddd"), byDate.value(d.toString("yyyy-MM-dd"), 0.0) });
    }
    return result;
}

QVector<QPair<QString, double>> AdminDashboard::fetchMonthlySales()
{
    QVector<QPair<QString, double>> result;

    QSqlQuery q;
    q.prepare(
        "SELECT strftime('%Y-%m', sale_date) AS m, COALESCE(SUM(quantity_sold * unit_price), 0) "
        "FROM sales "
        "WHERE date(sale_date) >= date('now', 'start of month', '-5 months') "
        "GROUP BY m ORDER BY m"
        );
    if (!q.exec()) {
        qWarning() << "Monthly sales query failed:" << q.lastError().text();
        return result;
    }

    QMap<QString, double> byMonth;
    while (q.next())
        byMonth[q.value(0).toString()] = q.value(1).toDouble();

    for (int i = 5; i >= 0; --i) {
        QDate d = QDate::currentDate().addMonths(-i);
        result.append({ d.toString("MMM"), byMonth.value(d.toString("yyyy-MM"), 0.0) });
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────
//  CHART BUILDERS
// ─────────────────────────────────────────────────────────────────

void AdminDashboard::setupReportChart()
{
    ReportData data = fetchReportData();

    QPieSeries *series = new QPieSeries();
    QPieSlice *revenueSlice = series->append("Revenue", data.revenue);
    QPieSlice *profitSlice  = series->append("Profit", data.profit);
    QPieSlice *lossSlice    = series->append("Loss", data.loss);

    revenueSlice->setBrush(QColor("#4A9EE8")); // blue
    profitSlice->setBrush(QColor("#2ECC71"));  // green
    lossSlice->setBrush(QColor("#E74C3C"));    // red

    for (auto *slice : series->slices())
        slice->setLabelVisible(false);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Sales Report Overview");
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setVisible(true);

    const QList<QLegendMarker *> markers = chart->legend()->markers(series);
    markers[0]->setLabel(QString("Revenue: Rs. %1").arg(data.revenue, 0, 'f', 0));
    markers[1]->setLabel(QString("Profit: Rs. %1").arg(data.profit, 0, 'f', 0));
    markers[2]->setLabel(QString("Loss: Rs. %1").arg(data.loss, 0, 'f', 0));

    reportChartView = new QChartView(chart);
    reportChartView->setRenderHint(QPainter::Antialiasing);
    reportChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->reportChartLayout->addWidget(reportChartView);
}

void AdminDashboard::setupWeeklySalesChart()
{
    auto data = fetchWeeklySales();

    QLineSeries *series = new QLineSeries();
    series->setName("Sales (Rs.)");

    QStringList categories;
    for (int i = 0; i < data.size(); ++i) {
        series->append(i, data[i].second);
        categories << data[i].first;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Weekly Sales");
    chart->legend()->hide();

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("Rs. %d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    weeklyChartView = new QChartView(chart);
    weeklyChartView->setRenderHint(QPainter::Antialiasing);
    weeklyChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->weeklyChartLayout->addWidget(weeklyChartView);
}

void AdminDashboard::setupMonthlySalesChart()
{
    auto data = fetchMonthlySales();

    QBarSet *set = new QBarSet("Sales (Rs.)");
    QStringList categories;
    for (auto &pair : data) {
        *set << pair.second;
        categories << pair.first;
    }

    QBarSeries *series = new QBarSeries();
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Monthly Sales");
    chart->legend()->hide();

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("Rs. %d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    monthlyChartView = new QChartView(chart);
    monthlyChartView->setRenderHint(QPainter::Antialiasing);
    monthlyChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->monthlyChartLayout->addWidget(monthlyChartView);
}

// ─────────────────────────────────────────────────────────────────
//  NAVIGATION SLOTS
// ─────────────────────────────────────────────────────────────────

void AdminDashboard::handleCategories_clicked()
{
    category *categoryPage = new category();
    categoryPage->show();
}

void AdminDashboard::handleProducts_clicked()
{
    Product *productPage = new Product();
    productPage->show();
}

void AdminDashboard::handleInventory_clicked()
{
    if (!inventoryPage) {
        inventoryPage = new inventory();
    }
    inventoryPage->show();
    inventoryPage->raise();
    inventoryPage->activateWindow();
}

void AdminDashboard::handleStaff_clicked()
{
    staff *staffPage = new staff();
    staffPage->show();
}

void AdminDashboard::handlePending_Request_clicked()
{
    pending *pendingPage = new pending();
    pendingPage->setAttribute(Qt::WA_DeleteOnClose);
    pendingPage->show();
}

void AdminDashboard::handleSuppliers_clicked()
{
    supplier *suppliersPage = new supplier();
    suppliersPage->setAttribute(Qt::WA_DeleteOnClose);
    suppliersPage->show();
}
