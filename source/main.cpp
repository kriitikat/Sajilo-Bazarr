#include <QApplication>
#include "admindashboard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AdminDashboard admindashboard;
    admindashboard.show();

    return a.exec();
}