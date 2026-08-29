#pragma once

#include <QDialog>
#include <QPixmap>
#include <QRectF>
#include <QWidget>

// Interactive crop-selection widget shown inside CropDialog
class CropPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CropPreviewWidget(QWidget* parent = nullptr);
    void setPixmap(const QPixmap& pixmap);
    void setCropRect(const QRectF& normRect);
    QRectF cropRect() const { return m_cropNorm; }

signals:
    void cropChanged(const QRectF& normRect);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    QRect  imageRect() const;
    QRectF toNormalized(const QRect& widgetRect) const;
    QRect  toWidget(const QRectF& normRect) const;

    QPixmap m_pixmap;
    QRectF  m_cropNorm;
    bool    m_dragging = false;
    QPoint  m_dragStart;
    QPoint  m_dragCurrent;
};

class CropDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CropDialog(const QPixmap& pixmap, const QRectF& existing, QWidget* parent = nullptr);

    QRectF cropRect() const;
    bool   applyToAll() const { return m_applyToAll; }

private:
    CropPreviewWidget* m_preview;
    bool m_applyToAll = false;
};
