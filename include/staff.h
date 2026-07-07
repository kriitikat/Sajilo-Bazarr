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

    // "+ Add Staff" button handler — opens an inline dialog to insert a new row.
    void on_addStaffBtn_3_clicked();

    // Per-row action buttons, dispatched with the staff id baked into the slot via lambda.
    void editStaff(int staffId);
    void disableStaff(int staffId); // toggles status between 'enabled' and 'disabled'
    void expireStaff(int staffId);  // sets status to 'expired'
    void taskStaff(int staffId);    // opens taskmanagement for this staff member

private:
    Ui::staff *ui;

    // Builds the ACTIONS cell widget (Task / Edit / Disable-Enable / Expire buttons)
    // for a given row + staff id. 'status' (enabled/disabled/expired) controls the
    // Disable/Enable label and whether the Expire button is available.
    // Visual styling for these buttons lives entirely in staff.ui via the
    // "actionRole" dynamic property + attribute-selector stylesheet rules;
    // this method only assigns roles/behaviour, never colors or fonts.
    QWidget *buildActionsWidget(int staffId, const QString &status);
};

#endif // STAFF_H