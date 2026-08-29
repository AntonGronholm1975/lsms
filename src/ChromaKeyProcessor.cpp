#include "ChromaKeyProcessor.h"

#include <algorithm>
#include <cmath>

static inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

QImage ChromaKeyProcessor::process(const QImage& src,
                                    const ChromaKeySettings& s,
                                    const QImage& bgOverride)
{
    if (!s.enabled || src.isNull())
        return src;

    // Ensure ARGB32 so scanLine gives BGRA quads
    QImage input  = src.convertToFormat(QImage::Format_ARGB32);
    QImage result(input.size(), QImage::Format_ARGB32);

    // Resolve background
    QImage bg;
    if (s.bgMode == ChromaKeySettings::BackgroundImage) {
        if (!bgOverride.isNull())
            bg = bgOverride.convertToFormat(QImage::Format_ARGB32);
        else if (!s.bgImagePath.isEmpty())
            bg = QImage(s.bgImagePath)
                     .scaled(input.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                     .convertToFormat(QImage::Format_ARGB32);
    }

    const float kr = s.keyColor.redF();
    const float kg = s.keyColor.greenF();
    const float kb = s.keyColor.blueF();

    // Normalised threshold and feather range
    const float tol       = s.tolerance  / 255.f;
    const float featherW  = tol * s.feather / 100.f;
    const float spill     = s.spillSuppress / 100.f;

    // Which channel is dominant in key colour?
    const bool greenKey = (kg >= kr && kg >= kb);
    const bool blueKey  = (kb >= kr && kb >= kg) && !greenKey;

    const float bgR = (s.bgMode == ChromaKeySettings::SolidColor) ? s.bgColor.redF()   : 0.f;
    const float bgG = (s.bgMode == ChromaKeySettings::SolidColor) ? s.bgColor.greenF() : 0.f;
    const float bgB = (s.bgMode == ChromaKeySettings::SolidColor) ? s.bgColor.blueF()  : 0.f;

    for (int y = 0; y < input.height(); ++y) {
        const QRgb* srcLine = reinterpret_cast<const QRgb*>(input.constScanLine(y));
        QRgb*       dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* bgLine  = (!bg.isNull())
            ? reinterpret_cast<const QRgb*>(bg.constScanLine(y))
            : nullptr;

        for (int x = 0; x < input.width(); ++x) {
            const QRgb px = srcLine[x];
            float r = qRed(px)   / 255.f;
            float g = qGreen(px) / 255.f;
            float b = qBlue(px)  / 255.f;

            // RGB Euclidean distance from key colour, normalised 0..1
            float dr = r - kr, dg = g - kg, db = b - kb;
            float dist = std::sqrt(dr*dr + dg*dg + db*db) / 1.7320508f;

            // Alpha: 0 = fully keyed, 1 = fully visible
            float alpha;
            if (dist <= tol)
                alpha = 0.f;
            else if (featherW > 0.f && dist <= tol + featherW)
                alpha = (dist - tol) / featherW;
            else
                alpha = 1.f;

            // Spill suppression (applied even to partially visible pixels)
            if (alpha < 1.f && spill > 0.f) {
                float spillAmt = (1.f - alpha) * spill;
                if (greenKey) {
                    float avg = (r + b) * 0.5f;
                    g = clamp01(g - spillAmt * std::max(0.f, g - avg));
                } else if (blueKey) {
                    float avg = (r + g) * 0.5f;
                    b = clamp01(b - spillAmt * std::max(0.f, b - avg));
                }
            }

            // Resolve background colour for this pixel
            float br = bgR, bgg = bgG, bgb = bgB;
            if (bgLine) {
                const QRgb bp = bgLine[x];
                br  = qRed(bp)   / 255.f;
                bgg = qGreen(bp) / 255.f;
                bgb = qBlue(bp)  / 255.f;
            }

            if (s.bgMode == ChromaKeySettings::Transparent) {
                dstLine[x] = qRgba(int(r*255), int(g*255), int(b*255), int(alpha*255));
            } else {
                // Alpha-composite foreground over background
                float fr = alpha*r  + (1.f-alpha)*br;
                float fg = alpha*g  + (1.f-alpha)*bgg;
                float fb = alpha*b  + (1.f-alpha)*bgb;
                dstLine[x] = qRgba(int(fr*255), int(fg*255), int(fb*255), 255);
            }
        }
    }
    return result;
}
