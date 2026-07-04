#include "staff.h"
#include "ui_staff.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCryptographicHash>
#include <QColor>

namespace {

// Passwords are stored as MD5 hex digests (consistent with the seed data in main.cpp).
// Swap for a stronger scheme if the login layer is ever upgraded.
QString hashPassword(const QString &plain)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(plain.toUtf8(), QCryptographicHash::Md5).toHex());
}

// Pretty-print the raw role value from the DB into a display label.
QString roleLabel(const QString &role)
{
    if (role.compare(QLatin1String("frontdesk"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Front Desk");
    if (role.compare(QLatin1String("staff"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Staff");
    // Fallback: capitalise first letter
    QString r = role;
    if (!r.isEmpty())
        r[0] = r[0].toUpper();
    return r;
}

} // namespace

// ─── Column indices ────────────────────────────────────────────────────────
// 0: ID  1: NAME  2: EMAIL  3: PHONE NO  4: STATUS  5: USERNAME  6: ROLE  7: ACTIONS
static constexpr int COL_ID       = 0;
static constexpr int COL_NAME     = 1;
static constexpr int COL_EMAIL    = 2;
static constexpr int COL_PHONE    = 3;
static constexpr int COL_STATUS   = 4;
static constexpr int COL_USERNAME = 5;
static constexpr int COL_ROLE     = 6;
static constexpr int COL_ACTIONS  = 7;
static constexpr int COL_COUNT    = 8;

staff::staff(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::staff)
{
    ui->setupUi(this);

    // ── Table setup (mirrors inventory.cpp's approach) ───────────────────
    const QStringList headers = {
        QStringLiteral("ID"),
        QStringLiteral("NAME"),
        QStringLiteral("EMAIL"),
        QStringLiteral("PHONE NO"),
        QStringLiteral("STATUS"),
        QStringLiteral("USERNAME"),
        QStringLiteral("ROLE"),
        QStringLiteral("ACTIONS")
    };
    ui->staffTable_3->setColumnCount(COL_COUNT);
    ui->staffTable_3->setHorizontalHeaderLabels(headers);
    ui->staffTable_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->staffTable_3->verticalHeader()->setVisible(false);

    connect(ui->addStaffBtn_3, &QPushButton::clicked, this, &staff::on_addStaffBtn_3_clicked);

    loadStaffTable();
}

staff::~staff()
{
    delete ui;
}

void staff::loadStaffTable()
{
    QSqlDatabase db = QSqlDatabase::database(); // default connection opened in main()
    if (!db.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("No open database connection."));
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, first_name, last_name, email, phone, status, username, role "
        "FROM information "
        "WHERE role = 'staff' OR role = 'frontdesk' "
        "ORDER BY id ASC"));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff: %1").arg(query.lastError().text()));
        return;
    }

    ui->staffTable_3->setRowCount(0);

    int row = 0;
    while (query.next()) {
        const int     id       = query.value(0).toInt();
        const QString name     = query.value(1).toString()
                             + QLatin1Char(' ')
                             + query.value(2).toString();
        const QString email    = query.value(3).toString();
        const QString phone    = query.value(4).toString();
        const QString status   = query.value(5).toString();
        const QString username = query.value(6).toString();
        const QString role     = query.value(7).toString();
        const bool    disabled = (status == QLatin1String("disabled"));

        ui->staffTable_3->insertRow(row);

        // Helper: create a non-editable cell; grey out text when disabled.
        auto setCell = [&](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            if (disabled)
                item->setForeground(QColor(QStringLiteral("#94a3b8")));
            ui->staffTable_3->setItem(row, col, item);
        };

        auto *idItem = new QTableWidgetItem(QString::number(id));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        ui->staffTable_3->setItem(row, COL_ID, idItem);

        setCell(COL_NAME,     name.trimmed());
        setCell(COL_EMAIL,    email);
        setCell(COL_PHONE,    phone);
        setCell(COL_STATUS,   status);
        setCell(COL_USERNAME, username);
        setCell(COL_ROLE,     roleLabel(role));

        ui->staffTable_3->setCellWidget(row, COL_ACTIONS, buildActionsWidget(id, disabled));

        ++row;
    }

    ui->staffTable_3->resizeRowsToContents();
}

QWidget *staff::buildActionsWidget(int staffId, bool disabled)
{
    auto *container = new QWidget(ui->staffTable_3);
    auto *layout    = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    const QString baseStyle = QStringLiteral(
        "QPushButton {"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  color: white;"
        "}");

    // Task — greyed out (not implemented)
    auto *taskBtn = new QPushButton(tr("Task"), container);
    taskBtn->setStyleSheet(baseStyle +
                           QStringLiteral("QPushButton { background-color: #94a3b8; }"));
    taskBtn->setEnabled(false);
    taskBtn->setToolTip(tr("Task assignment is not implemented yet"));
    connect(taskBtn, &QPushButton::clicked, this, [this, staffId]() { taskStaff(staffId); });

    // Edit — brand blue (matches inventory accent #2D4A7A)
    auto *editBtn = new QPushButton(tr("Edit"), container);
    editBtn->setStyleSheet(baseStyle +
                           QStringLiteral("QPushButton { background-color: #2D4A7A; }"
                                          "QPushButton:hover { background-color: #1B2A4A; }"));
    connect(editBtn, &QPushButton::clicked, this, [this, staffId]() { editStaff(staffId); });

    // Disable / Enable — amber / green
    auto *disableBtn = new QPushButton(disabled ? tr("Enable") : tr("Disable"), container);
    disableBtn->setStyleSheet(baseStyle + (disabled
                                               ? QStringLiteral("QPushButton { background-color: #16A34A; }"
                                                                "QPushButton:hover { background-color: #15803D; }")
                                               : QStringLiteral("QPushButton { background-color: #D97706; }"
                                                                "QPushButton:hover { background-color: #B45309; }")));
    connect(disableBtn, &QPushButton::clicked, this, [this, staffId]() { disableStaff(staffId); });

    // Delete — red
    auto *deleteBtn = new QPushButton(tr("Delete"), container);
    deleteBtn->setStyleSheet(baseStyle +
                             QStringLiteral("QPushButton { background-color: #DC2626; }"
                                            "QPushButton:hover { background-color: #991B1B; }"));
    connect(deleteBtn, &QPushButton::clicked, this, [this, staffId]() { deleteStaff(staffId); });

    layout->addWidget(taskBtn);
    layout->addWidget(editBtn);
    layout->addWidget(disableBtn);
    layout->addWidget(deleteBtn);
    layout->addStretch();

    container->setLayout(layout);
    return container;
}

// ─── Add Staff ─────────────────────────────────────────────────────────────

void staff::on_addStaffBtn_3_clicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Staff"));

    auto *form = new QFormLayout(&dialog);

    auto *firstNameEdit = new QLineEdit(&dialog);
    auto *lastNameEdit  = new QLineEdit(&dialog);
    auto *emailEdit     = new QLineEdit(&dialog);
    auto *phoneEdit     = new QLineEdit(&dialog);
    auto *usernameEdit  = new QLineEdit(&dialog);
    auto *passwordEdit  = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);

    // Role selector — matches the two roles stored in the DB
    auto *roleCombo = new QComboBox(&dialog);
    roleCombo->addItem(tr("Staff"),      QStringLiteral("staff"));
    roleCombo->addItem(tr("Front Desk"), QStringLiteral("frontdesk"));

    form->addRow(tr("First Name:"), firstNameEdit);
    form->addRow(tr("Last Name:"),  lastNameEdit);
    form->addRow(tr("Email:"),      emailEdit);
    form->addRow(tr("Phone:"),      phoneEdit);
    form->addRow(tr("Username:"),   usernameEdit);
    form->addRow(tr("Password:"),   passwordEdit);
    form->addRow(tr("Role:"),       roleCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString firstName = firstNameEdit->text().trimmed();
    const QString lastName  = lastNameEdit->text().trimmed();
    const QString email     = emailEdit->text().trimmed();
    const QString phone     = phoneEdit->text().trimmed();
    const QString username  = usernameEdit->text().trimmed();
    const QString password  = passwordEdit->text();
    const QString role      = roleCombo->currentData().toString();

    if (firstName.isEmpty() || username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Information"),
                             tr("First name, username, and password are required."));
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO information "
        "(first_name, last_name, username, role, email, phone, picture, password, status) "
        "VALUES (:first_name, :last_name, :username, :role, :email, :phone, NULL, :password, 'approved')"));
    query.bindValue(QStringLiteral(":first_name"), firstName);
    query.bindValue(QStringLiteral(":last_name"),  lastName);
    query.bindValue(QStringLiteral(":username"),   username);
    query.bindValue(QStringLiteral(":role"),       role);
    query.bindValue(QStringLiteral(":email"),      email);
    query.bindValue(QStringLiteral(":phone"),      phone);
    query.bindValue(QStringLiteral(":password"),   hashPassword(password));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to add staff: %1").arg(query.lastError().text()));
        return;
    }

    loadStaffTable();
}

// ─── Edit Staff ────────────────────────────────────────────────────────────

void staff::editStaff(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral(
        "SELECT first_name, last_name, email, phone, username, role "
        "FROM information WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), staffId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff record: %1").arg(fetch.lastError().text()));
        return;
    }

    const QString currentRole = fetch.value(5).toString();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Staff"));

    auto *form = new QFormLayout(&dialog);

    auto *firstNameEdit = new QLineEdit(fetch.value(0).toString(), &dialog);
    auto *lastNameEdit  = new QLineEdit(fetch.value(1).toString(), &dialog);
    auto *emailEdit     = new QLineEdit(fetch.value(2).toString(), &dialog);
    auto *phoneEdit     = new QLineEdit(fetch.value(3).toString(), &dialog);
    auto *usernameEdit  = new QLineEdit(fetch.value(4).toString(), &dialog);
    auto *passwordEdit  = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Leave blank to keep current password"));

    auto *roleCombo = new QComboBox(&dialog);
    roleCombo->addItem(tr("Staff"),      QStringLiteral("staff"));
    roleCombo->addItem(tr("Front Desk"), QStringLiteral("frontdesk"));
    // Pre-select the current role
    const int roleIdx = roleCombo->findData(currentRole);
    if (roleIdx >= 0)
        roleCombo->setCurrentIndex(roleIdx);

    form->addRow(tr("First Name:"),   firstNameEdit);
    form->addRow(tr("Last Name:"),    lastNameEdit);
    form->addRow(tr("Email:"),        emailEdit);
    form->addRow(tr("Phone:"),        phoneEdit);
    form->addRow(tr("Username:"),     usernameEdit);
    form->addRow(tr("New Password:"), passwordEdit);
    form->addRow(tr("Role:"),         roleCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString firstName = firstNameEdit->text().trimmed();
    const QString lastName  = lastNameEdit->text().trimmed();
    const QString email     = emailEdit->text().trimmed();
    const QString phone     = phoneEdit->text().trimmed();
    const QString username  = usernameEdit->text().trimmed();
    const QString password  = passwordEdit->text();
    const QString role      = roleCombo->currentData().toString();

    if (firstName.isEmpty() || username.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Information"),
                             tr("First name and username are required."));
        return;
    }

    QSqlQuery update(db);
    if (password.isEmpty()) {
        update.prepare(QStringLiteral(
            "UPDATE information "
            "SET first_name = :first_name, last_name = :last_name, "
            "    email = :email, phone = :phone, username = :username, role = :role "
            "WHERE id = :id"));
    } else {
        update.prepare(QStringLiteral(
            "UPDATE information "
            "SET first_name = :first_name, last_name = :last_name, "
            "    email = :email, phone = :phone, username = :username, role = :role, "
            "    password = :password "
            "WHERE id = :id"));
        update.bindValue(QStringLiteral(":password"), hashPassword(password));
    }
    update.bindValue(QStringLiteral(":first_name"), firstName);
    update.bindValue(QStringLiteral(":last_name"),  lastName);
    update.bindValue(QStringLiteral(":email"),      email);
    update.bindValue(QStringLiteral(":phone"),      phone);
    update.bindValue(QStringLiteral(":username"),   username);
    update.bindValue(QStringLiteral(":role"),       role);
    update.bindValue(QStringLiteral(":id"),         staffId);

    if (!update.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to update staff: %1").arg(update.lastError().text()));
        return;
    }

    loadStaffTable();
}

// ─── Disable / Enable Staff ────────────────────────────────────────────────

void staff::disableStaff(int staffId)
{
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery fetch(db);
    fetch.prepare(QStringLiteral("SELECT status FROM information WHERE id = :id"));
    fetch.bindValue(QStringLiteral(":id"), staffId);

    if (!fetch.exec() || !fetch.next()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to load staff status: %1").arg(fetch.lastError().text()));
        return;
    }

    const bool    currentlyDisabled = (fetch.value(0).toString() == QLatin1String("disabled"));
    const QString newStatus = currentlyDisabled
                                  ? QStringLiteral("approved")
                                  : QStringLiteral("disabled");

    const auto reply = QMessageBox::question(
        this,
        currentlyDisabled ? tr("Confirm Enable") : tr("Confirm Disable"),
        currentlyDisabled
            ? tr("Re-enable this staff member's account?")
            : tr("Disable this staff member's account? They will not be treated as active staff."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE information SET status = :status WHERE id = :id"));
    update.bindValue(QStringLiteral(":status"), newStatus);
    update.bindValue(QStringLiteral(":id"),     staffId);

    if (!update.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to update status: %1").arg(update.lastError().text()));
        return;
    }

    loadStaffTable();
}

// ─── Delete Staff ──────────────────────────────────────────────────────────

void staff::deleteStaff(int staffId)
{
    const auto reply = QMessageBox::question(
        this, tr("Confirm Delete"),
        tr("Are you sure you want to delete this staff member? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM information WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), staffId);

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to delete staff: %1").arg(query.lastError().text()));
        return;
    }

    loadStaffTable();
}

// ─── Task Staff (placeholder) ──────────────────────────────────────────────

void staff::taskStaff(int staffId)
{
    Q_UNUSED(staffId)
    QMessageBox::information(this, tr("Coming Soon"),
                             tr("Task assignment for staff is not implemented yet."));
}