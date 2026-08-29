#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Luna's Stop Motion Studio");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("LSMS");
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    return app.exec();
}
