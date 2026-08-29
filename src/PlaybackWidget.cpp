#include "PlaybackWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

PlaybackWidget::PlaybackWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(false);
}

void PlaybackWidget::showFrame(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    update();
}

void PlaybackWidget::clear()
{
    m_pixmap = {};
    m_onionFrames.clear();
    m_annotation = {};
    update();
}

void PlaybackWidget::setOnionEnabled(bool enabled)
{
    m_onionEnabled = enabled;
    update();
}

void PlaybackWidget::setOnionFrames(const QVector<QPixmap>& prevFrames)
{
    m_onionFrames = prevFrames;
    update();
}

void PlaybackWidget::setCropRect(const QRectF& normRect)
{
    m_cropRect = normRect;
    update();
}

void PlaybackWidget::setDrawingEnabled(bool enabled)
{
    m_drawEnabled = enabled;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
}

void PlaybackWidget::setDrawTool(DrawTool tool)  { m_drawTool  = tool;  }
void PlaybackWidget::setDrawColor(const QColor& c) { m_drawColor = c; }
void PlaybackWidget::setBrushSize(int s)           { m_brushSize = s; }

void PlaybackWidget::setAnnotation(const QPixmap& layer)
{
    m_annotation = layer;
    update();
}

void PlaybackWidget::clearAnnotation()
{
    m_annotation = {};
    emit annotationChanged(m_annotation);
    update();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

QRect PlaybackWidget::computeTarget(const QPixmap& px) const
{
    if (px.isNull()) return {};
    // Apply crop before computing display rect
    QSizeF srcSize = px.size();
    if (!m_cropRect.isEmpty())
        srcSize = QSizeF(px.width() * m_cropRect.width(),
                         px.height() * m_cropRect.height());
    QSize scaled = srcSize.toSize().scaled(size(), Qt::KeepAspectRatio);
    QRect r(QPoint(0, 0), scaled);
    r.moveCenter(rect().center());
    return r;
}

QPoint PlaybackWidget::widgetToAnnotation(const QPoint& wp) const
{
    if (m_target.isEmpty() || m_annotation.isNull()) return {};
    return QPoint(
        qRound(double(wp.x() - m_target.x()) * m_annotation.width()  / m_target.width()),
        qRound(double(wp.y() - m_target.y()) * m_annotation.height() / m_target.height())
    );
}

void PlaybackWidget::ensureAnnotation()
{
    if (!m_annotation.isNull() || m_target.isEmpty()) return;
    m_annotation = QPixmap(m_target.size());
    m_annotation.fill(Qt::transparent);
}

void PlaybackWidget::drawOnAnnotation(const QPoint& from, const QPoint& to)
{
    ensureAnnotation();
    if (m_annotation.isNull()) return;

    QPainter p(&m_annotation);
    p.setRenderHint(QPainter::Antialiasing);
    if (m_drawTool == DrawTool::Eraser) {
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.setPen(QPen(Qt::transparent, m_brushSize * 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else {
        p.setPen(QPen(m_drawColor, m_brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    p.drawLine(from, to);
    update();
}

void PlaybackWidget::commitShapeToAnnotation()
{
    ensureAnnotation();
    if (m_annotation.isNull()) return;

    QPoint aStart  = widgetToAnnotation(m_drawStart);
    QPoint aCurrent = widgetToAnnotation(m_drawCurrent);

    QPainter p(&m_annotation);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(m_drawColor, m_brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    switch (m_drawTool) {
        case DrawTool::Line:
            p.drawLine(aStart, aCurrent);
            break;
        case DrawTool::Rectangle:
            p.drawRect(QRect(aStart, aCurrent).normalized());
            break;
        case DrawTool::Ellipse:
            p.drawEllipse(QRect(aStart, aCurrent).normalized());
            break;
        default: break;
    }
    emit annotationChanged(m_annotation);
    update();
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void PlaybackWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0x1a, 0x1a, 0x1a));
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_pixmap.isNull()) {
        painter.setPen(QColor(0x88, 0x88, 0x88));
        QFont f = painter.font();
        f.setPointSize(12);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter,
            "No frames loaded\n\nUse File \u2192 Open Images\u2026 to get started");
        return;
    }

    m_target = computeTarget(m_pixmap);

    // Source rect within m_pixmap (crop)
    QRect srcRect = m_pixmap.rect();
    if (!m_cropRect.isEmpty()) {
        srcRect = QRect(
            qRound(m_pixmap.width()  * m_cropRect.x()),
            qRound(m_pixmap.height() * m_cropRect.y()),
            qRound(m_pixmap.width()  * m_cropRect.width()),
            qRound(m_pixmap.height() * m_cropRect.height())
        );
    }

    // Onion frames
    if (m_onionEnabled && !m_onionFrames.isEmpty()) {
        static const qreal kOpacities[] = {0.60, 0.35};
        int n = m_onionFrames.size();
        for (int i = n - 1; i >= 0; --i) {
            qreal op = (i < 2) ? kOpacities[i] : 0.15;
            painter.setOpacity(op);
            painter.drawPixmap(m_target,
                m_onionFrames.at(i).scaled(m_target.size(),
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            painter.setOpacity(op * 0.35);
            painter.fillRect(m_target, QColor(220, 60, 60));
        }
        painter.setOpacity(1.0);
    }

    painter.drawPixmap(m_target, m_pixmap, srcRect);

    // Annotation overlay
    if (!m_annotation.isNull())
        painter.drawPixmap(m_target, m_annotation);

    // In-progress shape preview (line / rect / ellipse)
    if (m_drawEnabled && m_isDrawing &&
        m_drawTool != DrawTool::Pen && m_drawTool != DrawTool::Eraser) {
        painter.setPen(QPen(m_drawColor, m_brushSize, Qt::SolidLine,
                             Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.setRenderHint(QPainter::Antialiasing);
        QRect preview = QRect(m_drawStart, m_drawCurrent).normalized();
        switch (m_drawTool) {
            case DrawTool::Line:      painter.drawLine(m_drawStart, m_drawCurrent); break;
            case DrawTool::Rectangle: painter.drawRect(preview); break;
            case DrawTool::Ellipse:   painter.drawEllipse(preview); break;
            default: break;
        }
    }

    if (m_onionEnabled && m_onionFrames.isEmpty()) {
        painter.setPen(QColor(200, 120, 120));
        QFont f = painter.font();
        f.setPointSize(9);
        painter.setFont(f);
        painter.drawText(rect().adjusted(4, 4, -4, -4),
            Qt::AlignTop | Qt::AlignRight, "Onion: no previous frame");
    }
}

// ── Mouse (drawing) ───────────────────────────────────────────────────────────

void PlaybackWidget::mousePressEvent(QMouseEvent* e)
{
    if (!m_drawEnabled || e->button() != Qt::LeftButton) return;
    m_isDrawing  = true;
    m_drawStart  = e->pos();
    m_drawLast   = e->pos();
    m_drawCurrent = e->pos();

    if (m_drawTool == DrawTool::Pen || m_drawTool == DrawTool::Eraser) {
        QPoint ap = widgetToAnnotation(e->pos());
        drawOnAnnotation(ap, ap);
    }
}

void PlaybackWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_drawEnabled || !m_isDrawing) return;
    m_drawCurrent = e->pos();

    if (m_drawTool == DrawTool::Pen || m_drawTool == DrawTool::Eraser) {
        QPoint from = widgetToAnnotation(m_drawLast);
        QPoint to   = widgetToAnnotation(e->pos());
        drawOnAnnotation(from, to);
        m_drawLast = e->pos();
    } else {
        update(); // repaint preview
    }
}

void PlaybackWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_drawEnabled || !m_isDrawing) return;
    m_isDrawing   = false;
    m_drawCurrent = e->pos();

    if (m_drawTool == DrawTool::Pen || m_drawTool == DrawTool::Eraser)
        emit annotationChanged(m_annotation);
    else
        commitShapeToAnnotation();
}
