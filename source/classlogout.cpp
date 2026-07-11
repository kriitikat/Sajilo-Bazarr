#include "../include/classlogout.h"
#include "../include/login.h"

#include <QMessageBox>

ClassLogout::ClassLogout(QWidget *parent)
    : QMainWindow(parent)
{
}

void ClassLogout::handleLogout_clicked()
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