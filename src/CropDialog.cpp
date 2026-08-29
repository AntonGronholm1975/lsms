#include "CropDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

// ── CropPreviewWidget ─────────────────────────────────────────────────────────

CropPreviewWidget::CropPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(480, 360);
}

void CropPreviewWidget::setPixmap(const QPixmap& px)
{
    m_pixmap = px;
    update();
}

void CropPreviewWidget::setCropRect(const QRectF& norm)
{
    m_cropNorm = norm;
    update();
}

QRect CropPreviewWidget::imageRect() const
{
    if (m_pixmap.isNull())
        return rect();
    QSize s = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
    QRect r(QPoint(0, 0), s);
    r.moveCenter(rect().center());
    return r;
}

QRectF CropPreviewWidget::toNormalized(const QRect& wr) const
{
    QRect ir = imageRect();
    if (ir.isEmpty()) return {};
    return QRectF(
        double(wr.x() - ir.x()) / ir.width(),
        double(wr.y() - ir.y()) / ir.height(),
        double(wr.width())       / ir.width(),
        double(wr.height())      / ir.height()
    );
}

QRect CropPreviewWidget::toWidget(const QRectF& n) const
{
    QRect ir = imageRect();
    return QRect(
        ir.x() + qRound(n.x()      * ir.width()),
        ir.y() + qRound(n.y()      * ir.height()),
        qRound(n.width()  * ir.width()),
        qRound(n.height() * ir.height())
    );
}

void CropPreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(40, 40, 40));
    if (m_pixmap.isNull()) return;

    QRect ir = imageRect();
    p.drawPixmap(ir, m_pixmap);

    // Active selection: dim, then re-expose the crop region
    QRectF normSel = m_dragging
        ? toNormalized(QRect(m_dragStart, m_dragCurrent).normalized().intersected(ir))
        : m_cropNorm;

    if (!normSel.isEmpty()) {
        p.fillRect(ir, QColor(0, 0, 0, 110));

        QRect sel = toWidget(normSel);

        // Re-draw the crop area undimmed
        QRect srcCrop(
            qRound(m_pixmap.width()  * normSel.x()),
            qRound(m_pixmap.height() * normSel.y()),
            qRound(m_pixmap.width()  * normSel.width()),
            qRound(m_pixmap.height() * normSel.height())
        );
        p.drawPixmap(sel, m_pixmap, srcCrop);

        p.setPen(QPen(Qt::white, 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(sel);

        // Info label
        QString info = QString("%1 × %2 px")
            .arg(qRound(m_pixmap.width()  * normSel.width()))
            .arg(qRound(m_pixmap.height() * normSel.height()));
        p.setPen(Qt::white);
        p.drawText(sel.bottomLeft() + QPoint(4, 16), info);
    } else {
        p.setPen(QColor(160, 160, 160));
        p.drawText(rect(), Qt::AlignCenter, "Drag to select crop area");
    }
}

void CropPreviewWidget::mousePressEvent(QMouseEvent* e)
{
    m_dragging = true;
    m_dragStart = m_dragCurrent = e->pos();
    m_cropNorm = {};
    update();
}

void CropPreviewWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_dragging) return;
    m_dragCurrent = e->pos();
    update();
}

void CropPreviewWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_dragging) return;
    m_dragging = false;
    m_dragCurrent = e->pos();

    QRect ir = imageRect();
    QRect sel = QRect(m_dragStart, m_dragCurrent).normalized().intersected(ir);
    if (sel.width() > 8 && sel.height() > 8) {
        m_cropNorm = toNormalized(sel);
        emit cropChanged(m_cropNorm);
    } else {
        m_cropNorm = {};
    }
    update();
}

// ── CropDialog ────────────────────────────────────────────────────────────────

CropDialog::CropDialog(const QPixmap& pixmap, const QRectF& existing, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Crop Frame");
    setMinimumSize(600, 500);

    m_preview = new CropPreviewWidget(this);
    m_preview->setPixmap(pixmap);
    m_preview->setCropRect(existing);

    auto* infoLbl = new QLabel("Drag on the image to define the crop area.", this);
    infoLbl->setAlignment(Qt::AlignCenter);

    auto* applyFrameBtn = new QPushButton("Apply to This Frame", this);
    auto* applyAllBtn   = new QPushButton("Apply to All Frames", this);
    auto* clearBtn      = new QPushButton("Clear Crop", this);
    auto* cancelBtn     = new QPushButton("Cancel", this);

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(applyFrameBtn);
    btnRow->addWidget(applyAllBtn);
    btnRow->addStretch();
    btnRow->addWidget(clearBtn);
    btnRow->addWidget(cancelBtn);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_preview, 1);
    layout->addWidget(infoLbl);
    layout->addLayout(btnRow);

    connect(applyFrameBtn, &QPushButton::clicked, this, [this]() {
        m_applyToAll = false;
        accept();
    });
    connect(applyAllBtn, &QPushButton::clicked, this, [this]() {
        m_applyToAll = true;
        accept();
    });
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        m_preview->setCropRect({});
        m_applyToAll = true; // clear makes most sense globally
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QRectF CropDialog::cropRect() const
{
    return m_preview->cropRect();
}
