#include "../include/frontdesk.h"
#include "../ui/ui_frontdesk.h"
#include "../include/billing.h"
#include "../include/frontproduct.h"
#include "../include/frontprofile.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QDebug>

frontdesk::frontdesk(int staffId, const QString &staffName, QWidget *parent)
    : ClassLogout(parent)
    , ui(new Ui::frontdesk)
    , m_staffId(staffId)
    , m_staffName(staffName)
{
    ui->setupUi(this);

    // Both the sidebar "Billing" button and the dashboard shortcut button
    // open the same standalone Billing window.
    connect(ui->btnBilling,     &QPushButton::clicked, this, &frontdesk::openBillingWindow);
    connect(ui->btnViewProducts, &QPushButton::clicked, this, &frontdesk::openViewProductsWindow);

    // Profile avatar (top-right) -> opens FrontProfile
    connect(ui->btnProfile,
            &QPushButton::clicked,
            this,
            &frontdesk::handleProfileClicked);

    // Logout button
    connect(ui->btnLogout,
            &QPushButton::clicked,
            this,
            &frontdesk::handleLogout_clicked);

    setupProductTable();
    loadCurrentUserInfo();
    loadDashboardStats();
}

frontdesk::~frontdesk()
{
    delete ui;
}

void frontdesk::openViewProductsWindow()
{
    // View-only product catalog for front desk staff: same page pattern
    // as Product (admin) / ProductStaff, but no add/edit/delete/stock
    // actions and no Action column at all.
    FrontProduct *viewProductsPage = new FrontProduct();
    viewProductsPage->setAttribute(Qt::WA_DeleteOnClose);
    viewProductsPage->show();
}

void frontdesk::openBillingWindow()
{
    // Billing is a separate QMainWindow (billing.cpp/.h/.ui). It does NOT
    // show itself in its own constructor - the caller is responsible for
    // that, same convention used elsewhere in this app (e.g. Login opening
    // frontdesk). We show the new window, then close this one, so only one
    // window is ever visible at a time.
    Billing *billingWindow = new Billing();
    billingWindow->setAttribute(Qt::WA_DeleteOnClose);
    billingWindow->show();
    this->close();
}

// ─────────────────────────────────────────────────────────────────
//  Pulls this front desk user's username/first_name/last_name/picture
//  from 'information' BY ID (m_staffId), and applies them to the
//  topbar (welcome text + circular avatar button) — exact same logic
//  as StaffDashboard::loadCurrentUserInfo(). Falls back to an
//  initials avatar if no photo file can be found. m_username is
//  cached here purely so handleProfileClicked() has something to
//  hand to FrontProfile — FrontProfile itself still looks everything
//  else up by username internally.
// ─────────────────────────────────────────────────────────────────
void frontdesk::loadCurrentUserInfo()
{
    if (m_staffId < 0) {
        qWarning() << "frontdesk: no staffId supplied, cannot personalize dashboard";
        ui->welcomeLabel->setText(m_staffName.isEmpty()
                                       ? QStringLiteral("Welcome back, Front Desk!")
                                       : QStringLiteral("Welcome back, %1!").arg(m_staffName));
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT first_name, last_name, username, picture "
              "FROM information WHERE id = :id");
    q.bindValue(":id", m_staffId);

    QString firstName;
    QString lastName;
    QString picture;

    if (q.exec() && q.next()) {
        firstName  = q.value(0).toString();
        lastName   = q.value(1).toString();
        m_username = q.value(2).toString();
        picture    = q.value(3).toString();

        // Keep m_staffName authoritative for anything shown after this —
        // DB values are fresher than whatever Login captured at sign-in
        // time.
        const QString freshFullName = QStringLiteral("%1 %2").arg(firstName, lastName).trimmed();
        if (!freshFullName.isEmpty())
            m_staffName = freshFullName;
    } else {
        qWarning() << "frontdesk: could not load user info for staffId"
                   << m_staffId << "-" << q.lastError().text();
    }

    ui->welcomeLabel->setText(
        firstName.isEmpty() ? QStringLiteral("Welcome back, %1!").arg(
                                   m_staffName.isEmpty() ? QStringLiteral("Front Desk") : m_staffName)
                             : QStringLiteral("Welcome back, %1!").arg(firstName));

    // ── Avatar ───────────────────────────────────────────────────
    QPixmap avatar;
    bool loaded = false;

    if (!picture.isEmpty()) {
        // Must match Profile/FrontProfile/Register's copyPictureToStorage()
        // save location exactly -- this used to read from
        // applicationDirPath()-relative resources/staff_photos/, which is
        // wherever the .exe happens to run from and doesn't match the
        // fixed save path below, so newly saved/approved photos never
        // showed up in the topbar icon even though the file was on disk.
        const QString diskPath =
            QStringLiteral("C:/Users/ASUS/Desktop/Sajilo-Bazarr/resources/staff_photos/") + picture;

        if (QFile::exists(diskPath))
            loaded = avatar.load(diskPath);

        if (!loaded)
            loaded = avatar.load(":/staff_photos/" + picture); // Qt resource fallback
    }

    if (!loaded) {
        // No photo available -> draw a simple initials avatar instead.
        QString initials;
        if (!firstName.isEmpty()) initials += firstName.at(0).toUpper();
        if (!lastName.isEmpty())  initials += lastName.at(0).toUpper();
        if (initials.isEmpty())   initials = "?";

        avatar = QPixmap(AVATAR_DIAMETER, AVATAR_DIAMETER);
        avatar.fill(Qt::transparent);

        QPainter p(&avatar);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#660033"));
        p.drawEllipse(0, 0, AVATAR_DIAMETER, AVATAR_DIAMETER);

        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(AVATAR_DIAMETER / 2);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(avatar.rect(), Qt::AlignCenter, initials);
    }

    ui->btnProfile->setIcon(QIcon(makeCircularPixmap(avatar, AVATAR_DIAMETER)));
    ui->btnProfile->setIconSize(QSize(AVATAR_DIAMETER, AVATAR_DIAMETER));
}

QPixmap frontdesk::makeCircularPixmap(const QPixmap &source, int diameter)
{
    QPixmap scaled = source.scaled(diameter, diameter,
                                    Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation);

    QPixmap circular(diameter, diameter);
    circular.fill(Qt::transparent);

    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath clipPath;
    clipPath.addEllipse(0, 0, diameter, diameter);
    painter.setClipPath(clipPath);

    const int x = (diameter - scaled.width())  / 2;
    const int y = (diameter - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    return circular;
}

// ─────────────────────────────────────────────────────────────────
//  Opens FrontProfile for the currently logged-in front desk user —
//  same pattern as StaffDashboard::handleProfileClicked().
// ─────────────────────────────────────────────────────────────────
void frontdesk::handleProfileClicked()
{
    FrontProfile *profileWindow = new FrontProfile(m_username, this);
    profileWindow->setAttribute(Qt::WA_DeleteOnClose);
    profileWindow->exec();
}

// ---------------------------------------------------------------------
// Builds a single small stat card (title + big value) as a QFrame.
// Positioned with absolute geometry to match the rest of this UI's style.
// ---------------------------------------------------------------------
QFrame *frontdesk::createStatCard(QWidget *parent, int x, int y, int w, int h,
                                  const QString &title, const QString &value,
                                  const QString &accentColor)
{
    QFrame *card = new QFrame(parent);
    card->setGeometry(x, y, w, h);
    card->setStyleSheet(QString(
                            "QFrame { background-color: #ffffff; border: 1px solid #e5e7eb; "
                            "border-left: 4px solid %1; border-radius: 4px; }"
                            ).arg(accentColor));

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setGeometry(16, 14, w - 28, 20);
    titleLabel->setStyleSheet(
        "QLabel { color: #6b7280; font-size: 12px; font-weight: 500; background: transparent; border: none; }");
    titleLabel->setWordWrap(true);

    QLabel *valueLabel = new QLabel(value, card);
    valueLabel->setObjectName("statValueLabel");
    valueLabel->setGeometry(16, 38, w - 28, 32);
    valueLabel->setStyleSheet(
        "QLabel { color: #1a2a4a; font-size: 24px; font-weight: 700; background: transparent; border: none; }");

    card->show();
    return card;
}

// ---------------------------------------------------------------------
// Creates the 4 stat cards in the gap between the search bar and the
// product table, then queries the DB and fills in real numbers.
// ---------------------------------------------------------------------
void frontdesk::loadDashboardStats()
{
    // Card row geometry: aligned with searchBar (x=20, width=760)
    const int rowY   = 98;
    const int rowH   = 76;
    const int gap    = 16;   // more breathing room between cards
    const int totalW = 760;
    const int cardW  = (totalW - 3 * gap) / 4;

    int x1 = 20;
    int x2 = x1 + cardW + gap;
    int x3 = x2 + cardW + gap;
    int x4 = x3 + cardW + gap;
    int lastW = 20 + totalW - x4; // absorbs rounding so the row lines up with searchBar

    QFrame *cardItems    = createStatCard(ui->mainFrame, x1, rowY, cardW, rowH,
                                       "Items Sold Today", "0", "#3b82f6");
    QFrame *cardSales    = createStatCard(ui->mainFrame, x2, rowY, cardW, rowH,
                                       "Sales Total Today", "Rs. 0", "#f59e0b");
    QFrame *cardProducts = createStatCard(ui->mainFrame, x3, rowY, cardW, rowH,
                                          "Unique Products Sold", "0", "#8b5cf6");
    QFrame *cardLowStock = createStatCard(ui->mainFrame, x4, rowY, lastW, rowH,
                                          "Low Stock Items", "0", "#ef4444");

    lblItemsSoldValue    = cardItems->findChild<QLabel *>("statValueLabel");
    lblSalesTotalValue   = cardSales->findChild<QLabel *>("statValueLabel");
    lblProductsSoldValue = cardProducts->findChild<QLabel *>("statValueLabel");
    lblLowStockValue     = cardLowStock->findChild<QLabel *>("statValueLabel");

    const QString today = QDate::currentDate().toString("yyyy-MM-dd");

    // --- Items sold today + sales total today ---
    {
        QSqlQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(quantity_sold), 0), "
            "       COALESCE(SUM(quantity_sold * unit_price), 0), "
            "       COUNT(DISTINCT product_id) "
            "FROM sales "
            "WHERE date(sale_date) = date(:today)");
        q.bindValue(":today", today);

        if (q.exec() && q.next()) {
            double itemsSold   = q.value(0).toDouble();
            double salesTotal  = q.value(1).toDouble();
            int    productsSold = q.value(2).toInt();

            if (lblItemsSoldValue)
                lblItemsSoldValue->setText(QString::number(itemsSold, 'f', 0));
            if (lblSalesTotalValue)
                lblSalesTotalValue->setText(QString("Rs. %1").arg(salesTotal, 0, 'f', 2));
            if (lblProductsSoldValue)
                lblProductsSoldValue->setText(QString::number(productsSold));
        } else {
            qDebug() << "Dashboard: sales stats query failed:" << q.lastError().text();
        }
    }

    // --- Low stock count ---
    {
        QSqlQuery q;
        q.prepare("SELECT COUNT(*) FROM products WHERE stock < :threshold");
        q.bindValue(":threshold", LOW_STOCK_THRESHOLD);

        if (q.exec() && q.next()) {
            if (lblLowStockValue)
                lblLowStockValue->setText(QString::number(q.value(0).toInt()));
        } else {
            qDebug() << "Dashboard: low stock query failed:" << q.lastError().text();
        }
    }

    loadRecentSales();
}

// ---------------------------------------------------------------------
// One-time setup of productTable's columns/behavior. We're repurposing
// this widget (already positioned perfectly in the .ui) to show the
// most recent sales, since front desk can't edit inventory anyway.
// ---------------------------------------------------------------------
void frontdesk::setupProductTable()
{
    ui->productTable->setColumnCount(4);
    ui->productTable->setHorizontalHeaderLabels(
        {"Product", "Qty", "Amount", "Date / Time"});
    ui->productTable->horizontalHeader()->setStretchLastSection(true);
    ui->productTable->verticalHeader()->setVisible(false);
    ui->productTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->productTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->productTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->productTable->setAlternatingRowColors(true);
}

// ---------------------------------------------------------------------
// Fills productTable with the 10 most recent sales, newest first.
// ---------------------------------------------------------------------
void frontdesk::loadRecentSales()
{
    QSqlQuery q;
    q.prepare(
        "SELECT product_name, quantity_sold, (quantity_sold * unit_price), sale_date "
        "FROM sales "
        "ORDER BY sale_date DESC "
        "LIMIT 10");

    if (!q.exec()) {
        qDebug() << "Dashboard: recent sales query failed:" << q.lastError().text();
        ui->productTable->setRowCount(0);
        return;
    }

    ui->productTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        ui->productTable->insertRow(row);

        QString productName = q.value(0).toString();
        double  qty          = q.value(1).toDouble();
        double  amount       = q.value(2).toDouble();
        QString saleDate     = q.value(3).toString();

        ui->productTable->setItem(row, 0, new QTableWidgetItem(productName));
        ui->productTable->setItem(row, 1, new QTableWidgetItem(QString::number(qty, 'f', 0)));
        ui->productTable->setItem(row, 2, new QTableWidgetItem(QString("Rs. %1").arg(amount, 0, 'f', 2)));
        ui->productTable->setItem(row, 3, new QTableWidgetItem(saleDate));

        ++row;
    }

    if (row == 0) {
        ui->productTable->setRowCount(1);
        QTableWidgetItem *empty = new QTableWidgetItem("No sales recorded yet");
        ui->productTable->setItem(0, 0, empty);
        ui->productTable->setSpan(0, 0, 1, 4);
    }
}