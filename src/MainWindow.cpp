#include "MainWindow.h"
#include "FilmstripWidget.h"
#include "PlaybackWidget.h"
#include "VideoExporter.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
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

    connect(m_fpsSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
        m_project->setFps(val);
        if (m_playing)
            m_playbackTimer->setInterval(1000 / val);
    });

    connect(m_playbackTimer, &QTimer::timeout, this, &MainWindow::onPlaybackTick);
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

    QString path = QFileDialog::getOpenFileName(this, "Open Project", {},
        "LSMS Project (*.lsms)");
    if (path.isEmpty())
        return;

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
    QString path = QFileDialog::getSaveFileName(this, "Save Project As", {},
        "LSMS Project (*.lsms)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".lsms", Qt::CaseInsensitive))
        path += ".lsms";
    if (!m_project->save(path))
        QMessageBox::critical(this, "Save Project", "Failed to save the project.");
    updateWindowTitle();
}

void MainWindow::onOpenImages()
{
    QFileDialog dialog(this, "Open Images");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter("Images (*.jpg *.jpeg *.png *.bmp *.tiff *.tif *.webp)");
    dialog.setOption(QFileDialog::DontUseNativeDialog);
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
    QString path = QFileDialog::getSaveFileName(this, "Export Video", {}, filter);
    if (path.isEmpty())
        return;

    QVector<QString> paths;
    paths.reserve(m_project->frameCount());
    for (int i = 0; i < m_project->frameCount(); ++i)
        paths.append(m_project->imagePath(i));

    VideoExporter exporter(this);
    if (exporter.exportVideo(paths, path, m_project->fps(), format, this))
        QMessageBox::information(this, "Export", "Video exported successfully.");
}

void MainWindow::onPlayPause()
{
    setPlaying(!m_playing);
}

void MainWindow::onStop()
{
    setPlaying(false);
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
    m_currentFrame = (m_currentFrame + 1) % m_project->frameCount();
    m_playbackWidget->showFrame(m_project->pixmap(m_currentFrame));

    m_filmstripWidget->blockSignals(true);
    m_filmstripWidget->selectFrame(m_currentFrame);
    m_filmstripWidget->blockSignals(false);
}

void MainWindow::refreshFilmstrip()
{
    m_filmstripWidget->populate(m_project->frameCount(), [this](int i) {
        return m_project->pixmap(i);
    });

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
    } else {
        m_playbackTimer->stop();
        m_playPauseBtn->setText("\u25B6 Play");
        updateOnionFrames(); // restore onion when paused
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
