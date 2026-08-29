#pragma once

#include <QObject>
#include <QVector>

class QWidget;

class VideoExporter : public QObject
{
    Q_OBJECT
public:
    explicit VideoExporter(QObject* parent = nullptr);

    static bool isFfmpegAvailable();

    bool exportVideo(const QVector<QString>& imagePaths,
                     const QString& outputPath,
                     int fps,
                     const QString& format,
                     QWidget* parentWidget);
};
