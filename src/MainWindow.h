#pragma once

#include <QMainWindow>
#include <QTimer>
#include "Project.h"

class PlaybackWidget;
class FilmstripWidget;
class QSpinBox;
class QPushButton;

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

private:
    void setupUi();
    void setupMenuBar();
    void connectSignals();
    void refreshFilmstrip();
    void showFrame(int index);
    void setPlaying(bool playing);
    void updateWindowTitle();
    bool confirmDiscardChanges();

    Project* m_project;
    PlaybackWidget* m_playbackWidget;
    FilmstripWidget* m_filmstripWidget;
    QPushButton* m_playPauseBtn;
    QSpinBox* m_fpsSpinBox;
    QTimer* m_playbackTimer;
    int m_currentFrame = 0;
    bool m_playing = false;
};
