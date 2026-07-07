#ifndef PENDING_H
#define PENDING_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class pending; }
QT_END_NAMESPACE

class pending : public QWidget
{
    Q_OBJECT

public:
    explicit pending(QWidget *parent = nullptr);
    ~pending();

private slots:
    // Loads / reloads all rows from the `pending_requests` table (the
    // self-registration queue). Every row there is implicitly pending —
    // there's no status column to filter on.
    void loadPendingTable();

    // Per-row action handlers, dispatched via lambda with the row id baked in.
    // approveRequest() moves the row into `information` (status='approved')
    // and removes it from `pending_requests`, inside one transaction.
    // declineRequest() simply deletes the row from `pending_requests`.
    void approveRequest(int userId);
    void declineRequest(int userId);

private:
    Ui::pending *ui;

    // Builds the ACTIONS cell widget (Approve / Decline buttons) for a given row.
    QWidget *buildActionsWidget(int userId);
};

#endif // PENDING_H