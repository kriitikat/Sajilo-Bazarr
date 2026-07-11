#ifndef CLASSLOGOUT_H
#define CLASSLOGOUT_H

#include <QMainWindow>

// Common base class for every window that has a "Logout" button
// (AdminDashboard, StaffDashboard, frontdesk, ...).
//
// OOP concept used: INHERITANCE.
// The logout logic lives here ONCE. Every dashboard inherits it
// instead of each one writing its own copy.
class ClassLogout : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClassLogout(QWidget *parent = nullptr);
    virtual ~ClassLogout() = default;

protected slots:
    void handleLogout_clicked();   // shared by all subclasses
};

#endif // CLASSLOGOUT_H