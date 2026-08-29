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
    m_onionFrames.clear();
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

    // Draw previous frames behind the main frame (oldest first, most recent last)
    if (m_onionEnabled && !m_onionFrames.isEmpty()) {
        static const qreal kOpacities[] = {0.60, 0.35};
        int n = m_onionFrames.size();
        for (int i = n - 1; i >= 0; --i) {
            qreal op = (i < 2) ? kOpacities[i] : 0.15;
            painter.setOpacity(op);
            painter.drawPixmap(target,
                m_onionFrames.at(i).scaled(target.size(),
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            // Reddish tint to distinguish onion frames from the main frame
            painter.setOpacity(op * 0.35);
            painter.fillRect(target, QColor(220, 60, 60));
        }
        painter.setOpacity(1.0);
    }

    painter.drawPixmap(target, m_pixmap);

    // Remind the user when onion is active but there is no previous frame
    if (m_onionEnabled && m_onionFrames.isEmpty()) {
        painter.setPen(QColor(200, 120, 120));
        QFont f = painter.font();
        f.setPointSize(9);
        painter.setFont(f);
        painter.drawText(rect().adjusted(4, 4, -4, -4),
            Qt::AlignTop | Qt::AlignRight, "Onion: no previous frame");
    }
}
