#pragma once

#include <QObject>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QVector>
#include "ChromaKeySettings.h"

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject* parent = nullptr);

    int     frameCount() const;
    QPixmap pixmap(int index) const;
    QString imagePath(int index) const;
    QRectF  cropRect(int index) const;
    ChromaKeySettings chromaSettings(int index) const;
    int     fps() const;
    QString filePath() const;
    bool    isModified() const;

    void addImages(const QStringList& paths);
    void removeFrame(int index);
    void duplicateFrame(int index);
    void moveFrame(int from, int to);
    void setFps(int fps);
    void setCropRect(int index, const QRectF& rect);
    void setCropRectAllFrames(const QRectF& rect);
    void setChromaSettings(int index, const ChromaKeySettings& s);
    void setChromaSettingsAllFrames(const ChromaKeySettings& s);

    // Per-frame hold duration (multiples of 1/fps)
    int  frameDuration(int index) const;
    void setFrameDuration(int index, int ticks);
    void setFrameDurationAllFrames(int ticks);

    // Optional background audio file path
    QString audioFilePath() const;
    void    setAudioFilePath(const QString& path);

    bool cropLayerEnabled()  const;
    bool chromaLayerEnabled() const;
    void setCropLayerEnabled(bool enabled);
    void setChromaLayerEnabled(bool enabled);

    bool save(const QString& path);
    bool load(const QString& path);
    void reset();

signals:
    void framesChanged();
    void fpsChanged(int fps);
    void modifiedChanged(bool modified);
    void layerVisibilityChanged();

private:
    void setModified(bool modified);

    QVector<QString>          m_imagePaths;
    QVector<QPixmap>          m_pixmaps;
    QVector<QRectF>           m_cropRects;
    QVector<ChromaKeySettings> m_chromaSettings;
    QVector<int>              m_frameDurations;
    QString                   m_audioFilePath;
    bool    m_cropLayerEnabled   = true;
    bool    m_chromaLayerEnabled  = true;
    int     m_fps = 12;
    QString m_filePath;
    bool    m_modified = false;
};
