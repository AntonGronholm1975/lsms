#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QTimer>
#include <QVector>
#include "Project.h"

class PlaybackWidget;
class FilmstripWidget;
class QSpinBox;
class QPushButton;
class QButtonGroup;
class QLabel;
class QMediaPlayer;
class QAudioOutput;
class QSlider;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onOpenImages();
    void onExport(const QString& format);
    void onCropFrame();
    void onChromaKey();

    void onPlayPause();
    void onStop();
    void onStepBack();
    void onStepForward();
    void onFpsChanged(int fps);

    void onFrameSelected(int index);
    void onFrameMoved(int from, int to);
    void onDeleteFrame(int index);
    void onDuplicateFrame(int index);

    void onProjectModifiedChanged(bool modified);
    void onPlaybackTick();
    void onAnnotationChanged(const QPixmap& layer);
    void onLoadAudio();
    void onFrameDurationChange(int index);

private:
    void setupUi();
    void setupMenuBar();
    void connectSignals();
    void refreshFilmstrip();
    void showFrame(int index);
    void setPlaying(bool playing);
    void updateWindowTitle();
    void updateOnionFrames();
    void syncDrawingToolbar(bool enabled);
    bool confirmDiscardChanges();

    Project*         m_project;
    PlaybackWidget*  m_playbackWidget;
    FilmstripWidget* m_filmstripWidget;
    QPushButton*     m_playPauseBtn;
    QPushButton*     m_onionBtn;
    QSpinBox*        m_fpsSpinBox;
    QTimer*          m_playbackTimer;
    int  m_currentFrame = 0;
    bool m_playing      = false;

    // Per-frame playback tick counter
    int m_ticksOnCurrentFrame = 0;

    // Audio
    QMediaPlayer* m_audioPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QLabel*       m_audioLabel  = nullptr;

    // Layers toolbar
    QPushButton* m_cropLayerBtn;
    QPushButton* m_chromaLayerBtn;

    // Drawing toolbar
    QPushButton*  m_drawModeBtn;
    QPushButton*  m_colorBtn;
    QSpinBox*     m_brushSpinBox;
    QButtonGroup* m_toolGroup;

    // Per-frame annotation layers (in-memory, not yet persisted)
    QVector<QPixmap> m_annotations;
};
