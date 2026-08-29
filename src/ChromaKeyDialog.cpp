#include "ChromaKeyDialog.h"
#include "ChromaKeyProcessor.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

static constexpr int kPreviewW = 480;
static constexpr int kPreviewH = 320;

ChromaKeyDialog::ChromaKeyDialog(const QPixmap& frame,
                                  const ChromaKeySettings& current,
                                  QWidget* parent)
    : QDialog(parent)
    , m_settings(current)
{
    setWindowTitle("Chroma Key");
    setMinimumSize(780, 420);

    // Scale frame to preview size once
    m_sourceImage = frame.scaled(kPreviewW, kPreviewH,
                                  Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation).toImage();

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(200);
    connect(m_debounce, &QTimer::timeout, this, &ChromaKeyDialog::updatePreview);

    buildUi(current);
    updatePreview();

    // Allow clicking on preview to pick the key colour
    m_previewLabel->setCursor(Qt::CrossCursor);
    m_previewLabel->setToolTip("Click on the image to pick the key colour");
    m_previewLabel->installEventFilter(this);
}

void ChromaKeyDialog::buildUi(const ChromaKeySettings& s)
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left: preview
    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(kPreviewW, kPreviewH);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background:#111; border:1px solid #555;");
    mainLayout->addWidget(m_previewLabel, 0, Qt::AlignTop);

    // Right: controls
    auto* ctrl = new QWidget(this);
    auto* cl   = new QVBoxLayout(ctrl);
    cl->setSpacing(8);

    m_enabledBox = new QCheckBox("Enable chroma key", ctrl);
    m_enabledBox->setChecked(s.enabled);
    cl->addWidget(m_enabledBox);
    connect(m_enabledBox, &QCheckBox::toggled, this, &ChromaKeyDialog::schedulePreviewUpdate);

    // Key colour
    auto* colRow = new QHBoxLayout;
    colRow->addWidget(new QLabel("Key colour:", ctrl));
    m_keyColorBtn = new QPushButton(ctrl);
    m_keyColorBtn->setFixedSize(32, 24);
    colRow->addWidget(m_keyColorBtn);
    colRow->addStretch();
    cl->addLayout(colRow);
    connect(m_keyColorBtn, &QPushButton::clicked, this, &ChromaKeyDialog::pickKeyColor);

    // Sliders
    auto addSlider = [&](const QString& label, int min, int max, int val, QSlider*& out) {
        auto* row = new QHBoxLayout;
        row->addWidget(new QLabel(label + ":", ctrl));
        out = new QSlider(Qt::Horizontal, ctrl);
        out->setRange(min, max);
        out->setValue(val);
        row->addWidget(out, 1);
        auto* valLbl = new QLabel(QString::number(val), ctrl);
        valLbl->setFixedWidth(28);
        row->addWidget(valLbl);
        cl->addLayout(row);
        connect(out, &QSlider::valueChanged, this, [valLbl, this](int v) {
            valLbl->setText(QString::number(v));
            schedulePreviewUpdate();
        });
    };

    addSlider("Tolerance",    0, 255, s.tolerance,     m_tolSlider);
    addSlider("Feather",      0, 100, s.feather,        m_featherSlider);
    addSlider("Spill suppress",0,100, s.spillSuppress,  m_spillSlider);

    // Background
    auto* bgGroup = new QGroupBox("Background", ctrl);
    auto* bgl = new QVBoxLayout(bgGroup);

    m_bgTransparent = new QRadioButton("Transparent (alpha channel)", bgGroup);
    m_bgSolid       = new QRadioButton("Solid colour", bgGroup);
    m_bgImage       = new QRadioButton("Image file", bgGroup);
    bgl->addWidget(m_bgTransparent);

    auto* solidRow = new QHBoxLayout;
    solidRow->addWidget(m_bgSolid);
    m_bgColorBtn = new QPushButton(bgGroup);
    m_bgColorBtn->setFixedSize(32, 20);
    solidRow->addWidget(m_bgColorBtn);
    solidRow->addStretch();
    bgl->addLayout(solidRow);
    connect(m_bgColorBtn, &QPushButton::clicked, this, &ChromaKeyDialog::pickBgColor);

    auto* imgRow = new QHBoxLayout;
    imgRow->addWidget(m_bgImage);
    m_bgImageBtn = new QPushButton("Browse…", bgGroup);
    m_bgImageBtn->setFixedWidth(72);
    imgRow->addWidget(m_bgImageBtn);
    m_bgImageLabel = new QLabel("(none)", bgGroup);
    m_bgImageLabel->setWordWrap(false);
    imgRow->addWidget(m_bgImageLabel, 1);
    bgl->addLayout(imgRow);
    connect(m_bgImageBtn, &QPushButton::clicked, this, &ChromaKeyDialog::browseBgImage);

    cl->addWidget(bgGroup);

    auto* btnGroup = new QButtonGroup(this);
    btnGroup->addButton(m_bgTransparent, int(ChromaKeySettings::Transparent));
    btnGroup->addButton(m_bgSolid,       int(ChromaKeySettings::SolidColor));
    btnGroup->addButton(m_bgImage,       int(ChromaKeySettings::BackgroundImage));
    connect(btnGroup, &QButtonGroup::idClicked, this, [this](int) { schedulePreviewUpdate(); });

    cl->addStretch();

    // Action buttons
    auto* actRow = new QHBoxLayout;
    auto* applyFrameBtn = new QPushButton("Apply to This Frame", ctrl);
    auto* applyAllBtn   = new QPushButton("Apply to All Frames", ctrl);
    auto* clearBtn      = new QPushButton("Clear / Disable",     ctrl);
    auto* cancelBtn     = new QPushButton("Cancel",              ctrl);
    actRow->addWidget(applyFrameBtn);
    actRow->addWidget(applyAllBtn);
    actRow->addStretch();
    actRow->addWidget(clearBtn);
    actRow->addWidget(cancelBtn);
    cl->addLayout(actRow);

    connect(applyFrameBtn, &QPushButton::clicked, this, [this]() {
        settingsFromUi();
        m_applyToAll = false;
        accept();
    });
    connect(applyAllBtn, &QPushButton::clicked, this, [this]() {
        settingsFromUi();
        m_applyToAll = true;
        accept();
    });
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        m_settings = ChromaKeySettings{}; // disabled by default
        m_settings.enabled = false;
        m_applyToAll = true;
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    mainLayout->addWidget(ctrl, 1);

    uiFromSettings(s);
}

void ChromaKeyDialog::uiFromSettings(const ChromaKeySettings& s)
{
    m_enabledBox->setChecked(s.enabled);
    m_tolSlider->setValue(s.tolerance);
    m_featherSlider->setValue(s.feather);
    m_spillSlider->setValue(s.spillSuppress);

    // Key colour swatch
    m_keyColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #555;").arg(s.keyColor.name()));

    // Background
    switch (s.bgMode) {
        case ChromaKeySettings::Transparent:     m_bgTransparent->setChecked(true); break;
        case ChromaKeySettings::SolidColor:      m_bgSolid->setChecked(true);       break;
        case ChromaKeySettings::BackgroundImage: m_bgImage->setChecked(true);       break;
    }
    m_bgColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #555;").arg(s.bgColor.name()));
    if (!s.bgImagePath.isEmpty())
        m_bgImageLabel->setText(QFileInfo(s.bgImagePath).fileName());
}

void ChromaKeyDialog::settingsFromUi()
{
    m_settings.enabled      = m_enabledBox->isChecked();
    m_settings.tolerance    = m_tolSlider->value();
    m_settings.feather      = m_featherSlider->value();
    m_settings.spillSuppress = m_spillSlider->value();

    if (m_bgTransparent->isChecked())     m_settings.bgMode = ChromaKeySettings::Transparent;
    else if (m_bgSolid->isChecked())      m_settings.bgMode = ChromaKeySettings::SolidColor;
    else if (m_bgImage->isChecked())      m_settings.bgMode = ChromaKeySettings::BackgroundImage;
}

void ChromaKeyDialog::schedulePreviewUpdate()
{
    m_debounce->start();
}

void ChromaKeyDialog::updatePreview()
{
    settingsFromUi();

    if (!m_settings.enabled) {
        m_previewLabel->setPixmap(QPixmap::fromImage(m_sourceImage));
        return;
    }

    // Load & scale bg image for preview if needed
    QImage bgImg;
    if (m_settings.bgMode == ChromaKeySettings::BackgroundImage &&
        !m_settings.bgImagePath.isEmpty()) {
        bgImg = QImage(m_settings.bgImagePath)
                    .scaled(m_sourceImage.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QImage processed = ChromaKeyProcessor::process(m_sourceImage, m_settings, bgImg);

    // Show on checkerboard if transparent
    if (m_settings.bgMode == ChromaKeySettings::Transparent) {
        QImage bg(processed.size(), QImage::Format_RGB32);
        for (int y = 0; y < bg.height(); ++y)
            for (int x = 0; x < bg.width(); ++x)
                bg.setPixel(x, y, ((x/8 + y/8) % 2 == 0) ? 0xCCCCCC : 0x888888);
        QPainter p(&bg);
        p.drawImage(0, 0, processed);
        m_previewLabel->setPixmap(QPixmap::fromImage(bg));
    } else {
        m_previewLabel->setPixmap(QPixmap::fromImage(processed));
    }
}

void ChromaKeyDialog::pickKeyColor()
{
    QColor c = QColorDialog::getColor(m_settings.keyColor, this, "Key Colour");
    if (!c.isValid()) return;
    m_settings.keyColor = c;
    m_keyColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #555;").arg(c.name()));
    schedulePreviewUpdate();
}

bool ChromaKeyDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_previewLabel && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            // Map label coords to source image coords (image is centred in label)
            int offX = (m_previewLabel->width()  - m_sourceImage.width())  / 2;
            int offY = (m_previewLabel->height() - m_sourceImage.height()) / 2;
            int imgX = me->pos().x() - offX;
            int imgY = me->pos().y() - offY;
            if (imgX >= 0 && imgX < m_sourceImage.width() &&
                imgY >= 0 && imgY < m_sourceImage.height()) {
                QColor picked = m_sourceImage.pixelColor(imgX, imgY);
                m_settings.keyColor = picked;
                m_keyColorBtn->setStyleSheet(
                    QString("background-color: %1; border: 1px solid #555;").arg(picked.name()));
                m_enabledBox->setChecked(true);
                schedulePreviewUpdate();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void ChromaKeyDialog::pickBgColor()
{
    QColor c = QColorDialog::getColor(m_settings.bgColor, this, "Background Colour");
    if (!c.isValid()) return;
    m_settings.bgColor = c;
    m_bgColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #555;").arg(c.name()));
    m_bgSolid->setChecked(true);
    schedulePreviewUpdate();
}

void ChromaKeyDialog::browseBgImage()
{
    QString path = QFileDialog::getOpenFileName(this, "Background Image", {},
        "Images (*.jpg *.jpeg *.png *.bmp *.tiff *.tif *.webp)");
    if (path.isEmpty()) return;
    m_settings.bgImagePath = path;
    m_bgImageLabel->setText(QFileInfo(path).fileName());
    m_bgImage->setChecked(true);
    schedulePreviewUpdate();
}
