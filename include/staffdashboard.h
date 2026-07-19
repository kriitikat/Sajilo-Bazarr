#ifndef STAFFDASHBOARD_H
#define STAFFDASHBOARD_H

#include <QMainWindow>
#include <QPixmap>
#include <QString>
#include "../include/productstaff.h"
#include "../include/classlogout.h"
#include "../include/mytasks.h"

class QLabel;
class QTableWidget;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class StaffDashboard;
}
QT_END_NAMESPACE

class StaffDashboard : public ClassLogout
{
    Q_OBJECT
public:
    // staffId   = information.id of whoever just logged in (comes from
    //             Login). This is the value MyTasks filters the `tasks`
    //             table by (staff_id) — not name — so it stays correct
    //             even if two staff share a name or a name gets edited.
    // staffName = display name only, used for the welcome banner and
    //             passed straight through into MyTasks' greeting header.
    //             Username/picture are re-fetched from 'information' by
    //             id in loadCurrentUserInfo() so the dashboard (and the
    //             Profile page it opens) always show the freshest data.
    explicit StaffDashboard(int staffId, const QString &staffName, QWidget *parent = nullptr);
    ~StaffDashboard() override;

private slots:
    void handleProductsClicked();   // opens ProductStaff as its own top-level window
    void refreshDashboardStats();   // re-pulls counts + low stock rows from DB
    void handleProfileClicked();    // opens Profile for the currently logged-in staff
    void handleViewTasksClicked(); // opens MyTasks, filtered by this staff member's id

private:
    void setupDashboardContent();   // builds the cards + table once, in the ctor
    void loadCurrentUserInfo();     // pulls username/first_name/picture by id, fills welcome bar + avatar

    // Turns any source pixmap into a circular (Facebook-style) avatar of the
    // given diameter, with transparent corners.
    static QPixmap makeCircularPixmap(const QPixmap &source, int diameter);

    Ui::StaffDashboard *ui;

    int     m_staffId = -1;  // information.id — source of truth for task filtering
    QString m_staffName;     // display name (welcome banner, MyTasks header)
    QString m_username;      // login username — only needed to open Profile;
    // re-fetched from 'information' by id, not passed in

    // Stat cards
    QLabel *lblTotalProducts   = nullptr;
    QLabel *lblLowStockCount   = nullptr;
    QLabel *lblOutOfStockCount = nullptr;
    QLabel *lblPendingTasks    = nullptr; // filled from tasks WHERE staff_id + status='Pending'

    // Low stock table
    QTableWidget *tblLowStock  = nullptr;

    static constexpr int LOW_STOCK_THRESHOLD = 10;
    static constexpr int AVATAR_DIAMETER     = 40;
};

#endif // STAFFDASHBOARD_H