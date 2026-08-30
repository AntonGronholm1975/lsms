#include "MainWindow.h"
#include "ChromaKeyDialog.h"
#include "ChromaKeySettings.h"
#include "CropDialog.h"
#include "FilmstripWidget.h"
#include "PlaybackWidget.h"
#include "VideoExporter.h"

#include <QAudioOutput>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMediaPlayer>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_project(new Project(this))
    , m_playbackTimer(new QTimer(this))
{
    resize(1200, 800);
    setupUi();
    setupMenuBar();
    connectSignals();
    updateWindowTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    m_playbackWidget = new PlaybackWidget(central);
    mainLayout->addWidget(m_playbackWidget, 1);

    // Transport bar
    auto* transport = new QWidget(central);
    transport->setFixedHeight(44);
    auto* tl = new QHBoxLayout(transport);
    tl->setContentsMargins(8, 4, 8, 4);
    tl->setSpacing(8);

    auto* stepBackBtn = new QPushButton("\u23EE", transport);
    stepBackBtn->setFixedWidth(44);
    stepBackBtn->setToolTip("Previous frame (Left)");
    connect(stepBackBtn, &QPushButton::clicked, this, &MainWindow::onStepBack);
    tl->addWidget(stepBackBtn);

    m_playPauseBtn = new QPushButton("\u25B6 Play", transport);
    m_playPauseBtn->setFixedWidth(90);
    m_playPauseBtn->setToolTip("Play / Pause (Space)");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    tl->addWidget(m_playPauseBtn);

    auto* stopBtn = new QPushButton("\u23F9 Stop", transport);
    stopBtn->setFixedWidth(80);
    stopBtn->setToolTip("Stop and go to first frame (Esc)");
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    tl->addWidget(stopBtn);

    auto* stepFwdBtn = new QPushButton("\u23ED", transport);
    stepFwdBtn->setFixedWidth(44);
    stepFwdBtn->setToolTip("Next frame (Right)");
    connect(stepFwdBtn, &QPushButton::clicked, this, &MainWindow::onStepForward);
    tl->addWidget(stepFwdBtn);

    tl->addStretch();

    m_onionBtn = new QPushButton("Onion", transport);
    m_onionBtn->setCheckable(true);
    m_onionBtn->setToolTip("Toggle onion skinning — shows previous frame(s) as a red ghost");
    m_onionBtn->setFixedWidth(60);
    connect(m_onionBtn, &QPushButton::toggled, this, [this](bool on) {
        m_playbackWidget->setOnionEnabled(on);
        updateOnionFrames();
    });
    tl->addWidget(m_onionBtn);

    tl->addWidget(new QLabel("FPS:", transport));
    m_fpsSpinBox = new QSpinBox(transport);
    m_fpsSpinBox->setRange(1, 60);
    m_fpsSpinBox->setValue(12);
    m_fpsSpinBox->setFixedWidth(60);
    tl->addWidget(m_fpsSpinBox);

    mainLayout->addWidget(transport);

    // Layers bar
    auto* layersBar = new QWidget(central);
    layersBar->setFixedHeight(34);
    auto* ll = new QHBoxLayout(layersBar);
    ll->setContentsMargins(8, 2, 8, 2);
    ll->setSpacing(6);

    ll->addWidget(new QLabel("Layers:", layersBar));

    m_cropLayerBtn = new QPushButton("Crop", layersBar);
    m_cropLayerBtn->setCheckable(true);
    m_cropLayerBtn->setChecked(true);
    m_cropLayerBtn->setFixedWidth(70);
    m_cropLayerBtn->setToolTip("Toggle crop layer on/off");
    ll->addWidget(m_cropLayerBtn);

    m_chromaLayerBtn = new QPushButton("Chroma Key", layersBar);
    m_chromaLayerBtn->setCheckable(true);
    m_chromaLayerBtn->setChecked(true);
    m_chromaLayerBtn->setFixedWidth(90);
    m_chromaLayerBtn->setToolTip("Toggle chroma key layer on/off");
    ll->addWidget(m_chromaLayerBtn);

    ll->addStretch();
    mainLayout->addWidget(layersBar);

    // Drawing toolbar
    auto* drawBar = new QWidget(central);
    drawBar->setFixedHeight(40);
    auto* dl = new QHBoxLayout(drawBar);
    dl->setContentsMargins(8, 2, 8, 2);
    dl->setSpacing(4);

    m_drawModeBtn = new QPushButton("\u270F Draw", drawBar);
    m_drawModeBtn->setCheckable(true);
    m_drawModeBtn->setFixedWidth(72);
    m_drawModeBtn->setToolTip("Toggle drawing mode");
    dl->addWidget(m_drawModeBtn);

    dl->addWidget(new QLabel("|", drawBar));

    // Exclusive tool buttons
    m_toolGroup = new QButtonGroup(this);
    auto addTool = [&](const QString& label, const QString& tip, DrawTool tool) {
        auto* btn = new QPushButton(label, drawBar);
        btn->setCheckable(true);
        btn->setFixedWidth(60);
        btn->setToolTip(tip);
        btn->setEnabled(false);
        m_toolGroup->addButton(btn, int(tool));
        dl->addWidget(btn);
        return btn;
    };
    addTool("Pen",     "Freehand pen",   DrawTool::Pen);
    addTool("Line",    "Straight line",  DrawTool::Line);
    addTool("Rect",    "Rectangle",      DrawTool::Rectangle);
    addTool("Ellipse", "Ellipse",        DrawTool::Ellipse);
    addTool("Eraser",  "Eraser",         DrawTool::Eraser);
    m_toolGroup->button(int(DrawTool::Pen))->setChecked(true);

    dl->addWidget(new QLabel("|", drawBar));

    m_colorBtn = new QPushButton(drawBar);
    m_colorBtn->setFixedSize(28, 28);
    m_colorBtn->setStyleSheet("background-color: red; border: 1px solid #888;");
    m_colorBtn->setToolTip("Pick drawing colour");
    m_colorBtn->setEnabled(false);
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(Qt::red, this, "Drawing Colour");
        if (c.isValid()) {
            m_playbackWidget->setDrawColor(c);
            m_colorBtn->setStyleSheet(
                QString("background-color: %1; border: 1px solid #888;").arg(c.name()));
        }
    });
    dl->addWidget(m_colorBtn);

    dl->addWidget(new QLabel("Size:", drawBar));
    m_brushSpinBox = new QSpinBox(drawBar);
    m_brushSpinBox->setRange(1, 40);
    m_brushSpinBox->setValue(4);
    m_brushSpinBox->setFixedWidth(52);
    m_brushSpinBox->setEnabled(false);
    connect(m_brushSpinBox, &QSpinBox::valueChanged, m_playbackWidget, &PlaybackWidget::setBrushSize);
    dl->addWidget(m_brushSpinBox);

    dl->addStretch();

    auto* clearFrameBtn = new QPushButton("Clear Frame", drawBar);
    clearFrameBtn->setEnabled(false);
    clearFrameBtn->setToolTip("Clear annotations on this frame");
    connect(clearFrameBtn, &QPushButton::clicked, this, [this]() {
        m_playbackWidget->clearAnnotation();
        if (m_currentFrame < m_annotations.size())
            m_annotations[m_currentFrame] = {};
    });
    dl->addWidget(clearFrameBtn);

    auto* clearAllBtn = new QPushButton("Clear All", drawBar);
    clearAllBtn->setEnabled(false);
    clearAllBtn->setToolTip("Clear annotations on all frames");
    connect(clearAllBtn, &QPushButton::clicked, this, [this, clearFrameBtn]() {
        m_annotations.fill({});
        m_playbackWidget->clearAnnotation();
    });
    dl->addWidget(clearAllBtn);

    // Wire draw mode toggle to enable/disable the rest of the toolbar
    connect(m_drawModeBtn, &QPushButton::toggled, this, [this, clearFrameBtn, clearAllBtn](bool on) {
        syncDrawingToolbar(on);
        clearFrameBtn->setEnabled(on);
        clearAllBtn->setEnabled(on);
        m_playbackWidget->setDrawingEnabled(on);
    });

    // Wire tool selection
    connect(m_toolGroup, &QButtonGroup::idClicked, this, [this](int id) {
        m_playbackWidget->setDrawTool(DrawTool(id));
    });

    mainLayout->addWidget(drawBar);

    // Audio bar
    auto* audioBar = new QWidget(central);
    audioBar->setFixedHeight(34);
    auto* al = new QHBoxLayout(audioBar);
    al->setContentsMargins(8, 2, 8, 2);
    al->setSpacing(6);
    al->addWidget(new QLabel("\U0001F3B5 Audio:", audioBar));
    auto* loadAudioBtn = new QPushButton("Load\u2026", audioBar);
    loadAudioBtn->setFixedWidth(58);
    connect(loadAudioBtn, &QPushButton::clicked, this, &MainWindow::onLoadAudio);
    al->addWidget(loadAudioBtn);
    m_audioLabel = new QLabel("(none)", audioBar);
    m_audioLabel->setMinimumWidth(120);
    al->addWidget(m_audioLabel, 1);
    al->addStretch();
    mainLayout->addWidget(audioBar);

    // Set up audio player
    m_audioPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.8f);
    m_audioPlayer->setAudioOutput(m_audioOutput);

    m_filmstripWidget = new FilmstripWidget(central);
    mainLayout->addWidget(m_filmstripWidget);
}

void MainWindow::setupMenuBar()
{
    QMenu* file = menuBar()->addMenu("&File");

    auto* newAct = file->addAction("&New Project");
    newAct->setShortcut(QKeySequence::New);
    connect(newAct, &QAction::triggered, this, &MainWindow::onNewProject);

    auto* openAct = file->addAction("&Open Project\u2026");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenProject);

    auto* saveAct = file->addAction("&Save Project");
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onSaveProject);

    auto* saveAsAct = file->addAction("Save Project &As\u2026");
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::onSaveProjectAs);

    file->addSeparator();

    auto* openImgAct = file->addAction("Open &Images\u2026");
    openImgAct->setShortcut(QKeySequence("Ctrl+I"));
    connect(openImgAct, &QAction::triggered, this, &MainWindow::onOpenImages);

    file->addSeparator();

    auto* exitAct = file->addAction("E&xit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    QMenu* edit = menuBar()->addMenu("&Edit");

    auto* delAct = edit->addAction("&Delete Frame");
    delAct->setShortcut(QKeySequence::Delete);
    connect(delAct, &QAction::triggered, this, [this]() {
        onDeleteFrame(m_currentFrame);
    });

    auto* dupAct = edit->addAction("D&uplicate Frame");
    dupAct->setShortcut(QKeySequence("Ctrl+D"));
    connect(dupAct, &QAction::triggered, this, [this]() {
        onDuplicateFrame(m_currentFrame);
    });

    edit->addSeparator();

    auto* cropAct = edit->addAction("&Crop Frame\u2026");
    cropAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(cropAct, &QAction::triggered, this, &MainWindow::onCropFrame);

    auto* chromaAct = edit->addAction("C&hroma Key\u2026");
    chromaAct->setShortcut(QKeySequence("Ctrl+H"));
    connect(chromaAct, &QAction::triggered, this, &MainWindow::onChromaKey);

    QMenu* playback = menuBar()->addMenu("&Playback");

    auto* playAct = playback->addAction("&Play / Pause");
    playAct->setShortcut(Qt::Key_Space);
    connect(playAct, &QAction::triggered, this, &MainWindow::onPlayPause);

    auto* stopAct = playback->addAction("&Stop");
    stopAct->setShortcut(Qt::Key_Escape);
    connect(stopAct, &QAction::triggered, this, &MainWindow::onStop);

    playback->addSeparator();

    auto* prevAct = playback->addAction("&Previous Frame");
    prevAct->setShortcut(Qt::Key_Left);
    connect(prevAct, &QAction::triggered, this, &MainWindow::onStepBack);

    auto* nextAct = playback->addAction("&Next Frame");
    nextAct->setShortcut(Qt::Key_Right);
    connect(nextAct, &QAction::triggered, this, &MainWindow::onStepForward);

    QMenu* exportMenu = menuBar()->addMenu("&Export");

    auto* mp4Act = exportMenu->addAction("Export as &MP4\u2026");
    connect(mp4Act, &QAction::triggered, this, [this]() { onExport("mp4"); });

    auto* webmAct = exportMenu->addAction("Export as &WebM\u2026");
    connect(webmAct, &QAction::triggered, this, [this]() { onExport("webm"); });

    QMenu* helpMenu = menuBar()->addMenu("&Help");

    auto* shortcutsAct = helpMenu->addAction("&Keyboard Shortcuts");
    connect(shortcutsAct, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, "Keyboard Shortcuts",
            "Space          Play / Pause\n"
            "Esc            Stop (return to frame 1)\n"
            "\u2190 / \u2192        Previous / Next frame\n"
            "Del            Delete selected frame\n"
            "Ctrl+D         Duplicate selected frame\n"
            "Ctrl+I         Open Images\u2026\n"
            "Ctrl+S         Save Project\n"
            "Ctrl+Shift+S   Save Project As\u2026\n"
            "Ctrl+O         Open Project\u2026\n"
            "Ctrl+N         New Project");
    });

    helpMenu->addSeparator();

    auto* aboutAct = helpMenu->addAction("&About");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About Luna\u2019s Stop Motion Studio",
            "<h2>Luna\u2019s Stop Motion Studio</h2>"
            "<p>Version 0.1</p>"
            "<p>A simple stop motion animation tool.</p>"
            "<p>Open images from disk, arrange them in the filmstrip, "
            "preview playback, and export to MP4 or WebM via ffmpeg.</p>"
            "<p><a href=\"https://github.com/AntonGronholm1975/lsms\">"
            "github.com/AntonGronholm1975/lsms</a></p>");
    });
}

void MainWindow::connectSignals()
{
    connect(m_project, &Project::framesChanged, this, &MainWindow::refreshFilmstrip);
    connect(m_project, &Project::fpsChanged, this, &MainWindow::onFpsChanged);
    connect(m_project, &Project::modifiedChanged, this, &MainWindow::onProjectModifiedChanged);

    connect(m_filmstripWidget, &FilmstripWidget::frameSelected, this, &MainWindow::onFrameSelected);
    connect(m_filmstripWidget, &FilmstripWidget::frameMoved, this, &MainWindow::onFrameMoved);
    connect(m_filmstripWidget, &FilmstripWidget::deleteFrameRequested, this, &MainWindow::onDeleteFrame);
    connect(m_filmstripWidget, &FilmstripWidget::duplicateFrameRequested, this, &MainWindow::onDuplicateFrame);
    connect(m_filmstripWidget, &FilmstripWidget::frameDurationChangeRequested,
            this, &MainWindow::onFrameDurationChange);

    connect(m_fpsSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
        m_project->setFps(val);
        if (m_playing)
            m_playbackTimer->setInterval(1000 / val);
    });

    connect(m_playbackTimer, &QTimer::timeout, this, &MainWindow::onPlaybackTick);
    connect(m_playbackWidget, &PlaybackWidget::annotationChanged,
            this, &MainWindow::onAnnotationChanged);

    connect(m_cropLayerBtn, &QPushButton::toggled,
            m_project, &Project::setCropLayerEnabled);
    connect(m_chromaLayerBtn, &QPushButton::toggled,
            m_project, &Project::setChromaLayerEnabled);

    connect(m_project, &Project::layerVisibilityChanged, this, [this]() {
        m_cropLayerBtn->blockSignals(true);
        m_cropLayerBtn->setChecked(m_project->cropLayerEnabled());
        m_cropLayerBtn->blockSignals(false);
        m_chromaLayerBtn->blockSignals(true);
        m_chromaLayerBtn->setChecked(m_project->chromaLayerEnabled());
        m_chromaLayerBtn->blockSignals(false);
        m_playbackWidget->setCropLayerEnabled(m_project->cropLayerEnabled());
        m_playbackWidget->setChromaLayerEnabled(m_project->chromaLayerEnabled());
    });
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmDiscardChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::onNewProject()
{
    if (!confirmDiscardChanges())
        return;
    setPlaying(false);
    m_currentFrame = 0;
    m_project->reset();
    m_filmstripWidget->clear();
    m_playbackWidget->clear();
    updateWindowTitle();
}

void MainWindow::onOpenProject()
{
    if (!confirmDiscardChanges())
        return;

    QString path = QFileDialog::getOpenFileName(this, "Open Project",
        QSettings().value("lastProjectDir").toString(),
        "LSMS Project (*.lsms)");
    if (path.isEmpty())
        return;

    QSettings().setValue("lastProjectDir", QFileInfo(path).absolutePath());
    setPlaying(false);
    m_currentFrame = 0;

    if (!m_project->load(path))
        QMessageBox::critical(this, "Open Project", "Failed to load the project file.");

    updateWindowTitle();
}

void MainWindow::onSaveProject()
{
    if (m_project->filePath().isEmpty())
        onSaveProjectAs();
    else if (!m_project->save(m_project->filePath()))
        QMessageBox::critical(this, "Save Project", "Failed to save the project.");
    updateWindowTitle();
}

void MainWindow::onSaveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Project As",
        QSettings().value("lastProjectDir").toString(),
        "LSMS Project (*.lsms)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".lsms", Qt::CaseInsensitive))
        path += ".lsms";
    if (!m_project->save(path))
        QMessageBox::critical(this, "Save Project", "Failed to save the project.");
    else
        QSettings().setValue("lastProjectDir", QFileInfo(path).absolutePath());
    updateWindowTitle();
}

void MainWindow::onOpenImages()
{
    QFileDialog dialog(this, "Open Images");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter("Images (*.jpg *.jpeg *.png *.bmp *.tiff *.tif *.webp)");
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setDirectory(QSettings().value("lastImageDir").toString());
    dialog.resize(960, 540);

    auto* preview = new QLabel(&dialog);
    preview->setMinimumSize(200, 200);
    preview->setMaximumWidth(240);
    preview->setAlignment(Qt::AlignCenter);
    preview->setText("No preview");
    preview->setStyleSheet("color:#888; border:1px solid #aaa; background:#1a1a1a;");

    // Append preview to the right of the existing dialog grid
    auto* grid = qobject_cast<QGridLayout*>(dialog.layout());
    if (grid)
        grid->addWidget(preview, 0, grid->columnCount(), grid->rowCount(), 1);

    connect(&dialog, &QFileDialog::currentChanged, preview, [preview](const QString& path) {
        QPixmap px(path);
        if (!px.isNull())
            preview->setPixmap(
                px.scaled(preview->width(), preview->height(),
                          Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else
            preview->setText("No preview");
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    QStringList paths = dialog.selectedFiles();
    if (paths.isEmpty())
        return;
    QSettings().setValue("lastImageDir", QFileInfo(paths.first()).absolutePath());
    paths.sort();
    m_project->addImages(paths);
}

void MainWindow::onExport(const QString& format)
{
    if (m_project->frameCount() == 0) {
        QMessageBox::information(this, "Export", "There are no frames to export.");
        return;
    }

    if (!VideoExporter::isFfmpegAvailable()) {
        QMessageBox::critical(this, "Export",
            "ffmpeg was not found on your PATH.\n\n"
            "Please install ffmpeg and make sure it is accessible from the command line.");
        return;
    }

    QString filter = (format == "mp4") ? "MP4 Video (*.mp4)" : "WebM Video (*.webm)";
    QString ext     = (format == "mp4") ? ".mp4" : ".webm";
    QString path = QFileDialog::getSaveFileName(this, "Export Video",
        QSettings().value("lastExportDir").toString(), filter);
    if (path.isEmpty())
        return;
    if (!path.endsWith(ext, Qt::CaseInsensitive))
        path += ext;

    QSettings().setValue("lastExportDir", QFileInfo(path).absolutePath());

    QVector<QString> paths;
    QVector<QRectF>  cropRects;
    QVector<ChromaKeySettings> chromaRects;
    QVector<int>     durations;
    paths.reserve(m_project->frameCount());
    for (int i = 0; i < m_project->frameCount(); ++i) {
        paths.append(m_project->imagePath(i));
        cropRects.append(m_project->cropRect(i));
        chromaRects.append(m_project->chromaSettings(i));
        durations.append(m_project->frameDuration(i));
    }

    VideoExporter exporter(this);
    if (exporter.exportVideo(paths, cropRects, chromaRects, durations,
                             m_project->audioFilePath(), path,
                             m_project->fps(), format,
                             m_project->cropLayerEnabled(), m_project->chromaLayerEnabled(), this))
        QMessageBox::information(this, "Export",
            QString("Video exported successfully.\n\n%1").arg(path));
}

void MainWindow::onCropFrame()
{
    if (m_project->frameCount() == 0) return;
    CropDialog dlg(m_project->pixmap(m_currentFrame),
                   m_project->cropRect(m_currentFrame), this);
    if (dlg.exec() != QDialog::Accepted) return;
    if (dlg.applyToAll())
        m_project->setCropRectAllFrames(dlg.cropRect());
    else
        m_project->setCropRect(m_currentFrame, dlg.cropRect());
    m_playbackWidget->setCropRect(m_project->cropRect(m_currentFrame));
    updateOnionFrames();
}

void MainWindow::onChromaKey()
{
    if (m_project->frameCount() == 0) return;
    ChromaKeyDialog dlg(m_project->pixmap(m_currentFrame),
                        m_project->chromaSettings(m_currentFrame), this);
    if (dlg.exec() != QDialog::Accepted) return;
    if (dlg.applyToAll())
        m_project->setChromaSettingsAllFrames(dlg.settings());
    else
        m_project->setChromaSettings(m_currentFrame, dlg.settings());
    m_playbackWidget->setChromaSettings(m_project->chromaSettings(m_currentFrame));
}

void MainWindow::syncDrawingToolbar(bool enabled)
{
    for (QAbstractButton* btn : m_toolGroup->buttons())
        btn->setEnabled(enabled);
    m_colorBtn->setEnabled(enabled);
    m_brushSpinBox->setEnabled(enabled);
}

void MainWindow::onAnnotationChanged(const QPixmap& layer)
{
    if (m_currentFrame < m_annotations.size())
        m_annotations[m_currentFrame] = layer;
}

void MainWindow::onLoadAudio()
{
    QString path = QFileDialog::getOpenFileName(this, "Load Audio File",
        QSettings().value("lastAudioDir").toString(),
        "Audio (*.mp3 *.wav *.ogg *.flac *.aac *.m4a)");
    if (path.isEmpty()) return;
    QSettings().setValue("lastAudioDir", QFileInfo(path).absolutePath());
    m_project->setAudioFilePath(path);
    m_audioPlayer->setSource(QUrl::fromLocalFile(path));
    m_audioLabel->setText(QFileInfo(path).fileName());
}

void MainWindow::onFrameDurationChange(int index)
{
    if (index < 0 || index >= m_project->frameCount()) return;
    bool ok;
    int dur = QInputDialog::getInt(this, "Frame Duration",
        QString("Hold duration for frame %1\n(multiples of 1\u2215fps):").arg(index + 1),
        m_project->frameDuration(index), 1, 60, 1, &ok);
    if (!ok) return;
    m_project->setFrameDuration(index, dur);
    refreshFilmstrip();
}

void MainWindow::onPlayPause()
{
    setPlaying(!m_playing);
}

void MainWindow::onStop()
{
    setPlaying(false);
    m_ticksOnCurrentFrame = 0;
    m_audioPlayer->stop();
    if (m_project->frameCount() > 0) {
        showFrame(0);
        m_filmstripWidget->blockSignals(true);
        m_filmstripWidget->selectFrame(0);
        m_filmstripWidget->blockSignals(false);
    }
}

void MainWindow::onStepBack()
{
    if (m_project->frameCount() == 0)
        return;
    setPlaying(false);
    int next = (m_currentFrame - 1 + m_project->frameCount()) % m_project->frameCount();
    showFrame(next);
    m_filmstripWidget->blockSignals(true);
    m_filmstripWidget->selectFrame(next);
    m_filmstripWidget->blockSignals(false);
}

void MainWindow::onStepForward()
{
    if (m_project->frameCount() == 0)
        return;
    setPlaying(false);
    int next = (m_currentFrame + 1) % m_project->frameCount();
    showFrame(next);
    m_filmstripWidget->blockSignals(true);
    m_filmstripWidget->selectFrame(next);
    m_filmstripWidget->blockSignals(false);
}

void MainWindow::onFpsChanged(int fps)
{
    m_fpsSpinBox->blockSignals(true);
    m_fpsSpinBox->setValue(fps);
    m_fpsSpinBox->blockSignals(false);
    if (m_playing)
        m_playbackTimer->setInterval(1000 / fps);
}

void MainWindow::onFrameSelected(int index)
{
    if (index != m_currentFrame)
        showFrame(index);
}

void MainWindow::onFrameMoved(int from, int to)
{
    m_currentFrame = to; // follow the moved frame before refresh
    m_project->moveFrame(from, to);
    // refreshFilmstrip is triggered by framesChanged
}

void MainWindow::onDeleteFrame(int index)
{
    if (index < 0 || index >= m_project->frameCount())
        return;
    m_currentFrame = qMin(index, m_project->frameCount() - 2);
    if (m_currentFrame < 0)
        m_currentFrame = 0;
    m_project->removeFrame(index);
}

void MainWindow::onDuplicateFrame(int index)
{
    if (index < 0 || index >= m_project->frameCount())
        return;
    m_currentFrame = index + 1;
    m_project->duplicateFrame(index);
}

void MainWindow::onProjectModifiedChanged(bool)
{
    updateWindowTitle();
}

void MainWindow::onPlaybackTick()
{
    if (m_project->frameCount() == 0) {
        setPlaying(false);
        return;
    }
    // Hold the current frame for its duration (in timer ticks)
    if (++m_ticksOnCurrentFrame < m_project->frameDuration(m_currentFrame))
        return;
    m_ticksOnCurrentFrame = 0;

    m_currentFrame = (m_currentFrame + 1) % m_project->frameCount();
    m_playbackWidget->showFrame(m_project->pixmap(m_currentFrame));
    m_playbackWidget->setCropRect(m_project->cropRect(m_currentFrame));
    m_playbackWidget->setChromaSettings(m_project->chromaSettings(m_currentFrame));

    m_filmstripWidget->blockSignals(true);
    m_filmstripWidget->selectFrame(m_currentFrame);
    m_filmstripWidget->blockSignals(false);
}

void MainWindow::refreshFilmstrip()
{
    m_filmstripWidget->populate(m_project->frameCount(),
        [this](int i) { return m_project->pixmap(i); },
        [this](int i) { return m_project->frameDuration(i); });

    // Keep annotation vector in sync with frame count
    m_annotations.resize(m_project->frameCount());

    if (m_project->frameCount() == 0) {
        m_currentFrame = 0;
        m_playbackWidget->clear();
        return;
    }

    m_currentFrame = qBound(0, m_currentFrame, m_project->frameCount() - 1);

    m_filmstripWidget->blockSignals(true);
    m_filmstripWidget->selectFrame(m_currentFrame);
    m_filmstripWidget->blockSignals(false);

    showFrame(m_currentFrame);
}

void MainWindow::showFrame(int index)
{
    if (index < 0 || index >= m_project->frameCount())
        return;
    m_currentFrame = index;
    m_playbackWidget->showFrame(m_project->pixmap(index));
    m_playbackWidget->setCropRect(m_project->cropRect(index));
    m_playbackWidget->setChromaSettings(m_project->chromaSettings(index));
    m_playbackWidget->setAnnotation(
        index < m_annotations.size() ? m_annotations.at(index) : QPixmap{});
    updateOnionFrames();
}

void MainWindow::updateOnionFrames()
{
    if (!m_onionBtn->isChecked() || m_playing) {
        m_playbackWidget->setOnionFrames({});
        return;
    }
    QVector<QPixmap> prev;
    for (int i = 1; i <= 2; ++i) {
        int idx = m_currentFrame - i;
        if (idx >= 0)
            prev.append(m_project->pixmap(idx));
    }
    m_playbackWidget->setOnionFrames(prev);
}

void MainWindow::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    if (playing && m_project->frameCount() == 0)
        return;
    m_playing = playing;
    if (m_playing) {
        m_playbackTimer->start(1000 / m_project->fps());
        m_playPauseBtn->setText("\u23F8 Pause");
        m_playbackWidget->setOnionFrames({}); // hide onion during playback
        // Seek audio to position of current frame and start
        if (m_audioPlayer->source().isValid()) {
            double msPerTick = 1000.0 / m_project->fps();
            qint64 audioMs = 0;
            for (int i = 0; i < m_currentFrame; ++i)
                audioMs += m_project->frameDuration(i) * msPerTick;
            m_audioPlayer->setPosition(audioMs);
            m_audioPlayer->play();
        }
    } else {
        m_playbackTimer->stop();
        m_playPauseBtn->setText("\u25B6 Play");
        updateOnionFrames(); // restore onion when paused
        m_audioPlayer->pause();
    }
}

void MainWindow::updateWindowTitle()
{
    QString name = m_project->filePath().isEmpty()
        ? "Untitled"
        : QFileInfo(m_project->filePath()).completeBaseName();

    setWindowTitle(QString("Luna\u2019s Stop Motion Studio \u2014 %1%2")
        .arg(name, m_project->isModified() ? " *" : ""));
}

bool MainWindow::confirmDiscardChanges()
{
    if (!m_project->isModified())
        return true;

    int ret = QMessageBox::question(this, "Unsaved Changes",
        "You have unsaved changes. Do you want to save before continuing?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (ret == QMessageBox::Save) {
        onSaveProject();
        return !m_project->isModified();
    }
    return ret == QMessageBox::Discard;
}
