#include "../include/admindashboard.h"
#include "../include/ui_admindashboard.h"

#include "../include/category.h"
#include "../include/product.h"
#include "../include/inventory.h"
#include "../include/staff.h"
#include "../include/pending.h"
#include "../include/supplier.h"

#include <QPushButton>
#include <QMessageBox>

AdminDashboard::AdminDashboard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminDashboard)
{
    ui->setupUi(this);

    // Categories button
    connect(ui->btnCategories,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnCategories_clicked);

    // Products button
    connect(ui->btnProducts,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnProducts_clicked);

    // Inventory button
    connect(ui->btnInventory,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnInventory_clicked);

    // Staff button
    connect(ui->btnStaff,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnStaff_clicked);
    // suppliers button
    connect(ui->btnSuppliers,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnSuppliers_clicked);


    // Pending Request button
    // Qt converts spaces in object names to underscores, so the button named
    // "btnPending Request" in the .ui file becomes ui->btnPending_Request here.
    connect(ui->btnPending_Request,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnPending_Request_clicked);

    // Logout button
    connect(ui->btnLogout,
            &QPushButton::clicked,
            this,
            &AdminDashboard::on_btnLogout_clicked);
}

AdminDashboard::~AdminDashboard()
{
    delete ui;
}

void AdminDashboard::on_btnCategories_clicked()
{
    category *categoryPage = new category();
    categoryPage->show();
}

void AdminDashboard::on_btnProducts_clicked()
{
    Product *productPage = new Product();
    productPage->show();
}

void AdminDashboard::on_btnInventory_clicked()
{
    inventory *inventoryPage = new inventory();
    inventoryPage->show();
}

void AdminDashboard::on_btnStaff_clicked()
{
    staff *staffPage = new staff();
    staffPage->show();
}

void AdminDashboard::on_btnPending_Request_clicked()
{
    pending *pendingPage = new pending();
    pendingPage->setAttribute(Qt::WA_DeleteOnClose);
    pendingPage->show();
}
void AdminDashboard::on_btnSuppliers_clicked()
{
    supplier *suppliersPage = new supplier();
    suppliersPage->setAttribute(Qt::WA_DeleteOnClose);
    suppliersPage->show();
}

void AdminDashboard::on_btnLogout_clicked()
{
    const auto reply = QMessageBox::question(
        this, tr("Confirm Logout"),
        tr("Are you sure you want to log out?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    Login *loginPage = new Login();
    loginPage->setAttribute(Qt::WA_DeleteOnClose);
    loginPage->show();

    this->close();
}