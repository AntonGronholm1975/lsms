#pragma once

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QRectF>
#include <QVector>
#include <QWidget>
#include "ChromaKeySettings.h"

enum class DrawTool { Pen, Line, Rectangle, Ellipse, Eraser };

class PlaybackWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackWidget(QWidget* parent = nullptr);

    void showFrame(const QPixmap& pixmap);
    void clear();
    void setOnionEnabled(bool enabled);
    void setOnionFrames(const QVector<QPixmap>& prevFrames);

    // Crop
    void setCropRect(const QRectF& normRect);

    // Chroma key
    void setChromaSettings(const ChromaKeySettings& settings);

    // Drawing overlay
    void setDrawingEnabled(bool enabled);
    void setDrawTool(DrawTool tool);
    void setDrawColor(const QColor& color);
    void setBrushSize(int size);
    void setAnnotation(const QPixmap& layer);
    QPixmap annotation() const { return m_annotation; }
    void clearAnnotation();

signals:
    void annotationChanged(const QPixmap& layer);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    QRect  computeTarget(const QPixmap& px) const;
    QPoint widgetToAnnotation(const QPoint& wp) const;
    void   drawOnAnnotation(const QPoint& from, const QPoint& to);
    void   commitShapeToAnnotation();
    void   ensureAnnotation();
    void   applyChromaKey();

    QPixmap        m_pixmap;
    QVector<QPixmap> m_onionFrames;
    bool           m_onionEnabled = false;
    QRectF         m_cropRect;

    // Chroma key
    ChromaKeySettings m_chromaSettings;
    QPixmap           m_displayPixmap; // processed result (chroma applied)
    QImage            m_bgImage;       // loaded bg image for chroma (at display res)

    // Drawing
    QPixmap   m_annotation;
    DrawTool  m_drawTool   = DrawTool::Pen;
    QColor    m_drawColor  = Qt::red;
    int       m_brushSize  = 4;
    bool      m_drawEnabled = false;
    bool      m_isDrawing  = false;
    QPoint    m_drawStart;
    QPoint    m_drawLast;
    QPoint    m_drawCurrent;

    QRect m_target; // last computed display rect, updated in paintEvent
};
