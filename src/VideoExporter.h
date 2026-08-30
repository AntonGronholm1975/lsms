#pragma once

#include <QObject>
#include <QRectF>
#include <QVector>
#include "ChromaKeySettings.h"

class QWidget;

class VideoExporter : public QObject
{
    Q_OBJECT
public:
    explicit VideoExporter(QObject* parent = nullptr);

    static bool isFfmpegAvailable();

    bool exportVideo(const QVector<QString>&          imagePaths,
                     const QVector<QRectF>&            cropRects,
                     const QVector<ChromaKeySettings>& chromaSettings,
                     const QVector<int>&               frameDurations,
                     const QString&                    audioFilePath,
                     const QString& outputPath,
                     int fps,
                     const QString& format,
                     bool cropLayerEnabled,
                     bool chromaLayerEnabled,
                     QWidget* parentWidget);
};
