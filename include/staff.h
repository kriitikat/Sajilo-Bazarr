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
    void disableStaff(int staffId); // toggles status between 'approved' and 'disabled'
    void deleteStaff(int staffId);
    void taskStaff(int staffId);    // placeholder only

private:
    Ui::staff *ui;

    // Builds the ACTIONS cell widget (Task / Edit / Disable / Delete buttons)
    // for a given row + staff id. 'disabled' controls the Disable/Enable label.
    QWidget *buildActionsWidget(int staffId, bool disabled);
};

#endif // STAFF_H