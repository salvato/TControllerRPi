#include "tcontrollerrpi.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TControllerRPi w;
    QCoreApplication::setOrganizationDomain("Gabriele.Salvato");
    QCoreApplication::setOrganizationName("Gabriele.Salvato");
    QCoreApplication::setApplicationName("TController");
    QCoreApplication::setApplicationVersion("1.0.0");
    w.show();

    return a.exec();
}
