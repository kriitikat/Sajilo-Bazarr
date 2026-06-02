#include "category.h"
#include "ui_category.h"
#include "addcategory.h"
#include <QDebug>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox> // Added to support confirmation boxes

category::category(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::category)
{
    ui->setupUi(this);

    // 1. Force layout to use Left-to-Right text direction
    ui->categoryTableWidget->setLayoutDirection(Qt::LeftToRight);

    // 2. Set exactly 4 columns to fit ID, Name, Description, and Actions
    ui->categoryTableWidget->setColumnCount(4);

    // 3. Establish the header names from left to right explicitly
    QStringList headers;
    headers << "ID" << "Category Name" << "Description" << "Actions";
    ui->categoryTableWidget->setHorizontalHeaderLabels(headers);

    // 4. Force columns to stretch out smoothly across the screen width
    ui->categoryTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 5. SLIM ROWS FIX: Enforce a strict, clean default height for all rows
    ui->categoryTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->categoryTableWidget->verticalHeader()->setDefaultSectionSize(38); // Compact 38px row height
    ui->categoryTableWidget->verticalHeader()->setVisible(false); // Hides default line index numbers

    connect(ui->addCategoryBtn, &QPushButton::clicked, this, &category::on_addCategoryBtn_clicked);
}

category::~category()
{
    delete ui;
}

void category::on_addCategoryBtn_clicked()
{
    addcategory *popup = new addcategory(this);
    popup->setWindowModality(Qt::WindowModal);

    if (popup->exec() == QDialog::Accepted) {
        QString name = popup->getCategoryName();
        QString desc = popup->getDescription();

        int currentRowCount = ui->categoryTableWidget->rowCount();
        ui->categoryTableWidget->insertRow(currentRowCount);

        // =============================================================
        // 4-COLUMN DATA PLACEMENT MAP (LEFT-TO-RIGHT)
        // =============================================================

        // Column 0: Auto-incrementing ID based on the row number (1, 2, 3...)
        QString idString = QString::number(currentRowCount + 1);
        QTableWidgetItem *idItem = new QTableWidgetItem(idString);
        idItem->setTextAlignment(Qt::AlignCenter); // Centers the ID text nicely
        ui->categoryTableWidget->setItem(currentRowCount, 0, idItem);

        // Column 1: Category Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        ui->categoryTableWidget->setItem(currentRowCount, 1, nameItem);

        // Column 2: Description
        QTableWidgetItem *descItem = new QTableWidgetItem(desc);
        ui->categoryTableWidget->setItem(currentRowCount, 2, descItem);

        // Column 3: Actions Container (Compact Delete Button)
        QWidget *actionContainer = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(actionContainer);
        layout->setContentsMargins(10, 2, 10, 2); // Keeps padding tight to keep row slim
        layout->setSpacing(0);

        QPushButton *deleteBtn = new QPushButton("Delete");
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #e53e3e;"
            "   color: white;"
            "   border-radius: 6px;"
            "   font-weight: bold;"
            "   height: 26px;" // Restricts button height to keep the cell sleek
            "}"
            "QPushButton:hover {"
            "   background-color: #c53030;"
            "}"
            );
        layout->addWidget(deleteBtn);

        // Row deletion routine with Confirmation Prompt
        // Row deletion routine with Custom Styled Confirmation Prompt
        connect(deleteBtn, &QPushButton::clicked, this, [this, deleteBtn]() {
            int row = ui->categoryTableWidget->indexAt(deleteBtn->pos()).row();
            if (row >= 0) {

                // 1. Create the message box instance manually
                QMessageBox msgBox(QMessageBox::Question,
                                   "Delete Confirmation",
                                   "Are you sure you want to delete this?",
                                   QMessageBox::Yes | QMessageBox::No,
                                   this);

                // 2. Apply custom CSS styling to the dialog window components
                msgBox.setStyleSheet(
                    "QMessageBox {"
                    "   background-color: #ffffff;" /* Clean white background */
                    "   border: 1px solid #cbd5e1;"
                    "   border-radius: 8px;"
                    "}"
                    "QLabel {"
                    "   color: #1e293b;" /* Dark slate color for readability */
                    "   font-size: 14px;"
                    "   font-weight: 500;"
                    "   padding-left: 10px;"
                    "}"
                    "QPushButton {"
                    "   background-color: #f1f5f9;" /* Soft grey default for buttons */
                    "   color: #334155;"
                    "   border: 1px solid #cbd5e1;"
                    "   border-radius: 5px;"
                    "   padding: 6px 14px;"
                    "   font-weight: bold;"
                    "   min-width: 65px;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #e2e8f0;"
                    "}"
                    /* Target the "Yes" button specifically to highlight it in red */
                    "QPushButton[text=\"&Yes\"], QPushButton[text=\"Yes\"] {"
                    "   background-color: #e53e3e;"
                    "   color: white;"
                    "   border: none;"
                    "}"
                    "QPushButton[text=\"&Yes\"]:hover, QPushButton[text=\"Yes\"]:hover {"
                    "   background-color: #c53030;"
                    "}"
                    );

                // 3. Execute the styled box and capture the answer
                int reply = msgBox.exec();

                // 4. If they confirm, wipe the row data out
                if (reply == QMessageBox::Yes) {
                    ui->categoryTableWidget->removeRow(row);

                    // Recalculate IDs so numbers stay uniform
                    for (int i = 0; i < ui->categoryTableWidget->rowCount(); ++i) {
                        ui->categoryTableWidget->item(i, 0)->setText(QString::number(i + 1));
                    }
                }
            }
        });

        actionContainer->setLayout(layout);
        ui->categoryTableWidget->setCellWidget(currentRowCount, 3, actionContainer);
    }

    delete popup;
}
