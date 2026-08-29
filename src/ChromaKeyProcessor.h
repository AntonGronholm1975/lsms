#pragma once

#include "ChromaKeySettings.h"
#include <QImage>

class ChromaKeyProcessor
{
public:
    // Process src at its native resolution. bgOverride replaces loading from disk when
    // a pre-loaded (and pre-scaled) background image is available.
    static QImage process(const QImage& src,
                          const ChromaKeySettings& settings,
                          const QImage& bgOverride = {});
};
