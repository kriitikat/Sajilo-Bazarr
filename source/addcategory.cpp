#include "../include/addcategory.h"
#include "../ui/ui_addcategory.h"

#include <QMessageBox>
#include <QLineEdit>

addcategory::addcategory(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::addcategory)
{
    ui->setupUi(this);

    // Bulletproof manual connections for the popup action buttons
    connect(ui->addButton, &QPushButton::clicked, this, &addcategory::on_addButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &addcategory::on_cancelButton_clicked);

    // Keep "Add" disabled until a non-blank category name is typed, so a
    // blank/whitespace-only category can never be submitted in the first
    // place (it would otherwise show up as an empty entry everywhere
    // categories are listed, including the Product page's dropdown).
    connect(ui->categoryNameInput, &QLineEdit::textChanged,
            this, &addcategory::updateAddButtonState);
    updateAddButtonState();
}

addcategory::~addcategory()
{
    delete ui;
}

QString addcategory::getCategoryName() const {
    return ui->categoryNameInput->text().trimmed();
}

QString addcategory::getDescription() const {
    return ui->descriptionInput->toPlainText().trimmed();
}

void addcategory::updateAddButtonState()
{
    ui->addButton->setEnabled(!ui->categoryNameInput->text().trimmed().isEmpty());
}

void addcategory::on_addButton_clicked()
{
    // Defensive re-check — covers cases like pasting only spaces, or the
    // text being set programmatically without going through textChanged.
    if (getCategoryName().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Category name cannot be empty.");
        ui->categoryNameInput->setFocus();
        return;
    }

    accept(); // Returns QDialog::Accepted to the main application loop
}

void addcategory::on_cancelButton_clicked()
{
    reject(); // Returns QDialog::Rejected and dismisses the window
}