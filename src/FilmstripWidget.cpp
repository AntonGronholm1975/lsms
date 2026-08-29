#include "FilmstripWidget.h"

#include <QContextMenuEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMouseEvent>

static constexpr int kThumbSize = 100;
static constexpr int kStripHeight = kThumbSize + 60;

FilmstripWidget::FilmstripWidget(QWidget* parent)
    : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setMovement(QListView::Snap);
    setResizeMode(QListView::Adjust);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize({kThumbSize, kThumbSize});
    setGridSize({kThumbSize + 20, kThumbSize + 30});
    setFlow(QListView::LeftToRight);
    setWrapping(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFixedHeight(kStripHeight);

    connect(this, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0)
            emit frameSelected(row);
    });
}

void FilmstripWidget::populate(int count, std::function<QPixmap(int)> pixmapFor)
{
    blockSignals(true);
    clear();
    for (int i = 0; i < count; ++i) {
        QPixmap thumb = pixmapFor(i).scaled(
            kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto* it = new QListWidgetItem(QIcon(thumb), QString::number(i + 1));
        it->setData(Qt::UserRole, i);
        addItem(it);
    }
    blockSignals(false);
}

void FilmstripWidget::selectFrame(int index)
{
    if (index >= 0 && index < count())
        setCurrentRow(index);
}

void FilmstripWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartItem = itemAt(event->pos());
    QListWidget::mousePressEvent(event);
}

void FilmstripWidget::dropEvent(QDropEvent* event)
{
    if (!m_dragStartItem) {
        QListWidget::dropEvent(event);
        return;
    }

    int fromRow = row(m_dragStartItem);
    QListWidget::dropEvent(event);
    int toRow = row(m_dragStartItem);
    m_dragStartItem = nullptr;

    if (fromRow != toRow)
        emit frameMoved(fromRow, toRow);
}

void FilmstripWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QListWidgetItem* it = itemAt(event->pos());
    if (!it)
        return;

    int frameIdx = row(it);
    QMenu menu(this);
    QAction* dupAct = menu.addAction("Duplicate");
    menu.addSeparator();
    QAction* delAct = menu.addAction("Delete");

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == dupAct)
        emit duplicateFrameRequested(frameIdx);
    else if (chosen == delAct)
        emit deleteFrameRequested(frameIdx);
}
