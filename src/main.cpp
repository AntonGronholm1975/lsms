#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include "MainWindow.h"

static QIcon createAppIcon()
{
    QPixmap pm(256, 256);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // Dark rounded-square background
    p.setBrush(QColor(18, 18, 42));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, 256, 256, 52, 52);

    // Crescent moon: large circle minus offset circle
    QPainterPath moonFull;
    moonFull.addEllipse(24, 44, 176, 176);
    QPainterPath cutout;
    cutout.addEllipse(80, 28, 156, 156);
    p.setBrush(QColor(248, 212, 72));
    p.drawPath(moonFull.subtracted(cutout));

    // Stars scattered in the upper-right area
    p.setBrush(Qt::white);
    for (auto [x, y, r] : {std::tuple{192, 52, 5},
                            {220, 96, 3},
                            {200, 148, 4},
                            {174, 44, 3},
                            {228, 148, 3}})
        p.drawEllipse(x - r, y - r, 2 * r, 2 * r);

    p.end();
    return QIcon(pm);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Luna's Stop Motion Studio");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("LSMS");
    app.setStyle("Fusion");
    app.setWindowIcon(createAppIcon());

    MainWindow window;
    window.show();

    return app.exec();
}
