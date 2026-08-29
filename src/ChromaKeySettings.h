#pragma once

#include <QColor>
#include <QString>

struct ChromaKeySettings {
    bool enabled = false;

    QColor keyColor      = QColor(0, 177, 64); // default: chroma green
    int    tolerance     = 80;   // 0–255  primary keying threshold
    int    feather       = 15;   // 0–100  extra partial-alpha range (% of tolerance)
    int    spillSuppress = 30;   // 0–100  reduce residual key-colour cast

    enum BgMode { Transparent, SolidColor, BackgroundImage };
    BgMode  bgMode      = Transparent;
    QColor  bgColor     = Qt::black;
    QString bgImagePath;
};
