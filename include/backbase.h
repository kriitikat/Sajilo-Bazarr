#ifndef BACKBASE_H
#define BACKBASE_H

#include <QObject>
#include <QPushButton>

// ═══════════════════════════════════════════════════════════════════
//  BackBase<WidgetBase>
//
//  A tiny header-only mixin that gives any top-level page a one-line
//  "Back to Dashboard" button.
//
//  Why a TEMPLATE instead of a plain class: every page that needs this
//  already inherits a *different* Qt widget class —
//      inventory   extends QMainWindow
//      ProductBase (Product's own base) extends QWidget
//  — and Qt does not allow a class to inherit from two separate
//  QObject-derived classes. Making BackBase a template lets it slot in
//  underneath whichever Qt base a page already needs, instead of
//  competing with it:
//
//      class inventory   : public BackBase<QMainWindow> { ... };
//      class ProductBase : public BackBase<QWidget>     { ... };
//
//  BackBase itself declares no Q_OBJECT / signals / slots — a template
//  class can't be run through moc — so it stays completely invisible to
//  moc. Only the concrete page classes (inventory, ProductBase, ...)
//  need their usual Q_OBJECT, exactly as before this change.
//
//  Why goBackToDashboard() only calls close(): every page that opens a
//  child page (category, Product, inventory, staff, ...) does so by
//  creating it as its own new top-level window while leaving the
//  AdminDashboard that spawned it open underneath (see
//  admindashboard.cpp's handle*_clicked() slots) — so "Back to
//  Dashboard" only has to close the current page; the dashboard is
//  already sitting right behind it. This mirrors the same pattern
//  admindashboard.cpp already uses for category's
//  backToDashboardRequested signal.
// ═══════════════════════════════════════════════════════════════════
template <class WidgetBase>
class BackBase : public WidgetBase
{
public:
    using WidgetBase::WidgetBase; // reuse WidgetBase's constructor(s) as-is

protected:
    // Call once, after ui->setupUi(this), passing the Designer button
    // that should trigger the return to the dashboard.
    void wireBackButton(QPushButton *backButton)
    {
        QObject::connect(backButton, &QPushButton::clicked,
                          static_cast<WidgetBase *>(this),
                          [this]() { goBackToDashboard(); });
    }

    // Reveals the AdminDashboard window already sitting behind this page
    // by simply closing this one. Left virtual so a specific page could
    // add its own step first (e.g. "discard unsaved changes?") without
    // needing a second mixin.
    virtual void goBackToDashboard()
    {
        static_cast<WidgetBase *>(this)->close();
    }
};

#endif // BACKBASE_H
