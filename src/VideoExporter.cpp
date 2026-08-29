#include "VideoExporter.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QString>
#include <QTemporaryDir>
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
    return proc.exitCode() == 0;
}

bool VideoExporter::exportVideo(const QVector<QString>& imagePaths,
                                 const QString& outputPath,
                                 int fps,
                                 const QString& format,
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

    for (int i = 0; i < imagePaths.size(); ++i) {
        QString dest = tmpDir.filePath(
            QString("frame_%1.%2").arg(i + 1, 4, 10, QChar('0')).arg(ext));
        if (!QFile::copy(imagePaths.at(i), dest)) {
            QMessageBox::critical(parentWidget, "Export",
                QString("Failed to prepare frame %1 for export.").arg(i + 1));
            return false;
        }
    }

    QStringList args;
    args << "-y"
         << "-framerate" << QString::number(fps)
         << "-start_number" << "1"
         << "-i" << tmpDir.filePath("frame_%04d." + ext);

    if (format == "mp4")
        args << "-c:v" << "libx264" << "-pix_fmt" << "yuv420p";
    else
        args << "-c:v" << "libvpx-vp9" << "-b:v" << "0" << "-crf" << "33";

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
