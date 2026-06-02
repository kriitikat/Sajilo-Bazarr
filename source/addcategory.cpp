#include "addcategory.h"
#include "ui_addcategory.h"

addcategory::addcategory(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::addcategory)
{
    ui->setupUi(this);

    // Bulletproof manual connections for the popup action buttons
    connect(ui->addButton, &QPushButton::clicked, this, &addcategory::on_addButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &addcategory::on_cancelButton_clicked);
}

addcategory::~addcategory()
{
    delete ui;
}

QString addcategory::getCategoryName() const {
    return ui->categoryNameInput->text();
}

QString addcategory::getDescription() const {
    return ui->descriptionInput->toPlainText();
}

void addcategory::on_addButton_clicked()
{
    accept(); // Returns QDialog::Accepted to the main application loops
}

void addcategory::on_cancelButton_clicked()
{
    reject(); // Returns QDialog::Rejected and dismisses the window
}