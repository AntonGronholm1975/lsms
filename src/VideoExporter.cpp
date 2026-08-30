#include "VideoExporter.h"
#include "ChromaKeyProcessor.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>
#include <QUrl>
#include <QWidget>

VideoExporter::VideoExporter(QObject* parent)
    : QObject(parent)
{
}

bool VideoExporter::isFfmpegAvailable()
{
    QProcess proc;
    proc.start("ffmpeg", {"-version"});
    proc.waitForFinished(3000);
    // error() == UnknownError means the process started correctly
    return proc.error() == QProcess::UnknownError && proc.exitCode() == 0;
}

bool VideoExporter::exportVideo(const QVector<QString>& imagePaths,
                                 const QVector<QRectF>&  cropRects,
                                 const QVector<ChromaKeySettings>& chromaSettings,
                                 const QVector<int>&     frameDurations,
                                 const QString&          audioFilePath,
                                 const QString& outputPath,
                                 int fps,
                                 const QString& format,
                                 bool cropLayerEnabled,
                                 bool chromaLayerEnabled,
                                 QWidget* parentWidget)
{
    if (imagePaths.isEmpty()) {
        QMessageBox::warning(parentWidget, "Export", "No frames to export.");
        return false;
    }

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        QMessageBox::critical(parentWidget, "Export", "Could not create temporary directory.");
        return false;
    }

    QString ext = QFileInfo(imagePaths.first()).suffix().toLower();
    if (ext.isEmpty())
        ext = "jpg";

    int frameNum = 1;
    for (int i = 0; i < imagePaths.size(); ++i) {
        QRectF crop = (i < cropRects.size()) ? cropRects.at(i) : QRectF();
        ChromaKeySettings ck = (i < chromaSettings.size()) ? chromaSettings.at(i) : ChromaKeySettings{};
        if (!cropLayerEnabled)   crop = {};
        if (!chromaLayerEnabled) ck   = {};
        int duration = (i < frameDurations.size()) ? qMax(1, frameDurations.at(i)) : 1;

        // Prepare the (possibly crop+chroma-processed) image once
        QImage processedImg;
        if (!crop.isEmpty() || ck.enabled) {
            QImage img(imagePaths.at(i));
            if (!crop.isEmpty()) {
                QRect px(qRound(img.width()  * crop.x()),
                         qRound(img.height() * crop.y()),
                         qRound(img.width()  * crop.width()),
                         qRound(img.height() * crop.height()));
                img = img.copy(px);
            }
            if (ck.enabled)
                img = ChromaKeyProcessor::process(img, ck);
            processedImg = img;
        }

        // Write the frame `duration` times for variable hold time
        for (int d = 0; d < duration; ++d) {
            QString dest = tmpDir.filePath(
                QString("frame_%1.%2").arg(frameNum++, 4, 10, QChar('0')).arg(ext));
            if (!processedImg.isNull()) {
                if (!processedImg.save(dest, "JPEG", 95)) {
                    QMessageBox::critical(parentWidget, "Export",
                        QString("Failed to prepare frame %1 for export.").arg(i + 1));
                    return false;
                }
            } else if (!QFile::copy(imagePaths.at(i), dest)) {
                QMessageBox::critical(parentWidget, "Export",
                    QString("Failed to prepare frame %1 for export.").arg(i + 1));
                return false;
            }
        }
    }
    int totalFrames = frameNum - 1;

    QStringList args;
    args << "-y"
         << "-framerate" << QString::number(fps)
         << "-start_number" << "1"
         << "-i" << tmpDir.filePath("frame_%04d." + ext);

    // Mix in audio if provided
    bool hasAudio = !audioFilePath.isEmpty() && QFile::exists(audioFilePath);
    if (hasAudio)
        args << "-i" << audioFilePath;

    if (format == "mp4") {
        // scale filter rounds down to even dimensions required by yuv420p/H.264
        args << "-vf" << "scale=trunc(iw/2)*2:trunc(ih/2)*2"
             << "-c:v" << "libx264" << "-pix_fmt" << "yuv420p";
        if (hasAudio)
            args << "-c:a" << "aac" << "-b:a" << "192k";
    } else {
        args << "-c:v" << "libvpx-vp9" << "-b:v" << "0" << "-crf" << "33";
        if (hasAudio)
            args << "-c:a" << "libopus";
    }
    if (hasAudio)
        args << "-shortest";  // truncate to shortest stream

    args << outputPath;

    QProgressDialog progress("Exporting video…", "Cancel", 0, 0, parentWidget);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.show();

    QProcess proc;
    bool cancelled = false;

    QObject::connect(&progress, &QProgressDialog::canceled, &proc, [&]() {
        cancelled = true;
        proc.kill();
    });

    proc.start("ffmpeg", args);
    if (!proc.waitForStarted(5000)) {
        QMessageBox::critical(parentWidget, "Export",
            "Failed to start ffmpeg. Make sure it is installed and on your PATH.");
        return false;
    }

    while (!proc.waitForFinished(100)) {
        QCoreApplication::processEvents();
        if (cancelled)
            break;
    }

    progress.close();

    if (cancelled)
        return false;

    if (proc.exitCode() != 0) {
        QString err = QString::fromLocal8Bit(proc.readAllStandardError());
        QMessageBox::critical(parentWidget, "Export",
            QString("ffmpeg reported an error:\n\n%1").arg(err));
        return false;
    }

    return true;
}
