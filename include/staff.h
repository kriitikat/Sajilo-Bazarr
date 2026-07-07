#ifndef STAFF_H
#define STAFF_H

#include <QWidget>

namespace Ui {
class staff;
}

class staff : public QWidget
{
    Q_OBJECT

public:
    explicit staff(QWidget *parent = nullptr);
    ~staff();

private slots:
    // Loads/reloads all rows with role = 'staff' or 'frontdesk' from the information table.
    void loadStaffTable();

    // Top-nav "Tasks" button — opens TaskManagement as its own window. This is
    // a page-level entry point now (no longer tied to a specific staff row).
    void on_navTasksBtn_3_clicked();

    // Per-row action buttons, dispatched with the staff id baked into the slot via lambda.
    void editStaff(int staffId);
    void disableStaff(int staffId); // toggles status between 'enabled' and 'disabled'

    // Toggles status between 'expired' and 'enabled'. When the row is
    // currently expired, the ACTIONS cell shows "Unexpire" instead of
    // "Expire" and calling this restores the account to 'enabled'.
    void expireStaff(int staffId);

private:
    Ui::staff *ui;

    // Builds the ACTIONS cell widget (Edit / Disable-Enable / Expire-Unexpire
    // buttons) for a given row + staff id. 'status' (enabled/disabled/expired)
    // controls the Disable/Enable label and the Expire/Unexpire label.
    // Visual styling for these buttons lives entirely in staff.ui via the
    // "actionRole" dynamic property + attribute-selector stylesheet rules;
    // this method only assigns roles/behaviour, never colors or fonts.
    QWidget *buildActionsWidget(int staffId, const QString &status);
};

#endif // STAFF_H