#pragma once

#include <QWidget>
#include <QPixmap>

class PlaybackWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackWidget(QWidget* parent = nullptr);

    void showFrame(const QPixmap& pixmap);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
};
