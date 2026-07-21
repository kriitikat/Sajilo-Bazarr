#ifndef FRONTDESK_H
#define FRONTDESK_H

#include <QMainWindow>
#include <QPixmap>
#include <QString>
#include "../include/classlogout.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class frontdesk;
}
QT_END_NAMESPACE

class QFrame;
class QLabel;

class frontdesk : public ClassLogout
{
    Q_OBJECT

public:
    // staffId   = information.id of whoever just logged in (comes from
    //             Login). Kept for parity with StaffDashboard and for
    //             any future front-desk feature that needs to filter by
    //             id rather than name; not required by anything here yet.
    // staffName = display name only, used for the welcome banner until
    //             loadCurrentUserInfo() re-fetches the freshest name by
    //             id. Username/picture are always pulled fresh from
    //             'information' by id, same convention as StaffDashboard,
    //             so the topbar (and the FrontProfile page it opens)
    //             always show current data.
    // Both params default so any existing call site that only passes a
    // parent still compiles; the welcome banner/avatar just won't be
    // personalized in that case, exactly like StaffDashboard's staffId<0
    // fallback.
    explicit frontdesk(int staffId = -1, const QString &staffName = QString(),
                        QWidget *parent = nullptr);
    ~frontdesk();

private slots:
    void openBillingWindow();
    void openViewProductsWindow();
    void handleProfileClicked();   // opens FrontProfile for the currently logged-in front desk user

private:
    Ui::frontdesk *ui;

    // Dashboard population
    void loadDashboardStats();
    void setupProductTable();
    void loadRecentSales();
    void loadCurrentUserInfo();    // pulls username/first_name/picture by id, fills welcome bar + avatar
    QFrame *createStatCard(QWidget *parent, const QString &title,
                           const QString &value, const QString &accentColor);
    // Turns any source pixmap into a circular (Facebook-style) avatar of the
    // given diameter, with transparent corners.
    static QPixmap makeCircularPixmap(const QPixmap &source, int diameter);

    // Keep pointers so we can refresh the values later without rebuilding cards
    QLabel *lblItemsSoldValue   = nullptr;
    QLabel *lblSalesTotalValue  = nullptr;
    QLabel *lblProductsSoldValue = nullptr;
    QLabel *lblLowStockValue   = nullptr;

    int     m_staffId = -1;  // information.id — source of truth once anything here needs it
    QString m_staffName;     // display name (welcome banner)
    QString m_username;      // login username — only needed to open FrontProfile;
    // re-fetched from 'information' by id, not passed in

    // Low stock threshold - adjust as needed, or wire this to a real
    // reorder-level column later if you add one to the products table.
    static const int LOW_STOCK_THRESHOLD = 10;
    static constexpr int AVATAR_DIAMETER = 40;
};

#endif // FRONTDESK_H
