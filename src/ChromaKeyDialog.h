#pragma once

#include "ChromaKeySettings.h"
#include <QDialog>
#include <QImage>
#include <QPixmap>

class QCheckBox;
class QLabel;
class QRadioButton;
class QSlider;
class QPushButton;
class QTimer;

class ChromaKeyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChromaKeyDialog(const QPixmap& frame,
                             const ChromaKeySettings& current,
                             QWidget* parent = nullptr);

    ChromaKeySettings settings() const { return m_settings; }
    bool applyToAll() const { return m_applyToAll; }

private slots:
    void schedulePreviewUpdate();
    void updatePreview();
    void pickKeyColor();
    void pickBgColor();
    void browseBgImage();

private:
    void buildUi(const ChromaKeySettings& initial);
    void settingsFromUi();
    void uiFromSettings(const ChromaKeySettings& s);

    QImage           m_sourceImage; // frame at dialog preview size
    ChromaKeySettings m_settings;
    bool              m_applyToAll = false;

    // Preview
    QLabel*      m_previewLabel;

    // Controls
    QCheckBox*   m_enabledBox;
    QPushButton* m_keyColorBtn;
    QSlider*     m_tolSlider;
    QSlider*     m_featherSlider;
    QSlider*     m_spillSlider;
    QRadioButton* m_bgTransparent;
    QRadioButton* m_bgSolid;
    QRadioButton* m_bgImage;
    QPushButton*  m_bgColorBtn;
    QPushButton*  m_bgImageBtn;
    QLabel*       m_bgImageLabel;

    QTimer* m_debounce;
};
