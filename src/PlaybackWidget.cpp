#include "PlaybackWidget.h"

#include <QPainter>
#include <QPaintEvent>

PlaybackWidget::PlaybackWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PlaybackWidget::showFrame(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    update();
}

void PlaybackWidget::clear()
{
    m_pixmap = {};
    update();
}

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

    QSize scaled = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
    QRect target(QPoint(0, 0), scaled);
    target.moveCenter(rect().center());
    painter.drawPixmap(target, m_pixmap);
}
