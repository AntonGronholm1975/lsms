#pragma once

#include <QListWidget>
#include <QPixmap>
#include <QVector>
#include <functional>

class FilmstripWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit FilmstripWidget(QWidget* parent = nullptr);

    void populate(int count,
                  std::function<QPixmap(int)> pixmapFor,
                  std::function<int(int)>     durationFor = nullptr);
    void selectFrame(int index);

signals:
    void frameSelected(int index);
    void frameMoved(int from, int to);
    void deleteFrameRequested(int index);
    void duplicateFrameRequested(int index);
    void frameDurationChangeRequested(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QListWidgetItem* m_dragStartItem = nullptr;
};
