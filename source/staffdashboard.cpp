// staffdashboard.cpp
#include "../include/staffdashboard.h"
#include "../ui/ui_staffdashboard.h"
#include "../include/productstaff.h"
#include <QPushButton>

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


}

StaffDashboard::~StaffDashboard()
{
    delete ui;
}

void StaffDashboard::handleProductsClicked()
{
    ProductStaff *productPage = new ProductStaff();
    productPage->show();
}