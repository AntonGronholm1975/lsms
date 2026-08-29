#pragma once

#include <QPixmap>
#include <QVector>
#include <QWidget>

class PlaybackWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackWidget(QWidget* parent = nullptr);

    void showFrame(const QPixmap& pixmap);
    void clear();
    void setOnionEnabled(bool enabled);
    void setOnionFrames(const QVector<QPixmap>& prevFrames);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    QVector<QPixmap> m_onionFrames;
    bool m_onionEnabled = false;
};
