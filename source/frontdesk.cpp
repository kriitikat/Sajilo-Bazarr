#include "../include/frontdesk.h"
#include "../ui/ui_frontdesk.h"

frontdesk::frontdesk(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::frontdesk)
{
    ui->setupUi(this);
}

frontdesk::~frontdesk()
{
    delete ui;
}