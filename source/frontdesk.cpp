#include "../include/frontdesk.h"
#include "../ui/ui_frontdesk.h"
#include "../include/billing.h"

frontdesk::frontdesk(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::frontdesk)
{
    ui->setupUi(this);

    // Both the sidebar "Billing" button and the dashboard shortcut button
    // open the same standalone Billing window.
    connect(ui->btnBilling,     &QPushButton::clicked, this, &frontdesk::openBillingWindow);
    connect(ui->btnBillingMain, &QPushButton::clicked, this, &frontdesk::openBillingWindow);
}

frontdesk::~frontdesk()
{
    delete ui;
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