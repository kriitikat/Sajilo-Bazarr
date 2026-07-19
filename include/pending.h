#ifndef PENDING_H
#define PENDING_H

#include <QWidget>
#include "../include/backbase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class pending; }
QT_END_NAMESPACE

// Fixed upper bound on how many pending requests can be shown at once.
// A plain #define constant, the same way a C program sizes a fixed
// array up front (Symbolic Constants).
#define MAX_PENDING_ROWS 200

// One row of data pulled from the `pending_requests` table. A plain
// C-style struct with fixed-size char arrays instead of QString, so the
// whole table-loading routine reads like a C program processing an
// array of structures
struct PendingRow
{
    int  id;
    char firstName[64];
    char lastName[64];
    char username[64];
    char role[32];
    char email[128];
    char phone[32];
};

// Inherits BackBase<QWidget> instead of QWidget directly — this is the
// only change needed to pick up wireBackButton()/goBackToDashboard()
// (see backbase.h for why it's a template). Everything else about this
// class is exactly as before.
class pending : public BackBase<QWidget>
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

    // A single Approve slot and a single Decline slot are connected to
    // EVERY approve/decline button in the table (no per-button lambdas).
    // Each button carries its own row id as a plain "userId" property;
    // the slot reads sender()'s property to know which row fired it —
    // the same idea as passing an extra argument into a function
    // (Ch. 6.4), just carried on the widget instead of a parameter list.
    void onApproveClicked();
    void onDeclineClicked();

private:
    Ui::pending *ui;

    // Fixed-size array of structures holding the rows currently on
    // tracks how many of the MAX_PENDING_ROWS slots are actually filled,
    // the same way a C program tracks a used-length alongside a fixed
    // array instead of relying on the array to know its own size.
    PendingRow pendingRows[MAX_PENDING_ROWS];
    int        rowCount;

    // Runs the SELECT and fills the caller-provided array/count through
    // pointers, instead of building and returning a container
    // function uses to "return" more than one value).
    void fetchPendingRows(PendingRow *outRows, int *outCount);

    // Builds the ACTIONS cell widget (Approve / Decline buttons) for a
    // given row id.
    QWidget *buildActionsWidget(int userId);

    // Runs the actual approve/decline database work for one row id.
    void approveRequest(int userId);
    void declineRequest(int userId);

    // Turns a role code ("staff"/"frontdesk") into its display label,
    // written into the caller's buffer. An if-else chain rather than a
    // switch, since C can only switch on an int/char value, not a
    void roleDisplayLabel(const char *roleCode, char *outLabel, int outLabelSize);
};

#endif // PENDING_H